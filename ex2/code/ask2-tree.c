#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"
#include "tree.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

void fork_procs(struct tree_node* node)
{
    pid_t child;
    int i;

    change_pname(node->name);
    printf("%s: Hello! I have %i children.\n", node->name, node->nr_children);
    for (i = 0; i < node->nr_children; ++i) {
        printf("%s: Forking child %s...\n", node->name, node->children[i].name);
        child = fork();
        if (child == 0) {
            fork_procs(&node->children[i]);
        }
    }
    printf("%s: Sleeping...\n", node->name);
    for (i = 0; i < node->nr_children; ++i) {
        wait(NULL);
    }
    if (node->nr_children == 0) {
        sleep(SLEEP_PROC_SEC);
    }
    printf("%s: Goodbye...\n", node->name);
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

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
        exit(1);
    }

    root = get_tree_from_file(argv[1]);
    printf("Constructing the following process tree:\n");
    print_tree(root);

    /* Fork root of process tree */
    pid = fork();
    if (pid < 0) {
        perror("main: fork");
        exit(1);
    }
    if (pid == 0) {
        /* Child */
        fork_procs(root);
        exit(1);
    }

    sleep(SLEEP_TREE_SEC);

    /* Print the process tree root at pid */
    show_pstree(pid);

    /* Wait for the root of the process tree to terminate */
    pid = wait(&status);
    explain_wait_status(pid, status);

    return 0;
}
