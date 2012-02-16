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
#define SCHED_TQ_SEC 1                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */
#define SHELL_EXECUTABLE_NAME "shell" /* executable for shell */

const int PROC_NUM = 20;
const int PRIORITY_LOW = 0;
const int PRIORITY_HIGH = 1;

pid_t* childid;
char** procnames;
int* alive;
int* priorities;
int current = 0, nproc;

/* Print a list of all tasks currently being scheduled.  */
static void
sched_print_tasks(void)
{
    int i;

    printf("Process scheduler: The following processes are currently running:\nname\tid\tpid\tactive\n");
    // normally this would be implemented using a linked list
    for (i = 0; i < PROC_NUM; ++i) {
        if (alive[i]) {
            printf("%s\t%i\t%i\t", procnames[i], i, childid[i]);
            if (current == i) {
                printf("*");
            }
            printf("\n");
        }
    }
	// assert(0 && "Please fill me!");
}

/* Send SIGKILL to a task determined by the value of its
 * scheduler-specific id.
 */
static int
sched_kill_task_by_id(int id)
{
    assert(id < PROC_NUM);
    assert(alive[id]);
    fprintf(stderr, "Killing process %i with pid %i.\n", id, childid[id]);
    kill(childid[id], SIGKILL);
	// assert(0 && "Please fill me!");
    return 0;
	// return -ENOSYS;
}

static void
sched_high_priority(int id)
{
    priorities[id] = PRIORITY_HIGH;
}

static void
sched_low_priority(int id)
{
    priorities[id] = PRIORITY_LOW;
}

/* Create a new task.  */
static void
sched_create_task(char *executable)
{
    int i;
    pid_t child;
    char *childargs[] = { NULL }, *environment[] = { NULL };

    for (i = 0; i < PROC_NUM; ++i) {
        if (!alive[i]) {
            child = fork();
            if (child == 0) {
                // child
                fprintf(stderr, "Forked new child %s.\n", executable);
                raise(SIGSTOP);
                execve(executable, childargs, environment);
                fprintf(stderr, "Unreachable point.\n");
                return;
            }
            // parent
            // potential race condition here: child has not yet reached SIGSTOP, but it is woken up by the scheduler
            strncpy(procnames[i], executable, 20);
            childid[i] = child;
            alive[i] = 1;

            return;
        }
    }
	// assert(0 && "Please fill me!");
} 

/* Process requests by the shell.  */
static int
process_request(struct request_struct *rq)
{
	switch (rq->request_no) {
		case REQ_PRINT_TASKS:
			sched_print_tasks();
			return 0;

		case REQ_KILL_TASK:
			return sched_kill_task_by_id(rq->task_arg);

		case REQ_EXEC_TASK:
			sched_create_task(rq->exec_task_arg);
			return 0;

        case REQ_HIGH_TASK:
            sched_high_priority(rq->task_arg);
            return 0;

        case REQ_LOW_TASK:
            sched_low_priority(rq->task_arg);
            return 0;

		default:
			return -ENOSYS;
	}
}

/* SIGALRM handler: Gets called whenever an alarm goes off.
 * The time quantum of the currently executing process has expired,
 * so send it a SIGSTOP. The SIGCHLD handler will take care of
 * activating the next in line.
 */
static void
sigalrm_handler(int signum)
{
    alarm(SCHED_TQ_SEC);
    kill(childid[current], SIGSTOP);
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
	int i, status, next, id;

    // fprintf(stderr, "SIGCHLD handler: Nproc is %i.\n", nproc);
    // fprintf(stderr, "SIGCHLD called.\n");

    /* Wait for any child, also get status for stopped children */
    p = waitpid(-1, &status, WUNTRACED|WCONTINUED);
    // find the id of the process that died
    id = -1;
    for (i = 0; i < PROC_NUM; ++i) {
        if (p == childid[i]) {
            id = i;
        }
    }
    // make sure one of our children died
    assert(id > -1);
    // fprintf(stderr, "SIGHCLD: Expected processid %i to stop; actually %i stopped.\n", childid[current], ( int )p );

    // fprintf(stderr, "SIGCHILD Fired for process %i with id = %i and signum = %i.\n", current, childid[current], signum);
    next = 0;
    if (WIFEXITED(status)) {
        // child died
        fprintf(stderr, "Child %i died; had processid %i\n", id, childid[id]);
        alive[id] = 0;
        next = 1;
    }
    if (WIFSIGNALED(status)) {
        fprintf(stderr, "Child %i which had processid %i was killed by a signal.\n", id, childid[id]);
        alive[id] = 0;
        next = 1;
    }
    if (WIFSTOPPED(status)) {
        // child stopped by scheduler
        // fprintf(stderr, "Child %i stopped; has processid %i\n", current, childid[current]);
        next = 1;
    }
    if (id != current) {
        // a different process than the current one was killed
        // so we don't need to wake up a different process
        return;
    }
    if (next == 0) {
        return;
    }
    // fprintf(stderr, "Round robin scheduler: Waking up next process.\n");
    int count = 0;
    int searchstart = current;
    // fprintf(stderr, "Entering do loop.\n");
    // look for high priority job
    do {
        // fprintf(stderr, "Inside do loop.\n");
        ++current;
        // fprintf(stderr, "Current has increased.\n");
        // fprintf(stderr, "Flipping current %i around the %i-clock.\n", current, nproc);
        current = current % PROC_NUM;
        // fprintf(stderr, "Current has gone around the clock.\n");
        ++count;
        // fprintf(stderr, "Checking if %i is still alive.\n", current);
    } while ((!alive[current] || priorities[current] == PRIORITY_LOW) && count <= PROC_NUM);
    if (count == PROC_NUM + 1) {
        // no high priority job found
        // look for low priority job
        current = searchstart;
        count = 0;
        do {
            // fprintf(stderr, "Inside do loop.\n");
            ++current;
            // fprintf(stderr, "Current has increased.\n");
            // fprintf(stderr, "Flipping current %i around the %i-clock.\n", current, nproc);
            current = current % PROC_NUM;
            // fprintf(stderr, "Current has gone around the clock.\n");
            ++count;
            // fprintf(stderr, "Checking if %i is still alive.\n", current);
        } while (!alive[current] && count <= PROC_NUM);
        if (count == PROC_NUM + 1) {
            fprintf(stderr, "All processes are died. Exiting.\n");
            // everyone has died
            exit(0);
        }
    }
    else {
        fprintf(stderr, "Found job with high priority to wake up.\n");
    }
    // fprintf(stderr, "Round robin scheduler: Waking up %i-th child with processid = %i.\n", current, childid[current]);
    kill(childid[current], SIGCONT);
	// assert(0 && "Please fill me!");
}

/* Disable delivery of SIGALRM and SIGCHLD. */
static void
signals_disable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
		perror("signals_disable: sigprocmask");
		exit(1);
	}
}

/* Enable delivery of SIGALRM and SIGCHLD.  */
static void
signals_enable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
		perror("signals_enable: sigprocmask");
		exit(1);
	}
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
}

static void
do_shell(char *executable, int wfd, int rfd)
{
	char arg1[10], arg2[10];
	char *newargv[] = { executable, NULL, NULL, NULL };
	char *newenviron[] = { NULL };

	sprintf(arg1, "%05d", wfd);
	sprintf(arg2, "%05d", rfd);
	newargv[1] = arg1;
	newargv[2] = arg2;

	raise(SIGSTOP);
	execve(executable, newargv, newenviron);

	/* execve() only returns on error */
	perror("scheduler: child: execve");
	exit(1);
}

/* Create a new shell task.
 *
 * The shell gets special treatment:
 * two pipes are created for communication and passed
 * as command-line arguments to the executable.
 */
static pid_t
sched_create_shell(char *executable, int *request_fd, int *return_fd)
{
	pid_t p;
	int pfds_rq[2], pfds_ret[2];

	if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
		perror("pipe");
		exit(1);
	}

	p = fork();
	if (p < 0) {
		perror("scheduler: fork");
		exit(1);
	}

	if (p == 0) {
		/* Child */
		close(pfds_rq[0]);
		close(pfds_ret[1]);
		do_shell(executable, pfds_rq[1], pfds_ret[0]);
		assert(0);
	}
	/* Parent */
	close(pfds_rq[1]);
	close(pfds_ret[0]);
	*request_fd = pfds_rq[0];
	*return_fd = pfds_ret[1];

    return p;
}

static void
shell_request_loop(int request_fd, int return_fd)
{
	int ret;
	struct request_struct rq;

	/*
	 * Keep receiving requests from the shell.
	 */
	for (;;) {
		if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
			perror("scheduler: read from shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}

		signals_disable();
		ret = process_request(&rq);
		signals_enable();

		if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
			perror("scheduler: write to shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	int i, child;
    char *childargs[] = { NULL }, *environment[] = { NULL };
	/* Two file descriptors for communication with the shell */
	static int request_fd, return_fd;

	nproc = argc; /* number of proccesses goes here */
    assert(nproc < PROC_NUM);
    // fprintf(stderr, "nproc is %i.\n", nproc);
    alive = ( int* )malloc(PROC_NUM * sizeof(int));
    childid = ( pid_t* )malloc(PROC_NUM * sizeof(pid_t));
    priorities = ( int* )malloc(PROC_NUM * sizeof(int));
    procnames = ( char** )malloc(PROC_NUM * sizeof(char*));
    for (i = 0; i < PROC_NUM; ++i) {
        procnames[i] = ( char* )malloc(30 * sizeof(char));
    }

	/* Create the shell. */
	childid[0] = sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);
    alive[0] = 1;
    procnames[0] = "shell";
    priorities[0] = PRIORITY_LOW;

	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */
    for (i = 1; i < argc; ++i) {
        child = fork();
        if (child == 0) {
            fprintf(stderr, "Scheduler child %i created.\n", i);
            raise(SIGSTOP);
            execve(argv[i], childargs, environment);
            fprintf(stderr, "Scheduler child: Unreachable point.\n");
            return 1;
        }
        else {
            fprintf(stderr, "Forked scheduler child %i (%s): PID = %i.\n", i, argv[i], child);
            alive[i] = 1;
            childid[i] = child;
            priorities[i] = PRIORITY_LOW;
            strncpy(procnames[i], argv[i], 20);
            // fprintf(stderr, "Forked child with processid = %i.\n", child);
        }
    }

	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc);

	if (nproc == 0) {
		fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
		exit(1);
	}
    // fprintf(stderr, "Before installing signal handlers: Nproc is %i.\n", nproc);

	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();

    alarm(SCHED_TQ_SEC);

    // start first process in the queue
    current = 0;
    // fprintf(stderr, "Before the kill: Nproc is %i.\n", nproc);
    kill(childid[current], SIGCONT);

	shell_request_loop(request_fd, return_fd);

	/* Now that the shell is gone, just loop forever
	 * until we exit from inside a signal handler.
	 */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
