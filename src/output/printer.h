#ifndef ASMGREP_PRINTER_H
#define ASMGREP_PRINTER_H

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

typedef struct {
    int color;
    int line_numbers;
    int count_only;
    int files_with_matches;
    int no_filename;
    int context_before;
    int context_after;
} print_options_t;

typedef struct {
    char buffer[65536];
    size_t pos;
    pthread_mutex_t lock;
    print_options_t opt;
    size_t total_matches;
} printer_t;

void printer_init(printer_t *p, const print_options_t *opt);
void printer_match_line(printer_t *p, const char *file, size_t line_num,
                        const uint8_t *line_start, size_t line_len,
                        const uint8_t *match_start, size_t match_len);
void printer_file_match(printer_t *p, const char *file);
void printer_file_count(printer_t *p, const char *file, size_t count);
void printer_flush(printer_t *p);
void printer_free(printer_t *p);

#endif // ASMGREP_PRINTER_H
