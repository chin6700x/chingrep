/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw. All rights reserved.
 * License: MIT License
 */

#ifndef CHINGREP_GITIGNORE_H
#define CHINGREP_GITIGNORE_H

#include <stddef.h>

#define MAX_IGNORE_RULES 512

typedef struct {
    char *pattern;
    int is_dir_only;
    int is_glob;
} ignore_rule_t;

typedef struct gitignore_stack {
    ignore_rule_t rules[MAX_IGNORE_RULES];
    int count;
} gitignore_t;

// Initializes gitignore engine with built-in standard pruning (.git, node_modules, dist, etc.)
void gitignore_init(gitignore_t *gi);

// Adds rules from a .gitignore file if found in dir_path
void gitignore_load_file(gitignore_t *gi, const char *dir_path);

// Returns 1 if path/filename should be ignored, 0 otherwise
int gitignore_is_ignored(const gitignore_t *gi, const char *rel_path, const char *name, int is_dir);

// Frees dynamically allocated pattern strings
void gitignore_free(gitignore_t *gi);

#endif // ASMGREP_GITIGNORE_H
