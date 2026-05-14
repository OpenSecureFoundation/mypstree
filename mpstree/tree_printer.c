/* tree_printer.c */
#include <stdio.h>
#include <string.h>
#include <unistd.h>   /* getpid() for -h option */
#include "tree_printer.h"

/* UTF-8 box-drawing characters for the tree */
#define BRANCH  "├─"   /* intermediate child */
#define LAST    "└─"   /* last child */
#define VERT    "│ "   /* vertical line continuation */
#define EMPTY   "  "   /* space where no line is needed */

/* ANSI escape codes for highlighting */
#define ANSI_BOLD    "\033[1m"
#define ANSI_RESET   "\033[0m"

/*
 * Recursive helper.
 * prefix: the string of connectors built up from ancestor levels
 * is_last: whether this node is the last child of its parent
 */
static void print_node(TreeNode *node, const char *prefix,
                        int is_last, const Options *opts,
                        int highlight_pid) {
    if (!node) return;

    /* Print the connector for this level */
    printf("%s", prefix);
    printf("%s", is_last ? LAST : BRANCH);

    /* Highlight if this is the target PID */
    int do_highlight = (opts->highlight && node->proc->pid == highlight_pid);
    if (do_highlight) printf("%s", ANSI_BOLD);

    /* Print process name */
    printf("%s", node->proc->name);

    /* -p: show PID */
    if (opts->show_pids) {
        printf("(%d)", node->proc->pid);
    }

    /* -a: show command line arguments */
    if (opts->show_args && node->proc->cmdline[0] != '\0') {
        printf(" %s", node->proc->cmdline);
    }

    if (do_highlight) printf("%s", ANSI_RESET);
    printf("\n");

    /* Build the prefix for children */
    char child_prefix[1024];
    snprintf(child_prefix, sizeof(child_prefix), "%s%s",
             prefix, is_last ? EMPTY : VERT);

    /* Recurse into children */
    for (int i = 0; i < node->child_count; i++) {
        int child_is_last = (i == node->child_count - 1);
        print_node(node->children[i], child_prefix,
                   child_is_last, opts, highlight_pid);
    }
}

void print_tree(TreeNode *node, const Options *opts, int highlight_pid) {
    if (!node) return;

    /* Print the root node without any prefix connector */
    int do_highlight = (opts->highlight && node->proc->pid == highlight_pid);
    if (do_highlight) printf("%s", ANSI_BOLD);

    printf("%s", node->proc->name);
    if (opts->show_pids)  printf("(%d)", node->proc->pid);
    if (opts->show_args && node->proc->cmdline[0] != '\0')
        printf(" %s", node->proc->cmdline);

    if (do_highlight) printf("%s", ANSI_RESET);
    printf("\n");

    /* Print all children */
    for (int i = 0; i < node->child_count; i++) {
        int is_last = (i == node->child_count - 1);
        print_node(node->children[i], "", is_last, opts, highlight_pid);
    }
}
