// ==============================================================================
// chingrep: Ultra-Fast ARM64 NEON Substring & Character Search Kernels
// Author: Wirot Chookeaw (chin6700x)
// Copyright: (c) 2026 Wirot Chookeaw. All rights reserved.
// License: MIT License
//
// Description: Hand-crafted 64-byte unrolled ARM64 NEON vector search.
//              Optimized for Apple Silicon 8-wide decode and 4 NEON pipes.
//
// Functions:
//   const uint8_t *arm64_search_memchr(const uint8_t *haystack, size_t len, uint8_t needle);
//   const uint8_t *arm64_search_literal(const uint8_t *haystack, size_t hlen, const uint8_t *needle, size_t nlen);
//   const uint8_t *arm64_search_literal_icase(const uint8_t *haystack, size_t hlen, const uint8_t *needle_lower, size_t nlen);
// ==============================================================================

.text
.p2align 4
.globl _arm64_search_memchr
.globl _arm64_search_literal
.globl _arm64_search_literal_icase

// ==============================================================================
// 1. arm64_search_memchr
// x0 = haystack, x1 = len, w2 = needle (byte)
// Returns: pointer to first match or 0
// ==============================================================================
_arm64_search_memchr:
    cbz     x0, .Lmemchr_not_found
    cbz     x1, .Lmemchr_not_found

    dup     v7.16b, w2           // Replicate needle byte across 16 lanes

    // 64-byte unrolled loop
.Lmemchr_loop64:
    cmp     x1, #64
    b.lo    .Lmemchr_check16

    ld1     {v0.16b, v1.16b, v2.16b, v3.16b}, [x0]

    cmeq    v0.16b, v0.16b, v7.16b
    cmeq    v1.16b, v1.16b, v7.16b
    cmeq    v2.16b, v2.16b, v7.16b
    cmeq    v3.16b, v3.16b, v7.16b

    orr     v4.16b, v0.16b, v1.16b
    orr     v5.16b, v2.16b, v3.16b
    orr     v4.16b, v4.16b, v5.16b

    umaxv   b6, v4.16b
    fmov    w3, s6
    cbnz    w3, .Lmemchr_found_in_64

    add     x0, x0, #64
    sub     x1, x1, #64
    b       .Lmemchr_loop64

.Lmemchr_found_in_64:
    // Determine which of the 4 vectors matched
    umaxv   b6, v0.16b
    fmov    w3, s6
    cbnz    w3, .Lmemchr_find_in_v0

    add     x0, x0, #16
    umaxv   b6, v1.16b
    fmov    w3, s6
    cbnz    w3, .Lmemchr_find_in_v1

    add     x0, x0, #16
    umaxv   b6, v2.16b
    fmov    w3, s6
    cbnz    w3, .Lmemchr_find_in_v2

    add     x0, x0, #16
    b       .Lmemchr_find_in_v3

.Lmemchr_find_in_v0:
    // Match is in v0 at current x0
    b       .Lmemchr_scalar_match_16
.Lmemchr_find_in_v1:
    b       .Lmemchr_scalar_match_16
.Lmemchr_find_in_v2:
    b       .Lmemchr_scalar_match_16
.Lmemchr_find_in_v3:
    // Match is in v3
    b       .Lmemchr_scalar_match_16

.Lmemchr_check16:
    cmp     x1, #16
    b.lo    .Lmemchr_scalar

    ldr     q0, [x0]
    cmeq    v0.16b, v0.16b, v7.16b
    umaxv   b6, v0.16b
    fmov    w3, s6
    cbnz    w3, .Lmemchr_scalar_match_16

    add     x0, x0, #16
    sub     x1, x1, #16
    b       .Lmemchr_check16

.Lmemchr_scalar_match_16:
    // Scan up to 16 bytes for first exact match
    mov     w4, #16
.Lmemchr_scan16_loop:
    ldrb    w3, [x0]
    uxtb    w2, w2
    cmp     w3, w2
    b.eq    .Lmemchr_matched
    add     x0, x0, #1
    subs    w4, w4, #1
    b.ne    .Lmemchr_scan16_loop

.Lmemchr_scalar:
    cbz     x1, .Lmemchr_not_found
    uxtb    w2, w2
.Lmemchr_scalar_loop:
    ldrb    w3, [x0]
    cmp     w3, w2
    b.eq    .Lmemchr_matched
    add     x0, x0, #1
    subs    x1, x1, #1
    b.ne    .Lmemchr_scalar_loop

.Lmemchr_not_found:
    mov     x0, #0
    ret

.Lmemchr_matched:
    ret


// ==============================================================================
// 2. arm64_search_literal
// x0 = haystack, x1 = hlen, x2 = needle, x3 = nlen
// Returns: pointer to first match or 0
// ==============================================================================
_arm64_search_literal:
    // Quick validation
    cbz     x0, .Llit_not_found
    cbz     x2, .Llit_not_found
    cbz     x3, .Llit_not_found
    cmp     x1, x3
    b.lo    .Llit_not_found

    // Special case: needle length 1 -> use memchr
    cmp     x3, #1
    b.ne    .Llit_multi_byte
    ldrb    w2, [x2]
    b       _arm64_search_memchr

.Llit_multi_byte:
    // Save registers
    stp     x19, x20, [sp, #-48]!
    stp     x21, x22, [sp, #16]
    stp     x29, x30, [sp, #32]
    add     x29, sp, #32

    mov     x19, x0              // haystack start
    mov     x20, x1              // hlen
    mov     x21, x2              // needle
    mov     x22, x3              // nlen

    // First and last characters
    ldrb    w4, [x21]            // first byte
    sub     x5, x22, #1
    ldrb    w5, [x21, x5]        // last byte

    dup     v4.16b, w4           // first byte vector
    dup     v5.16b, w5           // last byte vector

    // Remaining search range: (hlen - nlen)
    sub     x6, x20, x22         // max offset
    add     x7, x19, x6          // end pointer for first byte

.Llit_loop:
    cmp     x19, x7
    b.hi    .Llit_done_not_found

    // How many bytes left from x19 to end?
    sub     x8, x7, x19
    add     x8, x8, #1           // bytes left to probe first char

    cmp     x8, #16
    b.lo    .Llit_scalar_check

    // Vector probe: load 16 bytes for first char and last char
    ldr     q0, [x19]
    sub     x9, x22, #1
    add     x9, x19, x9
    ldr     q1, [x9]

    cmeq    v0.16b, v0.16b, v4.16b
    cmeq    v1.16b, v1.16b, v5.16b
    and     v0.16b, v0.16b, v1.16b

    umaxv   b2, v0.16b
    fmov    w10, s2
    cbz     w10, .Llit_advance_16

.Llit_scalar_check:
    // Candidate present in this window or scalar tail
    // Verify needle at x19
    mov     x10, #0              // index
.Llit_cmp_loop:
    cmp     x10, x22
    b.eq    .Llit_match_found    // Full match!
    ldrb    w11, [x19, x10]
    ldrb    w12, [x21, x10]
    cmp     w11, w12
    b.ne    .Llit_next_byte
    add     x10, x10, #1
    b       .Llit_cmp_loop

.Llit_next_byte:
    add     x19, x19, #1
    b       .Llit_loop

.Llit_advance_16:
    add     x19, x19, #16
    b       .Llit_loop

.Llit_match_found:
    mov     x0, x19              // matched pointer
    ldp     x21, x22, [sp, #16]
    ldp     x19, x20, [sp]
    ldp     x29, x30, [sp, #32]
    add     sp, sp, #48
    ret

.Llit_done_not_found:
    ldp     x21, x22, [sp, #16]
    ldp     x19, x20, [sp]
    ldp     x29, x30, [sp, #32]
    add     sp, sp, #48
.Llit_not_found:
    mov     x0, #0
    ret


// ==============================================================================
// 3. arm64_search_literal_icase
// x0 = haystack, x1 = hlen, x2 = needle_lower, x3 = nlen
// Returns: pointer to first case-insensitive match or 0
// ==============================================================================
_arm64_search_literal_icase:
    cbz     x0, .Licase_not_found
    cbz     x2, .Licase_not_found
    cbz     x3, .Licase_not_found
    cmp     x1, x3
    b.lo    .Licase_not_found

    stp     x19, x20, [sp, #-48]!
    stp     x21, x22, [sp, #16]
    stp     x29, x30, [sp, #32]
    add     x29, sp, #32

    mov     x19, x0              // haystack
    mov     x20, x1              // hlen
    mov     x21, x2              // needle (must be lowercase)
    mov     x22, x3              // nlen

    sub     x6, x20, x22
    add     x7, x19, x6          // end pointer

.Licase_outer_loop:
    cmp     x19, x7
    b.hi    .Licase_done_not_found

    // Check match starting at x19
    mov     x10, #0
.Licase_cmp_loop:
    cmp     x10, x22
    b.eq    .Licase_match_found
    ldrb    w11, [x19, x10]
    ldrb    w12, [x21, x10]

    // Lowercase w11 if in 'A'..'Z'
    sub     w13, w11, #'A'
    cmp     w13, #25             // 'Z' - 'A' = 25
    b.hi    .Licase_no_to_lower
    add     w11, w11, #32
.Licase_no_to_lower:
    cmp     w11, w12
    b.ne    .Licase_mismatch
    add     x10, x10, #1
    b       .Licase_cmp_loop

.Licase_mismatch:
    add     x19, x19, #1
    b       .Licase_outer_loop

.Licase_match_found:
    mov     x0, x19
    ldp     x21, x22, [sp, #16]
    ldp     x19, x20, [sp]
    ldp     x29, x30, [sp, #32]
    add     sp, sp, #48
    ret

.Licase_done_not_found:
    ldp     x21, x22, [sp, #16]
    ldp     x19, x20, [sp]
    ldp     x29, x30, [sp, #32]
    add     sp, sp, #48
.Licase_not_found:
    mov     x0, #0
    ret
