#include "walker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

static int simple_glob(const char *pat, const char *str) {
    if (!pat) return 1;
    while (*pat) {
        if (*pat == '*') {
            while (*(pat + 1) == '*') pat++;
            if (!*(pat + 1)) return 1;
            while (*str) {
                if (simple_glob(pat + 1, str)) return 1;
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

static void walk_recursive(const char *dir_path, const char *rel_base,
                           gitignore_t *parent_gi, thread_pool_t *pool,
                           const walker_options_t *opt) {
    DIR *d = opendir(dir_path);
    if (!d) return;

    gitignore_t local_gi;
    if (parent_gi) {
        local_gi = *parent_gi;
        // Deep copy rules if needed, but since parent_gi rules are static pointers we can borrow
    } else {
        gitignore_init(&local_gi);
    }

    if (!opt->no_ignore) {
        gitignore_load_file(&local_gi, dir_path);
    }

    struct dirent *entry;
    char full_path[4096];
    char child_rel[4096];

    while ((entry = readdir(d)) != NULL) {
        const char *name = entry->d_name;

        // Skip '.' and '..'
        if (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {
            continue;
        }

        int is_dir = (entry->d_type == DT_DIR);
        int is_reg = (entry->d_type == DT_REG);

        // Fallback for filesystems that do not provide DT_*
        if (entry->d_type == DT_UNKNOWN) {
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, name);
            struct stat st;
            if (lstat(full_path, &st) == 0) {
                is_dir = S_ISDIR(st.st_mode);
                is_reg = S_ISREG(st.st_mode);
            }
        }

        // Relative path for gitignore
        if (rel_base && rel_base[0]) {
            snprintf(child_rel, sizeof(child_rel), "%s/%s", rel_base, name);
        } else {
            snprintf(child_rel, sizeof(child_rel), "%s", name);
        }

        // Check gitignore pruning
        if (!opt->no_ignore && gitignore_is_ignored(&local_gi, child_rel, name, is_dir)) {
            continue;
        }

        // Check explicit --exclude-dir
        if (is_dir && opt->exclude_dir && strcmp(name, opt->exclude_dir) == 0) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, name);

        if (is_dir) {
            walk_recursive(full_path, child_rel, &local_gi, pool, opt);
        } else if (is_reg) {
            // Apply --include and --exclude globs
            if (opt->include_glob && !simple_glob(opt->include_glob, name)) {
                continue;
            }
            if (opt->exclude_glob && simple_glob(opt->exclude_glob, name)) {
                continue;
            }

            if (opt->files_only) {
                printf("%s\n", full_path);
            } else if (pool) {
                thread_pool_submit(pool, full_path);
            }
        }
    }

    closedir(d);
}

void walker_walk(const char *root_dir, thread_pool_t *pool, const walker_options_t *opt) {
    if (!root_dir) return;

    struct stat st;
    if (lstat(root_dir, &st) != 0) return;

    if (S_ISREG(st.st_mode)) {
        if (opt->files_only) {
            printf("%s\n", root_dir);
        } else if (pool) {
            thread_pool_submit(pool, root_dir);
        }
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        gitignore_t base_gi;
        gitignore_init(&base_gi);
        walk_recursive(root_dir, "", &base_gi, pool, opt);
        gitignore_free(&base_gi);
    }
}
