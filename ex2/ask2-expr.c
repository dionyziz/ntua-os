#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "proc-common.h"
#include "tree.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

const int INT_LEN = sizeof( int );

int do_write(int fd, int data)
{
    int bytes_written = 0;
    int status;

    while (bytes_written < INT_LEN) {
        status = write(fd, (void*)(&data) + bytes_written, INT_LEN - bytes_written);
        if ( status < 0 ) {
            return -1;
        }
        bytes_written += status;
    }
    return 0;
}

int do_read(int fd, int *data)
{
    int bytes_read;
    int status;

    while (bytes_read < INT_LEN) {
        status = read(fd, (void*)(data) + bytes_read, INT_LEN - bytes_read);
        if (status < 0) {
            return -1;
        }
        bytes_read += status;
    }

    return 0;
}

void fork_procs(struct tree_node node, int out)
{
    pid_t child;
    pid_t children[2];
    int i;
    int f[2][2];
    int a, b;
    int result;

	change_pname(node.name);
    printf("%s: Hello! I have %i children.\n", node.name, node.nr_children);
    if (node.nr_children == 0) {
        do_write(out, atoi(node.name));
        exit(0);
    }
    for (i = 0; i < node.nr_children; ++i) {
        printf("%s: Forking child %s...\n", node.name, node.children[i].name);
        pipe(f[i]);
        child = fork();
        if (child < 0) {
            perror("fork_procs: fork");
            exit(1);
        }
        if (child == 0) {
            close(f[i][0]);
            fork_procs(node.children[i], f[i][1]);
            // this point is never reached
        }
        children[i] = child;
        close(f[i][1]);
    }

    do_read(f[0][0], &a);
    do_read(f[1][0], &b);

    switch (node.name[0]) {
        case '+':
            result = a + b;
            break;
        case '*':
            result = a * b;
            break;
    }
    do_write(out, result);

	printf("%s: Goodbye...\n", node.name);
	exit(0);
}

/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 */
int main(int argc, char** argv)
{
	pid_t pid;
	int status;
	struct tree_node *root;
    int f[2];
    int result;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
		exit(1);
	}

	root = get_tree_from_file(argv[1]);
    printf("Constructing the following process tree:\n");
	print_tree(root);

	/* Fork root of process tree */
    pipe(f);
	pid = fork();
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		/* Child */
        close(f[0]);
		fork_procs(*root, f[1]);
		exit(1);
	}
    close(f[1]);
    do_read(f[0], &result);

    printf("Final result: %i\n", result);

	/* Print the process tree root at pid */
	// show_pstree(pid);

	/* Wait for the root of the process tree to terminate */
	pid = wait(&status);
	explain_wait_status(pid, status);

	return 0;
}
