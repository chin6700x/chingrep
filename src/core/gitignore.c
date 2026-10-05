/*
 * Author: Wirote Chukeaw (chin6700x)
 * Copyright: (c) 2026 Wirote Chukeaw. All rights reserved.
 * License: MIT License
 */

#include "gitignore.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int glob_match(const char *pat, const char *str) {
    while (*pat) {
        if (*pat == '*') {
            while (*(pat + 1) == '*') pat++;
            if (!*(pat + 1)) return 1;
            while (*str) {
                if (glob_match(pat + 1, str)) return 1;
                str++;
            }
            return 0;
        } else if (*pat == '?' || *pat == *str) {
            pat++;
            str++;
        } else {
            return 0;
        }
    }
    return *str == '\0';
}

void gitignore_init(gitignore_t *gi) {
    if (!gi) return;
    gi->count = 0;

    // Fast-path common directories & files
    const char *defaults[] = {
        ".git", "node_modules", ".DS_Store", "dist", ".idea", ".vscode", "target", "build", "coverage", ".next", ".nuxt"
    };
    int n = sizeof(defaults) / sizeof(defaults[0]);
    for (int i = 0; i < n && gi->count < MAX_IGNORE_RULES; i++) {
        gi->rules[gi->count].pattern = strdup(defaults[i]);
        gi->rules[gi->count].is_dir_only = 0;
        gi->rules[gi->count].is_glob = 0;
        gi->count++;
    }
}

void gitignore_load_file(gitignore_t *gi, const char *dir_path) {
    if (!gi || !dir_path) return;

    char path[1024];
    snprintf(path, sizeof(path), "%s/.gitignore", dir_path);

    FILE *f = fopen(path, "r");
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f) && gi->count < MAX_IGNORE_RULES) {
        // Strip trailing newline / carriage return
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || line[len - 1] == ' ')) {
            line[--len] = '\0';
        }

        // Skip comments and empty lines
        if (len == 0 || line[0] == '#' || line[0] == '!') continue;

        int is_dir_only = 0;
        if (line[len - 1] == '/') {
            is_dir_only = 1;
            line[--len] = '\0';
        }

        int is_glob = (strchr(line, '*') != NULL || strchr(line, '?') != NULL);

        gi->rules[gi->count].pattern = strdup(line);
        gi->rules[gi->count].is_dir_only = is_dir_only;
        gi->rules[gi->count].is_glob = is_glob;
        gi->count++;
    }

    fclose(f);
}

int gitignore_is_ignored(const gitignore_t *gi, const char *rel_path, const char *name, int is_dir) {
    if (!gi || !name) return 0;

    // Direct fast-path check for .git and node_modules
    if (strcmp(name, ".git") == 0 || strcmp(name, "node_modules") == 0 || strcmp(name, ".DS_Store") == 0) {
        return 1;
    }

    for (int i = 0; i < gi->count; i++) {
        const ignore_rule_t *rule = &gi->rules[i];
        if (rule->is_dir_only && !is_dir) continue;

        if (rule->is_glob) {
            if (glob_match(rule->pattern, name)) return 1;
            if (rel_path && glob_match(rule->pattern, rel_path)) return 1;
        } else {
            if (strcmp(rule->pattern, name) == 0) return 1;
            if (rel_path && strcmp(rule->pattern, rel_path) == 0) return 1;
        }
    }
    return 0;
}

void gitignore_free(gitignore_t *gi) {
    if (!gi) return;
    for (int i = 0; i < gi->count; i++) {
        if (gi->rules[i].pattern) {
            free(gi->rules[i].pattern);
            gi->rules[i].pattern = NULL;
        }
    }
    gi->count = 0;
}
