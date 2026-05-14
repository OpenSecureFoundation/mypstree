/* tree_builder.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree_builder.h"

/* Allocate and initialize a TreeNode wrapping the given ProcessInfo */
static TreeNode *create_node(ProcessInfo *proc) {
    TreeNode *node = malloc(sizeof(TreeNode));
    if (!node) {
        perror("malloc TreeNode");
        return NULL;
    }
    node->proc           = proc;
    node->children       = NULL;
    node->child_count    = 0;
    node->child_capacity = 0;
    return node;
}

/* Add a child to a parent node, growing the children array if needed */
static int add_child(TreeNode *parent, TreeNode *child) {
    if (parent->child_count >= parent->child_capacity) {
        /* Double the capacity (start at 4) */
        int new_cap = (parent->child_capacity == 0) ? 4 : parent->child_capacity * 2;
        TreeNode **new_children = realloc(parent->children,
                                          new_cap * sizeof(TreeNode *));
        if (!new_children) {
            perror("realloc children");
            return -1;
        }
        parent->children       = new_children;
        parent->child_capacity = new_cap;
    }
    parent->children[parent->child_count++] = child;
    return 0;
}

/* Find a TreeNode by PID in a flat array of node pointers */
static TreeNode *find_node_by_pid(TreeNode **nodes, int count, int pid) {
    for (int i = 0; i < count; i++) {
        if (nodes[i] && nodes[i]->proc->pid == pid) return nodes[i];
    }
    return NULL;
}

/* Comparison function for qsort — sort by process name */
static int compare_by_name(const void *a, const void *b) {
    TreeNode *na = *(TreeNode **)a;
    TreeNode *nb = *(TreeNode **)b;
    return strcmp(na->proc->name, nb->proc->name);
}

void sort_tree_by_name(TreeNode *node) {
    if (!node || node->child_count == 0) return;

    qsort(node->children, node->child_count, sizeof(TreeNode *), compare_by_name);

    /* Recursively sort all subtrees */
    for (int i = 0; i < node->child_count; i++) {
        sort_tree_by_name(node->children[i]);
    }
}

TreeNode *build_tree(ProcessInfo *procs, int proc_count, int root_pid) {
    /* Step 1: Create one TreeNode per process */
    TreeNode **nodes = calloc(proc_count, sizeof(TreeNode *));
    if (!nodes) {
        perror("calloc nodes");
        return NULL;
    }

    for (int i = 0; i < proc_count; i++) {
        nodes[i] = create_node(&procs[i]);
        if (!nodes[i]) {
            /* Cleanup on failure */
            for (int j = 0; j < i; j++) free(nodes[j]);
            free(nodes);
            return NULL;
        }
    }

    /* Step 2: Link each node to its parent */
    TreeNode *root = NULL;

    for (int i = 0; i < proc_count; i++) {
        int pid  = nodes[i]->proc->pid;
        int ppid = nodes[i]->proc->ppid;

        if (pid == root_pid) {
            root = nodes[i];
            continue;
        }

        /* Find the parent and add this node as a child */
        TreeNode *parent = find_node_by_pid(nodes, proc_count, ppid);
        if (parent) {
            add_child(parent, nodes[i]);
        }
        /* If no parent found, the process is an orphan — we silently skip it.
         * In a real system, init/systemd adopts orphans, so this is rare. */
    }

    free(nodes);  /* Free the lookup array (not the nodes themselves) */
    return root;
}

void free_tree(TreeNode *node) {
    if (!node) return;
    /* Recursively free children first */
    for (int i = 0; i < node->child_count; i++) {
        free_tree(node->children[i]);
    }
    free(node->children);
    free(node);
}
