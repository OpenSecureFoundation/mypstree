#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree_builder.h"

static TreeNode *create_node(ProcessInfo *proc) {
    TreeNode *node = calloc(1, sizeof(TreeNode));
    node->proc = proc;
    return node;
}

static void add_child(TreeNode *parent, TreeNode *child) {
    if (parent->child_count >= parent->child_capacity) {
        parent->child_capacity = (parent->child_capacity == 0) ? 4 : parent->child_capacity * 2;
        parent->children = realloc(parent->children, parent->child_capacity * sizeof(TreeNode *));
    }
    parent->children[parent->child_count++] = child;
}

static int compare_by_name(const void *a, const void *b) {
    return strcmp((*(TreeNode **)a)->proc->name, (*(TreeNode **)b)->proc->name);
}

static int compare_by_pid(const void *a, const void *b) {
    return (*(TreeNode **)a)->proc->pid - (*(TreeNode **)b)->proc->pid;
}

static int compare_by_ns(const void *a, const void *b) {
    return (*(TreeNode **)a)->proc->ns_net - (*(TreeNode **)b)->proc->ns_net;
}

void sort_tree(TreeNode *node, const Options *opts) {
    if (!node || node->child_count == 0) return;
    
    // Application du bon algorithme de tri
    if (opts->ns_sort) {
        qsort(node->children, node->child_count, sizeof(TreeNode *), compare_by_ns);
    } else if (opts->sort_by_pid) {
        qsort(node->children, node->child_count, sizeof(TreeNode *), compare_by_pid);
    } else {
        qsort(node->children, node->child_count, sizeof(TreeNode *), compare_by_name);
    }

    for (int i = 0; i < node->child_count; i++) {
        sort_tree(node->children[i], opts);
    }
}

TreeNode *build_tree(ProcessInfo *procs, int proc_count, int root_pid) {
    TreeNode **nodes = calloc(proc_count, sizeof(TreeNode *));
    for (int i = 0; i < proc_count; i++) nodes[i] = create_node(&procs[i]);

    TreeNode *root = NULL;
    for (int i = 0; i < proc_count; i++) {
        if (nodes[i]->proc->pid == root_pid) {
            root = nodes[i];
        } else {
            for (int j = 0; j < proc_count; j++) {
                if (nodes[j]->proc->pid == nodes[i]->proc->ppid) {
                    add_child(nodes[j], nodes[i]);
                    break;
                }
            }
        }
    }
    free(nodes);
    return root;
}

void free_tree(TreeNode *node) {
    if (!node) return;
    for (int i = 0; i < node->child_count; i++) free_tree(node->children[i]);
    free(node->children);
    free(node);
}

static int mark_parents(TreeNode *node, int target_pid) {
    if (!node) return 0;
    if (node->proc->pid == target_pid) { node->keep_for_parents = 1; return 1; }
    int keep = 0;
    for (int i = 0; i < node->child_count; i++) {
        if (mark_parents(node->children[i], target_pid)) keep = 1;
    }
    if (keep) node->keep_for_parents = 1;
    return keep;
}

static void prune_tree(TreeNode *node) {
    if (!node) return;
    int new_count = 0;
    for (int i = 0; i < node->child_count; i++) {
        if (node->children[i]->keep_for_parents) {
            node->children[new_count++] = node->children[i];
            prune_tree(node->children[i]);
        } else {
            free_tree(node->children[i]);
        }
    }
    node->child_count = new_count;
}

void filter_parents_only(TreeNode *node, int target_pid) {
    if (!node) return;
    mark_parents(node, target_pid);
    prune_tree(node);
}

static int recursive_highlight(TreeNode *node, int pid) {
    if (!node) return 0;
    if (node->proc->pid == pid) { node->highlight_path = 1; return 1; }
    for (int i = 0; i < node->child_count; i++) {
        if (recursive_highlight(node->children[i], pid)) {
            node->highlight_path = 1;
            return 1;
        }
    }
    return 0;
}

void mark_highlight_path(TreeNode *node, int target_pid) {
    recursive_highlight(node, target_pid);
}