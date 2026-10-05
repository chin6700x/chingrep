/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw. All rights reserved.
 * License: MIT License
 */

#ifndef CHINGREP_SIMD_ENGINE_H
#define CHINGREP_SIMD_ENGINE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Probe up to 512 bytes for null-bytes (indicating binary file)
int simd_is_binary(const uint8_t *buf, size_t len);

// Count newlines (\n) using vectorized hardware counters (NEON / AVX2 / SWAR)
size_t simd_count_newlines(const uint8_t *buf, size_t len);

// Search single byte
const uint8_t *simd_search_memchr(const uint8_t *haystack, size_t len, uint8_t needle);

// Exact substring literal search
const uint8_t *simd_search_literal(const uint8_t *haystack, size_t hlen, const uint8_t *needle, size_t nlen);

// Case-insensitive substring search (needle_lower must be ASCII lowercased)
const uint8_t *simd_search_literal_icase(const uint8_t *haystack, size_t hlen, const uint8_t *needle_lower, size_t nlen);

// Return name of the active SIMD engine (e.g. "ARM64 NEON (Assembly)", "x86_64 AVX2", "Portable 64-bit SWAR")
const char *simd_get_engine_name(void);

#ifdef __cplusplus
}
#endif

#endif // CHINGREP_SIMD_ENGINE_H
