/*
 * Author: Wirote Chukeaw (chin6700x)
 * Copyright: (c) 2026 Wirote Chukeaw. All rights reserved.
 * License: MIT License
 */

#ifndef CHINGREP_SIMD_ARM64_H
#define CHINGREP_SIMD_ARM64_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Fast 512-byte null-byte probe
int arm64_is_binary(const uint8_t *buf, size_t len);

// 64-byte unrolled SIMD newline counter
size_t arm64_count_newlines(const uint8_t *buf, size_t len);

// 64-byte unrolled single-byte search
const uint8_t *arm64_search_memchr(const uint8_t *haystack, size_t len, uint8_t needle);

// Two-vector first-and-last byte SIMD literal search
const uint8_t *arm64_search_literal(const uint8_t *haystack, size_t hlen, const uint8_t *needle, size_t nlen);

// Case-insensitive literal search (needle must be pre-lowercased)
const uint8_t *arm64_search_literal_icase(const uint8_t *haystack, size_t hlen, const uint8_t *needle_lower, size_t nlen);

#ifdef __cplusplus
}
#endif

#endif // ASMGREP_SIMD_ARM64_H
