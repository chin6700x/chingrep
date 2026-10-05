/*
 * Author: Wirote Chukeaw (chin6700x)
 * Copyright: (c) 2026 Wirote Chukeaw. All rights reserved.
 * License: MIT License
 */

#include "mmap_reader.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdlib.h>

int mmap_file_open(const char *filepath, mmap_file_t *out_file) {
    if (!filepath || !out_file) return -1;

    out_file->data = NULL;
    out_file->size = 0;
    out_file->fd = -1;
    out_file->is_mmap = 0;

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) return -1;

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return -1;
    }

    // Skip special files, directories, empty files
    if (!S_ISREG(st.st_mode) || st.st_size <= 0) {
        close(fd);
        return -1;
    }

    size_t size = (size_t)st.st_size;
    void *addr = mmap(NULL, size, PROT_READ, MAP_SHARED | MAP_FILE, fd, 0);

    if (addr == MAP_FAILED) {
        // Fallback for files that cannot be mmapped
        close(fd);
        return -1;
    }

    // Aggressive kernel prefetch for sequential streaming
#ifdef MADV_SEQUENTIAL
    madvise(addr, size, MADV_SEQUENTIAL);
#endif
#ifdef MADV_WILLNEED
    madvise(addr, size, MADV_WILLNEED);
#endif

    out_file->data = (const uint8_t *)addr;
    out_file->size = size;
    out_file->fd = fd;
    out_file->is_mmap = 1;

    return 0;
}

void mmap_file_close(mmap_file_t *file) {
    if (!file) return;

    if (file->is_mmap && file->data && file->size > 0) {
        munmap((void *)file->data, file->size);
    }

    if (file->fd >= 0) {
        close(file->fd);
        file->fd = -1;
    }

    file->data = NULL;
    file->size = 0;
    file->is_mmap = 0;
}
