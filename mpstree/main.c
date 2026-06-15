/* main.c */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   /* getpid() */
#include "options.h"
#include "proc_reader.h"
#include "tree_builder.h"
#include "tree_printer.h"

int main(int argc, char *argv[]) {
    Options opts;

    /* Step 1: Parse command-line options */
    if (parse_options(argc, argv, &opts) != 0) {
        return EXIT_FAILURE;
    }

    /* Step 2: Read all processes from /proc */
    ProcessInfo *procs = malloc(MAX_PROCESSES * sizeof(ProcessInfo));
    if (!procs) {
        perror("malloc procs");
        return EXIT_FAILURE;
    }

    int proc_count = read_all_processes(procs, MAX_PROCESSES);
    if (proc_count < 0) {
        fprintf(stderr, "Error: failed to read /proc\n");
        free(procs);
        return EXIT_FAILURE;
    }

    /* Step 3: Determine the root PID */
    int root_pid = (opts.target_pid > 0) ? opts.target_pid : 1;

    /* Step 4: Build the tree */
    TreeNode *root = build_tree(procs, proc_count, root_pid);
    if (!root) {
        fprintf(stderr, "Error: could not build process tree (is PID %d valid?)\n",
                root_pid);
        free(procs);
        return EXIT_FAILURE;
    }

    /* Step 5: Sort if -n option given */
    if (opts.sort_by_name) {
        sort_tree_by_name(root);
    }

    /* Step 6: Determine highlight PID (-h highlights current shell's ancestors) */
    int highlight_pid = opts.highlight ? (int)getpid() : -1;

    /* Step 7: Print the tree */
    print_tree(root, &opts, highlight_pid);

    /* Step 8: Clean up */
    free_tree(root);
    free(procs);

    return EXIT_SUCCESS;
}
