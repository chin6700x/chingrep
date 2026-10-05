/*
 * Author: Wirote Chukeaw (chin6700x)
 * Copyright: (c) 2026 Wirote Chukeaw. All rights reserved.
 * License: MIT License
 */

#ifndef CHINGREP_MMAP_READER_H
#define CHINGREP_MMAP_READER_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
    const uint8_t *data;
    size_t size;
    int fd;
    int is_mmap;
} mmap_file_t;

// Maps a file into memory with MADV_SEQUENTIAL | MADV_WILLNEED
int mmap_file_open(const char *filepath, mmap_file_t *out_file);

// Closes and unmaps the file
void mmap_file_close(mmap_file_t *file);

#endif // ASMGREP_MMAP_READER_H
