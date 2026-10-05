#ifndef ASMGREP_WALKER_H
#define ASMGREP_WALKER_H

#include "pool.h"
#include "gitignore.h"

typedef struct {
    const char *include_glob;
    const char *exclude_glob;
    const char *exclude_dir;
    int no_ignore;
    int files_only;
} walker_options_t;

// Recursively walks root_dir and feeds candidate files into thread_pool
void walker_walk(const char *root_dir, thread_pool_t *pool, const walker_options_t *opt);

#endif // ASMGREP_WALKER_H
