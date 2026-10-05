// ==============================================================================
// asmgrep: Ultra-Fast ARM64 NEON Binary Probe
// File: asmgrep/src/asm/binary_probe.s
//
// Description: Scans the first 512 bytes of a buffer for null bytes (0x00).
//              Uses 64-byte unrolled NEON SIMD to finish in ~8 vector operations.
//
// Function signature:
//   int arm64_is_binary(const uint8_t *buf, size_t len);
//   Returns: 1 if binary (contains null byte), 0 if text
// ==============================================================================

.text
.p2align 4
.globl _arm64_is_binary

_arm64_is_binary:
    // x0 = buf, x1 = len
    cbz     x0, .Lnot_binary
    cbz     x1, .Lnot_binary

    // Limit probe to min(len, 512)
    mov     x2, #512
    cmp     x1, x2
    csel    x1, x1, x2, lo        // x1 = min(len, 512)

    // v7 = zero vector for null byte comparison
    movi    v7.16b, #0

    // Check if at least 64 bytes
.Lloop64_probe:
    cmp     x1, #64
    b.lo    .Lcheck16_probe

    // Load 64 bytes unrolled into v0-v3
    ld1     {v0.16b, v1.16b, v2.16b, v3.16b}, [x0], #64
    sub     x1, x1, #64

    // Compare with zero
    cmeq    v0.16b, v0.16b, v7.16b
    cmeq    v1.16b, v1.16b, v7.16b
    cmeq    v2.16b, v2.16b, v7.16b
    cmeq    v3.16b, v3.16b, v7.16b

    // Combine comparison masks
    orr     v0.16b, v0.16b, v1.16b
    orr     v2.16b, v2.16b, v3.16b
    orr     v0.16b, v0.16b, v2.16b

    // Horizontal max across 16 bytes
    umaxv   b0, v0.16b
    fmov    w3, s0
    cbnz    w3, .Lis_binary_found
    b       .Lloop64_probe

.Lcheck16_probe:
    cmp     x1, #16
    b.lo    .Lcheck_scalar_probe

    ldr     q0, [x0], #16
    sub     x1, x1, #16
    cmeq    v0.16b, v0.16b, v7.16b
    umaxv   b0, v0.16b
    fmov    w3, s0
    cbnz    w3, .Lis_binary_found
    b       .Lcheck16_probe

.Lcheck_scalar_probe:
    cbz     x1, .Lnot_binary
.Lscalar_loop:
    ldrb    w2, [x0], #1
    cbz     w2, .Lis_binary_found
    subs    x1, x1, #1
    b.ne    .Lscalar_loop

.Lnot_binary:
    mov     w0, #0
    ret

.Lis_binary_found:
    mov     w0, #1
    ret
