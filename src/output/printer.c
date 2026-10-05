/*
 * Author: Wirote Chukeaw (chin6700x)
 * Copyright: (c) 2026 Wirote Chukeaw. All rights reserved.
 * License: MIT License
 */

#include "printer.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define COLOR_MAGENTA "\033[35m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_RED_BOLD "\033[1;31m"
#define COLOR_RESET   "\033[0m"

static void buffer_write(printer_t *p, const char *data, size_t len) {
    if (p->pos + len >= sizeof(p->buffer)) {
        write(STDOUT_FILENO, p->buffer, p->pos);
        p->pos = 0;
    }
    if (len > sizeof(p->buffer)) {
        write(STDOUT_FILENO, data, len);
    } else {
        memcpy(p->buffer + p->pos, data, len);
        p->pos += len;
    }
}

static void buffer_puts(printer_t *p, const char *str) {
    buffer_write(p, str, strlen(str));
}

void printer_init(printer_t *p, const print_options_t *opt) {
    p->pos = 0;
    p->total_matches = 0;
    if (opt) p->opt = *opt;
    else memset(&p->opt, 0, sizeof(p->opt));
    pthread_mutex_init(&p->lock, NULL);
}

void printer_match_line(printer_t *p, const char *file, size_t line_num,
                        const uint8_t *line_start, size_t line_len,
                        const uint8_t *match_start, size_t match_len) {
    pthread_mutex_lock(&p->lock);
    p->total_matches++;

    char num_buf[32];

    // 1. File prefix
    if (!p->opt.no_filename && file) {
        if (p->opt.color) buffer_puts(p, COLOR_MAGENTA);
        buffer_puts(p, file);
        if (p->opt.color) buffer_puts(p, COLOR_RESET);

        if (p->opt.color) buffer_puts(p, COLOR_CYAN);
        buffer_puts(p, ":");
        if (p->opt.color) buffer_puts(p, COLOR_RESET);
    }

    // 2. Line number prefix
    if (p->opt.line_numbers && line_num > 0) {
        int n = snprintf(num_buf, sizeof(num_buf), "%zu", line_num);
        if (p->opt.color) buffer_puts(p, COLOR_GREEN);
        buffer_write(p, num_buf, n);
        if (p->opt.color) buffer_puts(p, COLOR_RESET);

        if (p->opt.color) buffer_puts(p, COLOR_CYAN);
        buffer_puts(p, ":");
        if (p->opt.color) buffer_puts(p, COLOR_RESET);
    }

    // 3. Highlighted Line Content
    if (p->opt.color && match_start && match_len > 0 &&
        match_start >= line_start && match_start + match_len <= line_start + line_len) {
        // Text before match
        size_t before_len = match_start - line_start;
        buffer_write(p, (const char *)line_start, before_len);

        // Highlighted match
        buffer_puts(p, COLOR_RED_BOLD);
        buffer_write(p, (const char *)match_start, match_len);
        buffer_puts(p, COLOR_RESET);

        // Text after match
        size_t after_len = (line_start + line_len) - (match_start + match_len);
        buffer_write(p, (const char *)(match_start + match_len), after_len);
    } else {
        buffer_write(p, (const char *)line_start, line_len);
    }

    buffer_puts(p, "\n");
    pthread_mutex_unlock(&p->lock);
}

void printer_file_match(printer_t *p, const char *file) {
    if (!file) return;
    pthread_mutex_lock(&p->lock);
    p->total_matches++;
    if (p->opt.color) buffer_puts(p, COLOR_MAGENTA);
    buffer_puts(p, file);
    if (p->opt.color) buffer_puts(p, COLOR_RESET);
    buffer_puts(p, "\n");
    pthread_mutex_unlock(&p->lock);
}

void printer_file_count(printer_t *p, const char *file, size_t count) {
    pthread_mutex_lock(&p->lock);
    p->total_matches += count;
    char num_buf[32];
    int n = snprintf(num_buf, sizeof(num_buf), "%zu\n", count);

    if (!p->opt.no_filename && file) {
        if (p->opt.color) buffer_puts(p, COLOR_MAGENTA);
        buffer_puts(p, file);
        if (p->opt.color) buffer_puts(p, COLOR_CYAN);
        buffer_puts(p, ":");
        if (p->opt.color) buffer_puts(p, COLOR_RESET);
    }
    buffer_write(p, num_buf, n);
    pthread_mutex_unlock(&p->lock);
}

void printer_flush(printer_t *p) {
    pthread_mutex_lock(&p->lock);
    if (p->pos > 0) {
        write(STDOUT_FILENO, p->buffer, p->pos);
        p->pos = 0;
    }
    pthread_mutex_unlock(&p->lock);
}

void printer_free(printer_t *p) {
    printer_flush(p);
    pthread_mutex_destroy(&p->lock);
}
