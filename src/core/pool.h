/*
 * Author: Wirote Chukeaw (chin6700x)
 * Copyright: (c) 2026 Wirote Chukeaw. All rights reserved.
 * License: MIT License
 */

#ifndef CHINGREP_POOL_H
#define CHINGREP_POOL_H

#include <stddef.h>
#include <stdint.h>
#include "mmap_reader.h"
#include "../output/printer.h"

typedef struct {
    const char *pattern;
    size_t pattern_len;
    char *pattern_lower;
    int ignore_case;
    int word_regexp;
    int line_numbers;
    int count_only;
    int files_with_matches;
    int max_count;
} search_options_t;

typedef struct thread_pool thread_pool_t;

// Creates worker thread pool targeting Apple Silicon Performance Cores
thread_pool_t *thread_pool_create(int num_threads, printer_t *printer, const search_options_t *opt);

// Submits a file path for asynchronous searching
void thread_pool_submit(thread_pool_t *pool, const char *filepath);

// Waits for all queued files to be processed and terminates threads
void thread_pool_wait_and_destroy(thread_pool_t *pool);

// Single-file synchronous search (for STDIN or single file targets)
void search_single_file(const char *filepath, const uint8_t *data, size_t size,
                        printer_t *printer, const search_options_t *opt);

#endif // ASMGREP_POOL_H
