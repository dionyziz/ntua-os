/*
 * pipesem.c
 */

#include <unistd.h>
#include <stdio.h>

/* The functions are intentionally left blank :-) */

#include "pipesem.h"

void pipesem_init(struct pipesem *sem, int val)
{
    int i;
    int f[ 2 ];
    int status;

    status = pipe(f);
    if (status < 0) {
        perror("Could not create semaphore");
        return;
    }

    sem->rfd = f[ 0 ];
    sem->wfd = f[ 1 ];

    for (i = 0; i < val; ++i) {
        pipesem_signal(sem);
    }
}

void pipesem_wait(struct pipesem *sem)
{
    char buffer[ 1 ];
    int status;

    status = read(sem->rfd, buffer, 1);

    if (status < 0) {
        perror("Could not wait on semaphore");
        return;
    }
}

void pipesem_signal(struct pipesem *sem)
{
    int status;
    status = write(sem->wfd, "_", 1);

    if (status < 0) {
        perror("Could not signal semaphore");
        return;
    }
}

void pipesem_destroy(struct pipesem *sem)
{
    int status;

    status = close(sem->rfd);
    if (status < 0) {
        perror("Could not destroy semaphore (read end indestructible)");
        return;
    }

    status = close(sem->wfd);
    if (status < 0) {
        perror("Could not destroy semaphore (write end indestructible)");
        return;
    }
}
