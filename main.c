#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "options.h"
#include "proc_reader.h"
#include "tree_builder.h"
#include "tree_printer.h"

int main(int argc, char *argv[]) {
    Options opts;

    /* Step 1: Décommenté et finalisé - Parse command-line options */
    if (parse_options(argc, argv, &opts) != 0) {
        return EXIT_FAILURE;
    }

    /* Step 2: Read all processes from /proc */
    ProcessInfo *procs = malloc(MAX_PROCESSES * sizeof(ProcessInfo));
    if (!procs) {
        perror("malloc procs");
        return EXIT_FAILURE;
    }

    /* Ajout de la gestion de l'option -T (masquer les threads) */
    int proc_count = read_all_processes(procs, MAX_PROCESSES, opts.hide_threads);
    if (proc_count < 0) {
        fprintf(stderr, "Error: failed to read /proc\n");
        free(procs);
        return EXIT_FAILURE;
    }

    /* Step 3: Determine the root PID */
    /* Si on veut juste voir les parents d'un PID (-s), on a besoin de construire l'arbre depuis PID 1 */
    int root_pid = (opts.target_pid > 0 && !opts.show_parents_only) ? opts.target_pid : 1;

    /* Step 4: Build the tree */
    TreeNode *root = build_tree(procs, proc_count, root_pid);
    if (!root) {
        fprintf(stderr, "Error: could not build process tree (is PID %d valid?)\n",
                root_pid);
        free(procs);
        return EXIT_FAILURE;
    }

    /* Step 4b: Filtrage des parents (-s) */
    if (opts.show_parents_only && opts.target_pid > 0) {
        filter_parents_only(root, opts.target_pid);
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
