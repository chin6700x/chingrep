/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw. All rights reserved.
 * License: MIT License
 */

#include "pool.h"
#include "../asm/simd_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/sysctl.h>

#define QUEUE_CAPACITY 65536

struct thread_pool {
    pthread_t *threads;
    int num_threads;

    char *queue[QUEUE_CAPACITY];
    size_t head;
    size_t tail;
    size_t count;

    int stop;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    pthread_cond_t all_idle;
    int active_workers;

    printer_t *printer;
    search_options_t opt;
};

static inline int is_word_char(uint8_t c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '_');
}

void search_single_file(const char *filepath, const uint8_t *data, size_t size,
                        printer_t *printer, const search_options_t *opt) {
    if (!data || size == 0) return;

    // Fast-path: binary probe
    if (simd_is_binary(data, size)) {
        return;
    }

    const uint8_t *needle = (const uint8_t *)opt->pattern;
    const uint8_t *needle_lower = (const uint8_t *)opt->pattern_lower;
    size_t nlen = opt->pattern_len;
    if (nlen == 0 || nlen > size) return;

    const uint8_t *cur = data;
    const uint8_t *end = data + size;
    size_t line_num = 1;
    const uint8_t *prev_line_check = data;
    size_t match_count = 0;

    while (cur < end) {
        size_t rem = end - cur;
        const uint8_t *match = NULL;

        if (opt->ignore_case) {
            match = simd_search_literal_icase(cur, rem, needle_lower, nlen);
        } else {
            match = simd_search_literal(cur, rem, needle, nlen);
        }

        if (!match) break;

        // Word boundary check
        if (opt->word_regexp) {
            if (match > data && is_word_char(*(match - 1))) {
                cur = match + 1;
                continue;
            }
            if (match + nlen < end && is_word_char(*(match + nlen))) {
                cur = match + 1;
                continue;
            }
        }

        // Fast return for -l (files with matches)
        if (opt->files_with_matches) {
            printer_file_match(printer, filepath);
            return;
        }

        // Find line start (scan backward to previous '\n' or data start)
        const uint8_t *line_start = match;
        while (line_start > data && *(line_start - 1) != '\n') {
            line_start--;
        }

        // Find line end (scan forward to next '\n' or buffer end)
        const uint8_t *line_end = match + nlen;
        while (line_end < end && *line_end != '\n') {
            line_end++;
        }
        size_t line_len = line_end - line_start;

        // Compute exact line number via SIMD newline counter
        if (opt->line_numbers || opt->count_only) {
            if (line_start > prev_line_check) {
                line_num += simd_count_newlines(prev_line_check, line_start - prev_line_check);
                prev_line_check = line_start;
            }
        }

        match_count++;

        if (!opt->count_only) {
            printer_match_line(printer, filepath, line_num, line_start, line_len, match, nlen);
        }

        if (opt->max_count > 0 && match_count >= (size_t)opt->max_count) {
            break;
        }

        // Advance to next line to avoid printing duplicate lines
        cur = (line_end < end) ? (line_end + 1) : end;
        prev_line_check = cur;
        line_num++;
    }

    if (opt->count_only && match_count > 0) {
        printer_file_count(printer, filepath, match_count);
    }
}

static void *worker_thread(void *arg) {
    thread_pool_t *pool = (thread_pool_t *)arg;

    while (1) {
        char *filepath = NULL;

        pthread_mutex_lock(&pool->lock);
        while (pool->count == 0 && !pool->stop) {
            pthread_cond_wait(&pool->not_empty, &pool->lock);
        }

        if (pool->count == 0 && pool->stop) {
            pthread_mutex_unlock(&pool->lock);
            break;
        }

        filepath = pool->queue[pool->head];
        pool->head = (pool->head + 1) % QUEUE_CAPACITY;
        pool->count--;
        pool->active_workers++;

        pthread_cond_signal(&pool->not_full);
        pthread_mutex_unlock(&pool->lock);

        // Process file
        if (filepath) {
            mmap_file_t file;
            if (mmap_file_open(filepath, &file) == 0) {
                search_single_file(filepath, file.data, file.size, pool->printer, &pool->opt);
                mmap_file_close(&file);
            }
            free(filepath);
        }

        pthread_mutex_lock(&pool->lock);
        pool->active_workers--;
        if (pool->count == 0 && pool->active_workers == 0) {
            pthread_cond_broadcast(&pool->all_idle);
        }
        pthread_mutex_unlock(&pool->lock);
    }

    return NULL;
}

thread_pool_t *thread_pool_create(int num_threads, printer_t *printer, const search_options_t *opt) {
    if (num_threads <= 0) {
        // Detect Apple Silicon Performance Cores
        int pcores = 0;
        size_t len = sizeof(pcores);
        if (sysctlbyname("hw.perflevel0.physicalcpu", &pcores, &len, NULL, 0) == 0 && pcores > 0) {
            num_threads = pcores;
        } else {
            num_threads = (int)sysconf(_SC_NPROCESSORS_ONLN);
            if (num_threads <= 0) num_threads = 4;
        }
    }

    thread_pool_t *pool = (thread_pool_t *)calloc(1, sizeof(thread_pool_t));
    if (!pool) return NULL;

    pool->num_threads = num_threads;
    pool->printer = printer;
    if (opt) pool->opt = *opt;

    pthread_mutex_init(&pool->lock, NULL);
    pthread_cond_init(&pool->not_empty, NULL);
    pthread_cond_init(&pool->not_full, NULL);
    pthread_cond_init(&pool->all_idle, NULL);

    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&pool->threads[i], NULL, worker_thread, pool);
    }

    return pool;
}

void thread_pool_submit(thread_pool_t *pool, const char *filepath) {
    if (!pool || !filepath) return;

    pthread_mutex_lock(&pool->lock);
    while (pool->count >= QUEUE_CAPACITY && !pool->stop) {
        pthread_cond_wait(&pool->not_full, &pool->lock);
    }

    if (pool->stop) {
        pthread_mutex_unlock(&pool->lock);
        return;
    }

    pool->queue[pool->tail] = strdup(filepath);
    pool->tail = (pool->tail + 1) % QUEUE_CAPACITY;
    pool->count++;

    pthread_cond_signal(&pool->not_empty);
    pthread_mutex_unlock(&pool->lock);
}

void thread_pool_wait_and_destroy(thread_pool_t *pool) {
    if (!pool) return;

    pthread_mutex_lock(&pool->lock);
    while (pool->count > 0 || pool->active_workers > 0) {
        pthread_cond_wait(&pool->all_idle, &pool->lock);
    }
    pool->stop = 1;
    pthread_cond_broadcast(&pool->not_empty);
    pthread_mutex_unlock(&pool->lock);

    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    free(pool->threads);
    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->not_empty);
    pthread_cond_destroy(&pool->not_full);
    pthread_cond_destroy(&pool->all_idle);
    free(pool);
}
