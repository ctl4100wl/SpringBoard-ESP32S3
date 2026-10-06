/* Aligned ARGB span fill for ThorVG. Four pixels per LX7 vector store. */
    .section .text
    .align 4
    .global s3_fill32_aligned
    .type s3_fill32_aligned,@function
s3_fill32_aligned:
    entry a1, 32
    ee.movi.32.q q0, a3, 0
    ee.movi.32.q q0, a3, 1
    ee.movi.32.q q0, a3, 2
    ee.movi.32.q q0, a3, 3
1:
    ee.vst.128.ip q0, a2, 16
    addi a4, a4, -1
    bnez a4, 1b
    retw.n
    .size s3_fill32_aligned, .-s3_fill32_aligned
