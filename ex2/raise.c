#include <stdio.h>
#include <signal.h>

int main() {
    printf( "Before raise.\n" );
    raise( SIGSTOP );
    printf( "After raise.\n" );
    return 0;
}
