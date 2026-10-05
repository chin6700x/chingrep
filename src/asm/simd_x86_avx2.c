/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw. All rights reserved.
 * License: MIT License
 */

#include "simd_engine.h"
#include <string.h>

#if (defined(__x86_64__) || defined(_M_X64)) && defined(__AVX2__)
#include <immintrin.h>

int x86_avx2_is_binary(const uint8_t *buf, size_t len) {
    if (!buf || len == 0) return 0;
    size_t probe_len = len > 512 ? 512 : len;
    const uint8_t *p = buf;
    const uint8_t *end = buf + probe_len;

    __m256i zero = _mm256_setzero_si256();

    while (p + 32 <= end) {
        __m256i v = _mm256_loadu_si256((const __m256i *)p);
        __m256i eq = _mm256_cmpeq_epi8(v, zero);
        int mask = _mm256_movemask_epi8(eq);
        if (mask != 0) return 1;
        p += 32;
    }

    while (p < end) {
        if (*p == 0) return 1;
        p++;
    }
    return 0;
}

size_t x86_avx2_count_newlines(const uint8_t *buf, size_t len) {
    if (!buf || len == 0) return 0;
    size_t count = 0;
    const uint8_t *p = buf;
    const uint8_t *end = buf + len;

    __m256i v_nl = _mm256_set1_epi8('\n');

    // 64-byte unrolled loop
    while (p + 64 <= end) {
        __m256i v0 = _mm256_loadu_si256((const __m256i *)p);
        __m256i v1 = _mm256_loadu_si256((const __m256i *)(p + 32));

        __m256i eq0 = _mm256_cmpeq_epi8(v0, v_nl);
        __m256i eq1 = _mm256_cmpeq_epi8(v1, v_nl);

        uint32_t mask0 = (uint32_t)_mm256_movemask_epi8(eq0);
        uint32_t mask1 = (uint32_t)_mm256_movemask_epi8(eq1);

        count += (size_t)__builtin_popcount(mask0) + (size_t)__builtin_popcount(mask1);
        p += 64;
    }

    while (p + 32 <= end) {
        __m256i v0 = _mm256_loadu_si256((const __m256i *)p);
        __m256i eq0 = _mm256_cmpeq_epi8(v0, v_nl);
        uint32_t mask0 = (uint32_t)_mm256_movemask_epi8(eq0);
        count += (size_t)__builtin_popcount(mask0);
        p += 32;
    }

    while (p < end) {
        if (*p == '\n') count++;
        p++;
    }

    return count;
}

const uint8_t *x86_avx2_search_memchr(const uint8_t *haystack, size_t len, uint8_t needle) {
    if (!haystack || len == 0) return NULL;
    const uint8_t *p = haystack;
    const uint8_t *end = haystack + len;

    __m256i v_needle = _mm256_set1_epi8((char)needle);

    while (p + 32 <= end) {
        __m256i v = _mm256_loadu_si256((const __m256i *)p);
        __m256i eq = _mm256_cmpeq_epi8(v, v_needle);
        int mask = _mm256_movemask_epi8(eq);
        if (mask != 0) {
            int idx = __builtin_ctz(mask);
            return p + idx;
        }
        p += 32;
    }

    while (p < end) {
        if (*p == needle) return p;
        p++;
    }
    return NULL;
}

const uint8_t *x86_avx2_search_literal(const uint8_t *haystack, size_t hlen, const uint8_t *needle, size_t nlen) {
    if (!haystack || !needle || nlen == 0 || nlen > hlen) return NULL;
    if (nlen == 1) return x86_avx2_search_memchr(haystack, hlen, needle[0]);

    uint8_t first_b = needle[0];
    uint8_t last_b  = needle[nlen - 1];
    size_t last_off = nlen - 1;

    const uint8_t *cur = haystack;
    const uint8_t *max_start = haystack + (hlen - nlen);

    __m256i v_first = _mm256_set1_epi8((char)first_b);
    __m256i v_last  = _mm256_set1_epi8((char)last_b);

    while (cur + 32 <= max_start) {
        __m256i chunk_first = _mm256_loadu_si256((const __m256i *)cur);
        __m256i chunk_last  = _mm256_loadu_si256((const __m256i *)(cur + last_off));

        __m256i eq_first = _mm256_cmpeq_epi8(chunk_first, v_first);
        __m256i eq_last  = _mm256_cmpeq_epi8(chunk_last, v_last);
        __m256i candidate = _mm256_and_si256(eq_first, eq_last);

        int mask = _mm256_movemask_epi8(candidate);
        while (mask != 0) {
            int idx = __builtin_ctz(mask);
            const uint8_t *match = cur + idx;
            if (nlen <= 2 || memcmp(match + 1, needle + 1, nlen - 2) == 0) {
                return match;
            }
            mask &= (mask - 1); // clear least significant bit
        }
        cur += 32;
    }

    // Scalar tail
    while (cur <= max_start) {
        if (*cur == first_b && cur[last_off] == last_b) {
            if (nlen <= 2 || memcmp(cur + 1, needle + 1, nlen - 2) == 0) {
                return cur;
            }
        }
        cur++;
    }

    return NULL;
}

static inline uint8_t to_lower_ascii(uint8_t c) {
    return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}

const uint8_t *x86_avx2_search_literal_icase(const uint8_t *haystack, size_t hlen, const uint8_t *needle_lower, size_t nlen) {
    if (!haystack || !needle_lower || nlen == 0 || nlen > hlen) return NULL;

    uint8_t first_l = needle_lower[0];
    uint8_t first_u = (first_l >= 'a' && first_l <= 'z') ? (first_l - ('a' - 'A')) : first_l;
    uint8_t last_l  = needle_lower[nlen - 1];
    uint8_t last_u  = (last_l >= 'a' && last_l <= 'z') ? (last_l - ('a' - 'A')) : last_l;
    size_t last_off = nlen - 1;

    const uint8_t *cur = haystack;
    const uint8_t *max_start = haystack + (hlen - nlen);

    __m256i v_f_l = _mm256_set1_epi8((char)first_l);
    __m256i v_f_u = _mm256_set1_epi8((char)first_u);
    __m256i v_l_l = _mm256_set1_epi8((char)last_l);
    __m256i v_l_u = _mm256_set1_epi8((char)last_u);

    while (cur + 32 <= max_start) {
        __m256i chunk_first = _mm256_loadu_si256((const __m256i *)cur);
        __m256i chunk_last  = _mm256_loadu_si256((const __m256i *)(cur + last_off));

        __m256i eq_f_l = _mm256_cmpeq_epi8(chunk_first, v_f_l);
        __m256i eq_f_u = _mm256_cmpeq_epi8(chunk_first, v_f_u);
        __m256i eq_first = _mm256_or_si256(eq_f_l, eq_f_u);

        __m256i eq_l_l = _mm256_cmpeq_epi8(chunk_last, v_l_l);
        __m256i eq_l_u = _mm256_cmpeq_epi8(chunk_last, v_l_u);
        __m256i eq_last  = _mm256_or_si256(eq_l_l, eq_l_u);

        __m256i candidate = _mm256_and_si256(eq_first, eq_last);
        int mask = _mm256_movemask_epi8(candidate);

        while (mask != 0) {
            int idx = __builtin_ctz(mask);
            const uint8_t *match = cur + idx;
            size_t i = 1;
            int matched = 1;
            for (; i < last_off; i++) {
                if (to_lower_ascii(match[i]) != needle_lower[i]) {
                    matched = 0;
                    break;
                }
            }
            if (matched) return match;
            mask &= (mask - 1);
        }
        cur += 32;
    }

    // Scalar tail
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

#endif // x86_64 AVX2
