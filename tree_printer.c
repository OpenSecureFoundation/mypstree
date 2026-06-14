#include <stdio.h>
#include <string.h>
#include "tree_printer.h"

/* UTF-8 */
#define U_BRANCH  "├─"
#define U_LAST    "└─"
#define U_VERT    "│ "
#define U_EMPTY   "  "

/* ASCII */
#define A_BRANCH  "|-"
#define A_LAST    "`-"
#define A_VERT    "| "
#define A_EMPTY   "  "

/* ANSI Colors */
#define ANSI_BOLD    "\033[1m"
#define ANSI_RESET   "\033[0m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_RED     "\033[31m"

static void set_color(TreeNode *node, const Options *opts) {
    if (node->highlight_path) {
        printf("%s", ANSI_BOLD);
    } else if (opts->color_attr && strcmp(opts->color_attr, "age") == 0) {
        // Logique fictive age : rouge ancien, vert recent
        if (node->proc->starttime < 10000) printf("%s", ANSI_RED);
        else printf("%s", ANSI_GREEN);
    }
}

static void print_node(TreeNode *node, const char *prefix, int is_last, const Options *opts, int duplicate_count) {
    if (!node) return;
    
    printf("%s", prefix);
    if (opts->ascii_trace) printf("%s", is_last ? A_LAST : A_BRANCH);
    else printf("%s", is_last ? U_LAST : U_BRANCH);

    // Transitions Namespace (-S)
    if (opts->show_ns_changes && node->proc->ppid != 0) {
        printf("(ns)%s", opts->ascii_trace ? "-" : "─");
    }

    set_color(node, opts);

    // Affichage compact
    if (duplicate_count > 1) printf("%d*[", duplicate_count);

    // Format threads
    if (node->proc->is_thread) {
        if (opts->show_full_threads) printf("%s", node->proc->name);
        else printf("{%s}", node->proc->name);
    } else {
        printf("%s", node->proc->name);
    }

    if (duplicate_count > 1) printf("]");

    // Arguments supplementaires
    if (opts->show_selinux) printf(" [%s]", node->proc->selinux_context);
    if (opts->show_pids) printf("(%d)", node->proc->pid);
    if (opts->show_pgid) printf("[pgid:%d]", node->proc->pgid);
    if (opts->show_args && node->proc->cmdline[0] != '\0') printf(" %s", node->proc->cmdline);
    
    printf("%s\n", ANSI_RESET);

    // Preparation prefixe enfant
    char child_prefix[1024];
    if (opts->ascii_trace) snprintf(child_prefix, sizeof(child_prefix), "%s%s", prefix, is_last ? A_EMPTY : A_VERT);
    else snprintf(child_prefix, sizeof(child_prefix), "%s%s", prefix, is_last ? U_EMPTY : U_VERT);

    // Logique de compactage enfant
    int do_compact = !opts->disable_compact && !opts->show_pids;

    for (int i = 0; i < node->child_count; ) {
        int count = 1;
        if (do_compact) {
            while (i + count < node->child_count && 
                   strcmp(node->children[i]->proc->name, node->children[i + count]->proc->name) == 0 &&
                   node->children[i]->child_count == 0 && node->children[i + count]->child_count == 0) {
                count++;
            }
        }
        int child_is_last = (i + count >= node->child_count);
        print_node(node->children[i], child_prefix, child_is_last, opts, count);
        i += count;
    }
}

void print_tree(TreeNode *node, const Options *opts) {
    if (!node) return;

    set_color(node, opts);
    printf("%s", node->proc->name);
    if (opts->show_selinux) printf(" [%s]", node->proc->selinux_context);
    if (opts->show_pids) printf("(%d)", node->proc->pid);
    printf("%s\n", ANSI_RESET);

    int do_compact = !opts->disable_compact && !opts->show_pids;

    for (int i = 0; i < node->child_count; ) {
        int count = 1;
        if (do_compact) {
            while (i + count < node->child_count && 
                   strcmp(node->children[i]->proc->name, node->children[i + count]->proc->name) == 0 &&
                   node->children[i]->child_count == 0 && node->children[i + count]->child_count == 0) {
                count++;
            }
        }
        int is_last = (i + count >= node->child_count);
        print_node(node->children[i], "", is_last, opts, count);
        i += count;
    }
}