/*
 * Author: Wirote Chukeaw (chin6700x)
 * Copyright: (c) 2026 Wirote Chukeaw. All rights reserved.
 * License: MIT License
 */

#include "simd_engine.h"
#include <string.h>

// SWAR (SIMD Within A Register) zero byte detector
static inline uint64_t has_zero_byte(uint64_t v) {
    return (v - 0x0101010101010101ULL) & ~v & 0x8080808080808080ULL;
}

int portable_is_binary(const uint8_t *buf, size_t len) {
    if (!buf || len == 0) return 0;
    size_t probe_len = len > 512 ? 512 : len;
    const uint8_t *p = buf;
    const uint8_t *end = buf + probe_len;

    // Check 8 bytes at a time using 64-bit SWAR
    while (p + 8 <= end) {
        uint64_t v;
        memcpy(&v, p, 8);
        if (has_zero_byte(v)) return 1;
        p += 8;
    }

    while (p < end) {
        if (*p == 0) return 1;
        p++;
    }
    return 0;
}

size_t portable_count_newlines(const uint8_t *buf, size_t len) {
    if (!buf || len == 0) return 0;
    size_t count = 0;
    const uint8_t *p = buf;
    const uint8_t *end = buf + len;

    // 8-byte SWAR newline counter
    uint64_t nl_mask = 0x0A0A0A0A0A0A0A0AULL;
    while (p + 8 <= end) {
        uint64_t v;
        memcpy(&v, p, 8);
        uint64_t diff = v ^ nl_mask;
        uint64_t matches = has_zero_byte(diff);
        if (matches) {
            // Count matched bytes
            for (int i = 0; i < 8; i++) {
                if (p[i] == '\n') count++;
            }
        }
        p += 8;
    }

    while (p < end) {
        if (*p == '\n') count++;
        p++;
    }
    return count;
}

const uint8_t *portable_search_memchr(const uint8_t *haystack, size_t len, uint8_t needle) {
    if (!haystack || len == 0) return NULL;
    return (const uint8_t *)memchr(haystack, needle, len);
}

const uint8_t *portable_search_literal(const uint8_t *haystack, size_t hlen, const uint8_t *needle, size_t nlen) {
    if (!haystack || !needle || nlen == 0 || nlen > hlen) return NULL;
    if (nlen == 1) return portable_search_memchr(haystack, hlen, needle[0]);

    uint8_t first_b = needle[0];
    uint8_t last_b  = needle[nlen - 1];
    size_t last_off = nlen - 1;

    const uint8_t *cur = haystack;
    const uint8_t *max_start = haystack + (hlen - nlen);

    while (cur <= max_start) {
        // Fast skip using memchr
        const uint8_t *cand = (const uint8_t *)memchr(cur, first_b, (max_start - cur) + 1);
        if (!cand) return NULL;

        if (cand[last_off] == last_b) {
            if (nlen <= 2 || memcmp(cand + 1, needle + 1, nlen - 2) == 0) {
                return cand;
            }
        }
        cur = cand + 1;
    }
    return NULL;
}

static inline uint8_t to_lower_ascii(uint8_t c) {
    return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}

const uint8_t *portable_search_literal_icase(const uint8_t *haystack, size_t hlen, const uint8_t *needle_lower, size_t nlen) {
    if (!haystack || !needle_lower || nlen == 0 || nlen > hlen) return NULL;

    uint8_t first_l = needle_lower[0];
    uint8_t last_l  = needle_lower[nlen - 1];
    size_t last_off = nlen - 1;

    const uint8_t *cur = haystack;
    const uint8_t *max_start = haystack + (hlen - nlen);

    while (cur <= max_start) {
        uint8_t c0 = to_lower_ascii(*cur);
        uint8_t cl = to_lower_ascii(cur[last_off]);
        if (c0 == first_l && cl == last_l) {
            size_t i = 1;
            int matched = 1;
            for (; i < last_off; i++) {
                if (to_lower_ascii(cur[i]) != needle_lower[i]) {
                    matched = 0;
                    break;
                }
            }
            if (matched) return cur;
        }
        cur++;
    }
    return NULL;
}
