/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw. All rights reserved.
 * License: MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>

#include "asm/simd_engine.h"
#include "core/mmap_reader.h"
#include "core/gitignore.h"
#include "core/pool.h"
#include "core/walker.h"
#include "output/printer.h"

#define CHINGREP_VERSION "1.0.0"

static void print_usage(void) {
    printf("chingrep v%s (Ultra-Fast Hardware-Accelerated Grep)\n", CHINGREP_VERSION);
    printf("Engine: %s\n", simd_get_engine_name());
    printf("Author: Wirot Chookeaw (chin6700x)\n\n");
    printf("Usage:\n");
    printf("  chingrep [OPTIONS] <PATTERN> [PATH ...]\n");
    printf("  cg       [OPTIONS] <PATTERN> [PATH ...] (short alias)\n\n");
    printf("Options:\n");
    printf("  -i, --ignore-case         Case-insensitive search (ASCII)\n");
    printf("  -n, --line-number         Prefix each output line with 1-based line number\n");
    printf("  -c, --count               Print only a count of matching lines per file\n");
    printf("  -w, --word-regexp         Match only whole words\n");
    printf("  -r, -R, --recursive       Recursively search directories\n");
    printf("  -l, --files-with-matches  Print only names of files containing matches\n");
    printf("  -F, --fixed-strings       Interpret PATTERN as literal string (default)\n");
    printf("  -m, --max-count <NUM>     Stop searching after NUM matching lines per file\n");
    printf("  -t, --threads <NUM>       Number of worker threads (default: hardware P-cores)\n");
    printf("      --include <GLOB>      Search only files matching GLOB (e.g. \"*.js\")\n");
    printf("      --exclude <GLOB>      Skip files matching GLOB\n");
    printf("      --exclude-dir <DIR>   Skip directories matching DIR\n");
    printf("      --no-ignore           Do not respect .gitignore files\n");
    printf("      --files               List candidate files without searching content\n");
    printf("      --color[=WHEN]        Colorize output: always, never, auto (default: auto)\n");
    printf("  -h, --help                Show this help screen\n");
    printf("  -V, --version             Show version and hardware engine information\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    search_options_t s_opt;
    memset(&s_opt, 0, sizeof(s_opt));
    s_opt.line_numbers = 1; // Default to line numbers like ripgrep/nodegrep in terminal

    print_options_t p_opt;
    memset(&p_opt, 0, sizeof(p_opt));
    p_opt.line_numbers = 1;
    p_opt.color = isatty(STDOUT_FILENO);

    walker_options_t w_opt;
    memset(&w_opt, 0, sizeof(w_opt));

    int recursive = 0;
    (void)recursive;
    int num_threads = 0;
    const char *pattern = NULL;
    char *paths[256];
    int path_count = 0;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            print_usage();
            return 0;
        } else if (strcmp(arg, "-V") == 0 || strcmp(arg, "--version") == 0) {
            printf("chingrep %s (%s)\n", CHINGREP_VERSION, simd_get_engine_name());
            return 0;
        } else if (arg[0] == '-' && arg[1] != '-' && arg[1] != '\0') {
            // Generic clustered short flags: -rn, -rni, -wi, -cw, -m 5, -m5, etc.
            int handled = 1;
            for (size_t j = 1; arg[j] != '\0'; j++) {
                char c = arg[j];
                if (c == 'i') {
                    s_opt.ignore_case = 1;
                } else if (c == 'n') {
                    s_opt.line_numbers = 1;
                    p_opt.line_numbers = 1;
                } else if (c == 'c') {
                    s_opt.count_only = 1;
                    p_opt.count_only = 1;
                } else if (c == 'w') {
                    s_opt.word_regexp = 1;
                } else if (c == 'r' || c == 'R') {
                    recursive = 1;
                } else if (c == 'l') {
                    s_opt.files_with_matches = 1;
                    p_opt.files_with_matches = 1;
                } else if (c == 'F') {
                    // literal strings
                } else if (c == 'm') {
                    if (arg[j + 1] != '\0') {
                        s_opt.max_count = atoi(&arg[j + 1]);
                    } else if (i + 1 < argc) {
                        s_opt.max_count = atoi(argv[++i]);
                    }
                    break;
                } else if (c == 't') {
                    if (arg[j + 1] != '\0') {
                        num_threads = atoi(&arg[j + 1]);
                    } else if (i + 1 < argc) {
                        num_threads = atoi(argv[++i]);
                    }
                    break;
                } else {
                    handled = 0;
                    break;
                }
            }
            if (handled) continue;
        } else if (strcmp(arg, "--ignore-case") == 0) {
            s_opt.ignore_case = 1;
        } else if (strcmp(arg, "--line-number") == 0) {
            s_opt.line_numbers = 1;
            p_opt.line_numbers = 1;
        } else if (strcmp(arg, "--no-line-number") == 0) {
            s_opt.line_numbers = 0;
            p_opt.line_numbers = 0;
        } else if (strcmp(arg, "--count") == 0) {
            s_opt.count_only = 1;
            p_opt.count_only = 1;
        } else if (strcmp(arg, "--word-regexp") == 0) {
            s_opt.word_regexp = 1;
        } else if (strcmp(arg, "--recursive") == 0) {
            recursive = 1;
        } else if (strcmp(arg, "--files-with-matches") == 0) {
            s_opt.files_with_matches = 1;
            p_opt.files_with_matches = 1;
        } else if (strcmp(arg, "--fixed-strings") == 0) {
            // asmgrep default is literal fixed strings
        } else if (strncmp(arg, "--max-count=", 12) == 0) {
            s_opt.max_count = atoi(arg + 12);
        } else if (strcmp(arg, "--threads") == 0 && i + 1 < argc) {
            num_threads = atoi(argv[++i]);
        } else if (strncmp(arg, "--include=", 10) == 0) {
            w_opt.include_glob = arg + 10;
        } else if (strcmp(arg, "--include") == 0 && i + 1 < argc) {
            w_opt.include_glob = argv[++i];
        } else if (strncmp(arg, "--exclude=", 10) == 0) {
            w_opt.exclude_glob = arg + 10;
        } else if (strcmp(arg, "--exclude") == 0 && i + 1 < argc) {
            w_opt.exclude_glob = argv[++i];
        } else if (strncmp(arg, "--exclude-dir=", 14) == 0) {
            w_opt.exclude_dir = arg + 14;
        } else if (strcmp(arg, "--exclude-dir") == 0 && i + 1 < argc) {
            w_opt.exclude_dir = argv[++i];
        } else if (strcmp(arg, "--no-ignore") == 0) {
            w_opt.no_ignore = 1;
        } else if (strcmp(arg, "--files") == 0) {
            w_opt.files_only = 1;
        } else if (strcmp(arg, "--color") == 0 || strcmp(arg, "--color=always") == 0) {
            p_opt.color = 1;
        } else if (strcmp(arg, "--color=never") == 0) {
            p_opt.color = 0;
        } else if (strcmp(arg, "--color=auto") == 0) {
            p_opt.color = isatty(STDOUT_FILENO);
        } else if (arg[0] == '-') {
            // Unknown flag, skip gracefully
        } else {
            if (!pattern && !w_opt.files_only) {
                pattern = arg;
            } else if (path_count < 256) {
                paths[path_count++] = (char *)arg;
            }
        }
    }

    if (!pattern && !w_opt.files_only) {
        fprintf(stderr, "Error: No search pattern specified.\n");
        return 1;
    }

    if (w_opt.files_only) {
        const char *target = (path_count > 0) ? paths[0] : (pattern ? pattern : ".");
        walker_walk(target, NULL, &w_opt);
        return 0;
    }

    s_opt.pattern = pattern;
    s_opt.pattern_len = strlen(pattern);

    // Pre-lowercase pattern for case-insensitive search
    s_opt.pattern_lower = (char *)malloc(s_opt.pattern_len + 1);
    for (size_t k = 0; k < s_opt.pattern_len; k++) {
        s_opt.pattern_lower[k] = (char)tolower((unsigned char)pattern[k]);
    }
    s_opt.pattern_lower[s_opt.pattern_len] = '\0';

    printer_t printer;
    printer_init(&printer, &p_opt);

    // If no paths specified, check if STDIN or current directory
    if (path_count == 0) {
        if (!isatty(STDIN_FILENO)) {
            // Read from STDIN
            size_t cap = 1048576; // 1 MB initial
            size_t sz = 0;
            uint8_t *buf = (uint8_t *)malloc(cap);
            ssize_t bytes;
            while ((bytes = read(STDIN_FILENO, buf + sz, cap - sz)) > 0) {
                sz += bytes;
                if (sz + 65536 > cap) {
                    cap *= 2;
                    buf = (uint8_t *)realloc(buf, cap);
                }
            }
            p_opt.no_filename = 1;
            printer_init(&printer, &p_opt);
            search_single_file(NULL, buf, sz, &printer, &s_opt);
            free(buf);
            printer_flush(&printer);
            printer_free(&printer);
            free(s_opt.pattern_lower);
            return (printer.total_matches > 0) ? 0 : 1;
        } else {
            paths[path_count++] = ".";
            recursive = 1;
        }
    }

    thread_pool_t *pool = thread_pool_create(num_threads, &printer, &s_opt);

    for (int p = 0; p < path_count; p++) {
        const char *target = paths[p];
        struct stat st;
        if (lstat(target, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            walker_walk(target, pool, &w_opt);
        } else {
            thread_pool_submit(pool, target);
        }
    }

    thread_pool_wait_and_destroy(pool);
    printer_flush(&printer);
    int exit_code = (printer.total_matches > 0) ? 0 : 1;
    printer_free(&printer);
    free(s_opt.pattern_lower);

    return exit_code;
}
