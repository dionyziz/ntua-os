#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <sys/wait.h>
#include <sys/types.h>

#include "proc-common.h"
#include "request.h"

/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */

int *childid, *alive, current = 0, nproc;

static void
sigchld_handler(int signum);

/* SIGALRM handler: Gets called whenever an alarm goes off.
 * The time quantum of the currently executing process has expired,
 * so send it a SIGSTOP. The SIGCHLD handler will take care of
 * activating the next in line.
 */
static void
sigalrm_handler(int signum)
{
    fprintf(stderr, "SIGALRM Fired.\n");
    fprintf(stderr, "Stopping process %i with id = %i.\n", current, childid[current]);
    alarm(SCHED_TQ_SEC);
    kill(childid[current], SIGSTOP);
    // next();
	// assert(0 && "Please fill me!");
}

/* SIGCHLD handler: Gets called whenever a process is stopped,
 * terminated due to a signal, or exits gracefully.
 *
 * If the currently executing task has been stopped,
 * it means its time quantum has expired and a new one has
 * to be activated.
 */
static void
sigchld_handler(int signum)
{
	pid_t p;
	int status, next;

    fprintf(stderr, "SIGCHLD called.\n");

    /* Wait for any child, also get status for stopped children */
    p = waitpid(-1, &status, WUNTRACED|WCONTINUED);
    // fprintf(stderr, "SIGHCLD: Expected processid %i to stop; actually %i stopped.\n", childid[current], ( int )p );
    assert(( int )p == childid[current]);

    fprintf(stderr, "SIGCHILD Fired for process %i with id = %i and signum = %i.\n", current, childid[current], signum);
    next = 0;
    if (WIFEXITED(status)) {
        // child died
        fprintf(stderr, "Child %i died; had processid %i\n", current, childid[current]);
        alive[current] = 0;
        next = 1;
    }
    if (WIFSTOPPED(status)) {
        // child stopped by scheduler
        fprintf(stderr, "Child %i stopped; has processid %i\n", current, childid[current]);
        next = 1;
    }
    if (next == 0) {
        return;
    }
    fprintf(stderr, "Round robin scheduler: Waking up next process.\n");
    int count = 0;
    do {
        current = (current + 1) % nproc;
        ++count;
    } while (!alive[current] && count <= nproc);
    if (count == nproc + 1) {
        fprintf(stderr, "All processes are died. Exiting.\n");
        // everyone has died
        exit(0);
    }
    fprintf(stderr, "Waking up %i-th child with processid = %i.\n", current, childid[current]);
    kill(childid[current], SIGCONT);
    // next();
	// assert(0 && "Please fill me!");
}

/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
static void
install_signal_handlers(void)
{
	sigset_t sigset;
	struct sigaction sa;

    fprintf(stderr, "Installing signal handlers.\n");

	sa.sa_handler = sigchld_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	sigaddset(&sigset, SIGALRM);
	sa.sa_mask = sigset;
	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("sigaction: sigchld");
		exit(1);
	}

	sa.sa_handler = sigalrm_handler;
	if (sigaction(SIGALRM, &sa, NULL) < 0) {
		perror("sigaction: sigalrm");
		exit(1);
	}

	/*
	 * Ignore SIGPIPE, so that write()s to pipes
	 * with no reader do not result in us being killed,
	 * and write() returns EPIPE instead.
	 */
	if (signal(SIGPIPE, SIG_IGN) < 0) {
		perror("signal: sigpipe");
		exit(1);
	}
    fprintf(stderr, "Signal handlers installed successfully.\n");
}

int main(int argc, char *argv[])
{
	int i, child;
    char *childargs[] = { NULL }, *environment[] = { NULL };

    alive = ( int* )malloc((argc - 1) * sizeof(int));
    childid = ( int* )malloc((argc - 1) * sizeof(int));

	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */

    for (i = 0; i < argc - 1; ++i) {
        fprintf(stderr, "Forking scheduler child %i.\n", i);
        child = fork();
        if (child == 0) {
            fprintf(stderr, "Scheduler child %i created.\n", i);
            raise(SIGSTOP);
            execve(argv[i + 1], childargs, environment);
            fprintf(stderr, "Scheduler child: Unreachable point.\n");
            return 0;
        }
        else {
            alive[i] = 1;
            childid[i] = child;
            fprintf(stderr, "Forked child with processid = %i.\n", child);
        }
    }

	nproc = argc - 1; /* number of proccesses goes here */

    fprintf(stderr, "Waiting for forks of %i children to complete.\n", nproc);

	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc);

    fprintf(stderr, "All %i children forked successully. Ready to schedule.\n", nproc);

	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();

	if (nproc == 0) {
		fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
		exit(1);
	}

    alarm(SCHED_TQ_SEC);

    // start first process in the queue
    current = 0;
    kill(childid[current], SIGCONT);

	/* loop forever  until we exit from inside a signal handler. */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
