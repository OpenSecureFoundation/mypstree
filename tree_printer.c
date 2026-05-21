#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "tree_printer.h"

/* UTF-8 box-drawing characters */
#define U_BRANCH  "├─"
#define U_LAST    "└─"
#define U_VERT    "│ "
#define U_EMPTY   "  "

/* ASCII box-drawing characters (Option -A) */
#define A_BRANCH  "|-"
#define A_LAST    "`-"
#define A_VERT    "| "
#define A_EMPTY   "  "

#define ANSI_BOLD    "\033[1m"
#define ANSI_RESET   "\033[0m"

static void print_node(TreeNode *node, const char *prefix,
                        int is_last, const Options *opts,
                        int highlight_pid) {
    if (!node) return;

    printf("%s", prefix);

    /* Choix entre ASCII et UTF-8 */
    if (opts->ascii_trace) {
        printf("%s", is_last ? A_LAST : A_BRANCH);
    } else {
        printf("%s", is_last ? U_LAST : U_BRANCH);
    }

    int do_highlight = (opts->highlight && node->proc->pid == highlight_pid);
    if (do_highlight) printf("%s", ANSI_BOLD);

    printf("%s", node->proc->name);

    /* Options d'affichage */
    if (opts->show_pids) {
        printf("(%d)", node->proc->pid);
    }
    if (opts->show_pgid) {
        printf("[pgid:%d]", node->proc->pgid);
    }
    if (opts->show_args && node->proc->cmdline[0] != '\0') {
        printf(" %s", node->proc->cmdline);
    }

    if (do_highlight) printf("%s", ANSI_RESET);
    printf("\n");

    /* Préparation du préfixe enfant */
    char child_prefix[1024];
    if (opts->ascii_trace) {
        snprintf(child_prefix, sizeof(child_prefix), "%s%s",
                 prefix, is_last ? A_EMPTY : A_VERT);
    } else {
        snprintf(child_prefix, sizeof(child_prefix), "%s%s",
                 prefix, is_last ? U_EMPTY : U_VERT);
    }

    /* Récursion */
    for (int i = 0; i < node->child_count; i++) {
        int child_is_last = (i == node->child_count - 1);
        print_node(node->children[i], child_prefix,
                   child_is_last, opts, highlight_pid);
    }
}

void print_tree(TreeNode *node, const Options *opts, int highlight_pid) {
    if (!node) return;

    int do_highlight = (opts->highlight && node->proc->pid == highlight_pid);
    if (do_highlight) printf("%s", ANSI_BOLD);

    printf("%s", node->proc->name);

    if (opts->show_pids) printf("(%d)", node->proc->pid);
    if (opts->show_pgid) printf("[pgid:%d]", node->proc->pgid);

    if (opts->show_args && node->proc->cmdline[0] != '\0') {
        printf(" %s", node->proc->cmdline);
    }

    if (do_highlight) printf("%s", ANSI_RESET);
    printf("\n");

    for (int i = 0; i < node->child_count; i++) {
        int is_last = (i == node->child_count - 1);
        print_node(node->children[i], "", is_last, opts, highlight_pid);
    }
}
