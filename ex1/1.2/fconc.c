#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

const char* const FCONC_MESSAGE_USAGE = "Usage: fconc infile1 infile2 ... outfile\n";
const char* const FCONC_MESSAGE_FILE_NOT_FOUND = "Unable to open file: %s\n"; // used both for reading and writing
const char* const FCONC_DEFAULT_OUT = "fconc.out";

int doWrite( int fd, const char* buff, int len ) {
    int written = 0;

    do {
        int code = write( outFD, buff + written, len - written );
        if ( code < 0 ) {
            return -1;
        }
        written += code;
    } while ( written < size );

    return 0;
}

int copyContents( int outFD, int inFD ) {
    int err;
    int size;
    char buffer[ 1024 ];

    while ( ( size = read( inFD, buffer, 1024 ) ) ) {
        doWrite( inFD, buffer, size );
    }
    if ( size == -1 ) {
        return -1;
    }
    return 0;
}

int parseArgs( int argc, char** argv, char*** inFiles, int* count, char** outFile ) {
    if ( argc < 3 ) {
        fprintf( stderr, FCONC_MESSAGE_USAGE );
        return 1;
    }

    *inFiles = ( char** )malloc( ( argc - 2 ) * sizeof( char* ) );
    
    for ( int i = 0; i < argc - 2; i++ ) {
        ( *inFiles )[ i ] = argv[ i + 1 ];
    }

    *outFile = argv[ argc - 1 ];

    return 0;
}

int safeOpen( char* file, int flags ) {
    int fd = open( file, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );

    if ( fd == -1 ) {
        fprintf( stderr, FCONC_MESSAGE_FILE_NOT_FOUND, file );
        return -1;
    }
    return fd;
}

int main( int argc, char** argv ) {
    char **inFiles, *outFile;
    int inFD1, inFD2, outFD;

    if ( parseArgs( argc, argv, &inFiles, &outFile ) ) {
        return 2;
    }

    if ( ( outFD = safeOpen( outFile, O_WRONLY | O_CREAT ) ) == -1
      || ( inFD1 = safeOpen( inFile1, O_RDONLY ) ) == -1
      || ( inFD2 = safeOpen( inFile2, O_RDONLY ) ) == -1 ) {
        return 3;
    }

    copyContents( outFD, inFD1 );
    copyContents( outFD, inFD2 );

    close( outFD );
    close( inFD1 );
    close( inFD2 );

    return 0;
}
