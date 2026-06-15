#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "options.h"
#include "proc_reader.h"
#include "tree_builder.h"
#include "tree_printer.h"

int main(int argc, char *argv[]) {
    Options opts;
    
    if (parse_options(argc, argv, &opts) != 0) {
        return EXIT_FAILURE;
    }

    ProcessInfo *procs = calloc(MAX_PROCESSES, sizeof(ProcessInfo));
    if (!procs) {
        perror("calloc procs");
        return EXIT_FAILURE;
    }

    int proc_count = read_all_processes(procs, MAX_PROCESSES, opts.hide_threads);
    if (proc_count < 0) {
        fprintf(stderr, "Error: failed to read /proc\n");
        free(procs);
        return EXIT_FAILURE;
    }

    int root_pid = (opts.target_pid > 0 && !opts.show_parents_only) ? opts.target_pid : 1;
    
    TreeNode *root = build_tree(procs, proc_count, root_pid);
    if (!root) {
        fprintf(stderr, "Error: could not build process tree (is PID %d valid?)\n", root_pid);
        free(procs);
        return EXIT_FAILURE;
    }

    if (opts.show_parents_only && opts.target_pid > 0) {
        filter_parents_only(root, opts.target_pid);
    }

    // Le tri englobe desormais -n (pid) et -N (ns)
    sort_tree(root, &opts);

    // Mettre en evidence le processus lanceur et ses ancetres
    if (opts.highlight) {
        mark_highlight_path(root, getpid());
    }

    print_tree(root, &opts);

    free_tree(root);
    free(procs);
    
    return EXIT_SUCCESS;
}