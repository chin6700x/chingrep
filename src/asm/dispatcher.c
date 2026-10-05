/*
 * Author: Wirote Chukeaw (chin6700x)
 * Copyright: (c) 2026 Wirote Chukeaw. All rights reserved.
 * License: MIT License
 */

#include "simd_engine.h"
#include "simd_arm64.h"

// Prototypes for portable engine
int portable_is_binary(const uint8_t *buf, size_t len);
size_t portable_count_newlines(const uint8_t *buf, size_t len);
const uint8_t *portable_search_memchr(const uint8_t *haystack, size_t len, uint8_t needle);
const uint8_t *portable_search_literal(const uint8_t *haystack, size_t hlen, const uint8_t *needle, size_t nlen);
const uint8_t *portable_search_literal_icase(const uint8_t *haystack, size_t hlen, const uint8_t *needle_lower, size_t nlen);

// Prototypes for x86 AVX2 engine (if compiled)
#if (defined(__x86_64__) || defined(_M_X64)) && defined(__AVX2__)
int x86_avx2_is_binary(const uint8_t *buf, size_t len);
size_t x86_avx2_count_newlines(const uint8_t *buf, size_t len);
const uint8_t *x86_avx2_search_memchr(const uint8_t *haystack, size_t len, uint8_t needle);
const uint8_t *x86_avx2_search_literal(const uint8_t *haystack, size_t hlen, const uint8_t *needle, size_t nlen);
const uint8_t *x86_avx2_search_literal_icase(const uint8_t *haystack, size_t hlen, const uint8_t *needle_lower, size_t nlen);
#endif

const char *simd_get_engine_name(void) {
#if defined(__aarch64__) || defined(__arm64__)
    return "ARM64 NEON (Apple Silicon Hand-Tuned Assembly)";
#elif (defined(__x86_64__) || defined(_M_X64)) && defined(__AVX2__)
    return "x86_64 AVX2 256-bit Vector SIMD";
#else
    return "Universal 64-bit SWAR Engine";
#endif
}

int simd_is_binary(const uint8_t *buf, size_t len) {
#if defined(__aarch64__) || defined(__arm64__)
    return arm64_is_binary(buf, len);
#elif (defined(__x86_64__) || defined(_M_X64)) && defined(__AVX2__)
    return x86_avx2_is_binary(buf, len);
#else
    return portable_is_binary(buf, len);
#endif
}

size_t simd_count_newlines(const uint8_t *buf, size_t len) {
#if defined(__aarch64__) || defined(__arm64__)
    return arm64_count_newlines(buf, len);
#elif (defined(__x86_64__) || defined(_M_X64)) && defined(__AVX2__)
    return x86_avx2_count_newlines(buf, len);
#else
    return portable_count_newlines(buf, len);
#endif
}

const uint8_t *simd_search_memchr(const uint8_t *haystack, size_t len, uint8_t needle) {
#if defined(__aarch64__) || defined(__arm64__)
    return arm64_search_memchr(haystack, len, needle);
#elif (defined(__x86_64__) || defined(_M_X64)) && defined(__AVX2__)
    return x86_avx2_search_memchr(haystack, len, needle);
#else
    return portable_search_memchr(haystack, len, needle);
#endif
}

const uint8_t *simd_search_literal(const uint8_t *haystack, size_t hlen, const uint8_t *needle, size_t nlen) {
#if defined(__aarch64__) || defined(__arm64__)
    return arm64_search_literal(haystack, hlen, needle, nlen);
#elif (defined(__x86_64__) || defined(_M_X64)) && defined(__AVX2__)
    return x86_avx2_search_literal(haystack, hlen, needle, nlen);
#else
    return portable_search_literal(haystack, hlen, needle, nlen);
#endif
}

const uint8_t *simd_search_literal_icase(const uint8_t *haystack, size_t hlen, const uint8_t *needle_lower, size_t nlen) {
#if defined(__aarch64__) || defined(__arm64__)
    return arm64_search_literal_icase(haystack, hlen, needle_lower, nlen);
#elif (defined(__x86_64__) || defined(_M_X64)) && defined(__AVX2__)
    return x86_avx2_search_literal_icase(haystack, hlen, needle_lower, nlen);
#else
    return portable_search_literal_icase(haystack, hlen, needle_lower, nlen);
#endif
}
