#include <stdio.h>

int main() {
    unsigned int a[3];
    a[ 0 ] = 0;
    a[ 1 ] = 0xffffffff;
    a[ 2 ] = 0xff000000;

    printf( "%u\n", a[0] );
    printf( "%u\n", a[1] );
    printf( "%u\n\n", a[2] );

    printf( "%u\n", * ( (int*) ( ( (char*) a ) + 1 ) ) );
    printf( "%u\n", * ( (int*) ( (char*) ( a + 1 ) ) ) );
    printf( "%u\n", * ( (int*) ( (char*) a + 1 ) ) );
    return 0;
}
