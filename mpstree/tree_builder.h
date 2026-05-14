/* tree_builder.h */
#ifndef TREE_BUILDER_H
#define TREE_BUILDER_H

#include "proc_reader.h"

typedef struct TreeNode {
    ProcessInfo      *proc;          /* Pointer to process data */
    struct TreeNode **children;      /* Dynamic array of child pointers */
    int               child_count;
    int               child_capacity;
} TreeNode;

/*
 * Build a process tree from the flat procs[] array.
 * root_pid: the PID to use as root (usually 1).
 * Returns a pointer to the root TreeNode, or NULL on error.
 * Caller must free with free_tree().
 */
TreeNode *build_tree(ProcessInfo *procs, int proc_count, int root_pid);

/* Sort children of every node alphabetically by name */
void sort_tree_by_name(TreeNode *node);

/* Free all memory allocated by build_tree() */
void free_tree(TreeNode *node);

#endif /* TREE_BUILDER_H */
