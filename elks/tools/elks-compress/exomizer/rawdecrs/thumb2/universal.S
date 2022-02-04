/*
 * Copyright (C) 2020 by Alex Kazik <git@kazik.de>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose
 * with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 */

/*
 * the file has to be crunched with
 * $ exomizer raw -C -b -P39 IN -o OUT
 * you can change the P39 to anything from 0-63 as long as the value below is also changed
 */

#define FLAGS_PROTO 39

/*
 * there are two optional security options
 * - CHECK_BUFFER_SIZE - for input and output a buffer size has to be specified
 * - CHECK_OVERRUN - the input and output MUST be in the same buffer - check if the output overruns the input
 * none, one or both can be used
 * in case of a failure the output pointer is NULL
 * to enable them just uncomment the following define
 */
//#define CHECK_BUFFER_SIZE
//#define CHECK_OVERRUN

/*
 * bit 0  Controls bit bit orientation, 1=big endian, 0=little endian
 * bit 1  Contols how more than 7 bits are shifted 1=split into a shift of
 *        of less than 8 bits + a byte (new), 0=all bits are shifted
 * bit 2  Implicit first literal byte: 1=enable, 0=disable
 * bit 3  Align bit stream towards start without flag: 1=enable, 0=disable
 * bit 4  Decides if we are to have two lengths (1 and 2) or three lengths
 *        (1, 2 and 3) using dedicated decrunch tables: 0=two, 1=three
 * bit 5  Decides if we are reusing offsets: 1=enable, 0=disable
 */

#define PBIT_BITS_ORDER_BE     0
#define PBIT_BITS_COPY_GT_7    1
#define PBIT_IMPL_1LITERAL     2
#define PBIT_BITS_ALIGN_START  3
#define PBIT_4_OFFSET_TABLES   4
#define PBIT_REUSE_OFFSET      5

#define PFLAG_BITS_ORDER_BE    (1 << PBIT_BITS_ORDER_BE)
#define PFLAG_BITS_COPY_GT_7   (1 << PBIT_BITS_COPY_GT_7)
#define PFLAG_IMPL_1LITERAL    (1 << PBIT_IMPL_1LITERAL)
#define PFLAG_BITS_ALIGN_START (1 << PBIT_BITS_ALIGN_START)
#define PFLAG_4_OFFSET_TABLES  (1 << PBIT_4_OFFSET_TABLES)
#define PFLAG_REUSE_OFFSET     (1 << PBIT_REUSE_OFFSET)

#if !defined(FLAGS_PROTO) || FLAGS_PROTO < 0 || FLAGS_PROTO > 63
#error "FLAGS_PROTO must be set"
#endif

.syntax unified
.thumb
.section .text
.global exo_decrunch
.type exo_decrunch,%function
.fnstart

    /*
    PARAMETER:
        r0 = output pointer
        r1 = input pointer
        r2 = output size (only with CHECK_BUFFER_SIZE)
        r3 = input size (only with CHECK_BUFFER_SIZE)

    RETURN:
        r0 = output pointer (null with a filed check)
        r1 = input pointer

    GLOBAL REGISTER USAGE:
        r0 = output pointer
        r1 = input pointer
        r2 = scratch / output get_bits
        r3 = scratch / input get_bits
        r4 = length (used different in init)
        r5 = index / offset (used different in init)
        r6 = scratch get_bits (only when COPY_GT_7 is active)
        r7 = bit_buffer
        r8 = base pointer
        ..
        r10 = output size (only with CHECK_BUFFER_SIZE)
        r11 = input size (only with CHECK_BUFFER_SIZE)
        r12 = reuse_offset_state (only with PFLAG_REUSE_OFFSET) (used different in init)
        ..
        sp = base pointer (2*52 / 2*68 bytes)
    */

    #define reg_ptr_out r0
    #define reg_ptr_in r1
    #define reg_bit_buffer r7
    #define reg_ptr_bits r8
    #define reg_out_size r10
    #define reg_in_size r11
    #define reg_ptr_base sp

.p2align 2
exo_decrunch:
    #ifdef CHECK_BUFFER_SIZE
    #if FLAGS_PROTO & PFLAG_BITS_COPY_GT_7
    push {r4, r5, r6, r7, r8, r10, r11, lr}
    #else
    push {r4, r5,     r7, r8, r10, r11, lr}
    #endif
    #else
    #if FLAGS_PROTO & PFLAG_BITS_COPY_GT_7
    push {r4, r5, r6, r7, r8, lr}
    #else
    push {r4, r5,     r7, r8, lr}
    #endif
    #endif

    #ifdef CHECK_BUFFER_SIZE
    // copy size
    mov reg_out_size, r2
    mov reg_in_size, r3
    #endif

    // reserve stack
    #if !(FLAGS_PROTO & PFLAG_4_OFFSET_TABLES)
    sub sp, sp, 2*52+52
    add reg_ptr_bits, sp, # 2*52
    #else
    sub sp, sp, 2*68+68
    add reg_ptr_bits, sp, # 2*68
    #endif

    // init
    #if FLAGS_PROTO & PFLAG_BITS_ALIGN_START
    movs reg_bit_buffer, # 0
    #else
    #ifdef CHECK_BUFFER_SIZE
    subs reg_in_size, # 1
    bmi exit_failure
    #endif
    ldrb reg_bit_buffer, [reg_ptr_in, #-1]!
    #if FLAGS_PROTO & PFLAG_BITS_ORDER_BE
    lsls reg_bit_buffer, reg_bit_buffer, # 24
    #endif
    #endif

    /*
    init_table
        r4 = "i"
        r5 = "a"
        r2 = "b" - when get_bits does not need to be called
        r12 = "b" - only temporary
    */
    movs r4, # 0
init_loop:
    tst r4, # 0xf
    iteee eq
    moveq r5, # 1
    movne r3, # 1
    lslne r3, r3, r2
    addne r5, r5, r3
    strh r5, [reg_ptr_base, r4, LSL # 1]

    #if FLAGS_PROTO & PFLAG_BITS_COPY_GT_7
    movs r3, # 3
    bl get_bits
    mov r12, r2
    movs r3, # 1
    bl get_bits
    lsls r2, r2, # 3
    orrs r2, r2, r12
    #else
    movs r3, # 4
    bl get_bits
    #endif
    strb r2, [reg_ptr_bits, r4]
    adds r4, r4, # 1
    #if !(FLAGS_PROTO & PFLAG_4_OFFSET_TABLES)
    cmp r4, # 52
    #else
    cmp r4, # 68
    #endif
    bne init_loop

    /*
    decrunch
        r4 = length
        r5 = index / offset
        r12 = reuse_offset_state (only with PFLAG_REUSE_OFFSET)
    */
    #if FLAGS_PROTO & PFLAG_REUSE_OFFSET
    mov r12, # 1
    #endif
    #if FLAGS_PROTO & PFLAG_IMPL_1LITERAL
    b literal_1_byte
    #else
    b main_loop
    #endif

get_offset:
    // r5 = base index, r3 = bits to add to index
    bl get_bits
    adds r5, r2, r5 // correct index
    // fetch offset
    ldrb r3, [reg_ptr_bits, r5]
    bl get_bits
    ldrh r5, [reg_ptr_base, r5, LSL # 1]
    adds r5, r5, r2
    subs r5, r5, # 1
    #ifdef CHECK_BUFFER_SIZE
    subs reg_out_size, r4
    bmi exit_failure
    #endif
    #ifdef CHECK_OVERRUN
    sub r3, reg_ptr_out, r4
    cmp reg_ptr_in, r3
    bhs exit_failure
    #endif
copy_loop:
    ldrb r2, [reg_ptr_out, r5]
    strb r2, [reg_ptr_out, #-1]!
    subs r4, r4, # 1
    bne copy_loop

    #if FLAGS_PROTO & PFLAG_REUSE_OFFSET
    lsl r12, r12, # 16
    #endif

main_loop:
    movs r2, # 0
gamma_loop:
    adds r2, r2, 1

    // get a single bit
    #if FLAGS_PROTO & PFLAG_BITS_ORDER_BE
    lsls reg_bit_buffer, reg_bit_buffer, # 1
    bne get_bit_skip_reload
    #ifdef CHECK_BUFFER_SIZE
    subs reg_in_size, # 1
    bmi exit_failure
    #endif
    ldrb reg_bit_buffer, [reg_ptr_in, #-1]!
    lsls reg_bit_buffer, reg_bit_buffer, # 25
    orr reg_bit_buffer, reg_bit_buffer, # 1 << 24
    #else
    lsrs reg_bit_buffer, reg_bit_buffer, # 1
    bne get_bit_skip_reload
    #ifdef CHECK_BUFFER_SIZE
    subs reg_in_size, # 1
    bmi exit_failure
    #endif
    ldrb reg_bit_buffer, [reg_ptr_in, #-1]!
    lsrs reg_bit_buffer, reg_bit_buffer, # 1
    orr reg_bit_buffer, reg_bit_buffer, # 1 << 7
    #endif
get_bit_skip_reload:

    bcc gamma_loop
    subs r2, r2, # 2
    bpl no_literal_1_byte

literal_1_byte:
    movs r2, # 1
    b copy_literal

no_literal_1_byte:
    cmp r2, # 16
    beq exit
    blo no_literal_gamma
    // index 17 -> copy literal sequence

    #if FLAGS_PROTO & PFLAG_BITS_COPY_GT_7
    #ifdef CHECK_BUFFER_SIZE
    subs reg_in_size, # 2
    bmi exit_failure
    #endif
    ldrh r2, [reg_ptr_in, #-2]!
    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    rev16 r2, r2
    #endif
    #else
    movs r3, # 16
    bl get_bits
    #endif

    #ifdef CHECK_BUFFER_SIZE
    subs reg_in_size, r2
    bmi exit_failure
    #endif

copy_literal:
    ldrb r3, [reg_ptr_in, #-1]!
    strb r3, [reg_ptr_out, #-1]!
    subs r2, r2, # 1
    bne copy_literal
    #if FLAGS_PROTO & PFLAG_REUSE_OFFSET
    lsl r12, r12, # 16
    orr r12, r12, # 1
    #endif
    b main_loop

no_literal_gamma:
    ldrh r4, [reg_ptr_base, r2, LSL # 1]
    ldrb r3, [reg_ptr_bits, r2]
    bl get_bits
    adds r4, r4, r2

    // reuse offset?
    #if FLAGS_PROTO & PFLAG_REUSE_OFFSET
    cmp r12, # 1
    bne no_reuse
    // get a bit
    #if FLAGS_PROTO & PFLAG_BITS_ORDER_BE
    lsls reg_bit_buffer, reg_bit_buffer, # 1
    bne reuse_get_bits_skip_reload
    #ifdef CHECK_BUFFER_SIZE
    subs reg_in_size, # 1
    bmi exit_failure
    #endif
    ldrb reg_bit_buffer, [reg_ptr_in, #-1]!
    lsls reg_bit_buffer, reg_bit_buffer, # 25
    orr reg_bit_buffer, # 1 << 24
    #else
    lsrs reg_bit_buffer, reg_bit_buffer, # 1
    bne reuse_get_bits_skip_reload
    #ifdef CHECK_BUFFER_SIZE
    subs reg_in_size, # 1
    bmi exit_failure
    #endif
    ldrb reg_bit_buffer, [reg_ptr_in, #-1]!
    lsrs reg_bit_buffer, reg_bit_buffer, # 1
    orr reg_bit_buffer, # 1 << 7
    #endif
reuse_get_bits_skip_reload:
    bcs copy_loop
no_reuse:
    #endif

    // copy length and saturate it
    mov r2, r4
    #if !(FLAGS_PROTO & PFLAG_4_OFFSET_TABLES)
    usat r2, # 2, r2
    tbb [pc, r2]
tab:
    .byte 0 // length zero never happens
    .byte (len1 - tab) / 2
    .byte (len2 - tab) / 2
    .byte (len3ff - tab) / 2
len1:
    movs r3, # 2
    movs r5, # 48
    b get_offset
len2:
    movs r3, # 4
    movs r5, # 32
    b get_offset
len3ff:
    movs r3, # 4
    movs r5, # 16
    b get_offset
    #else
    usat r2, # 3, r2
    tbb [pc, r2]
tab:
    .byte 0 // length zero never happens
    .byte (len1 - tab) / 2
    .byte (len2 - tab) / 2
    .byte (len3 - tab) / 2
    .byte (len4ff - tab) / 2
    .byte (len4ff - tab) / 2
    .byte (len4ff - tab) / 2
    .byte (len4ff - tab) / 2
len1:
    movs r3, # 2
    movs r5, # 64
    b get_offset
len2:
    movs r3, # 4
    movs r5, # 48
    b get_offset
len3:
    movs r3, # 4
    movs r5, # 32
    b get_offset
len4ff:
    movs r3, # 4
    movs r5, # 16
    b get_offset
    #endif

#if defined(CHECK_BUFFER_SIZE) || defined(CHECK_OVERRUN)
exit_failure:
    movs r0, # 0
#endif

exit:
    #if !(FLAGS_PROTO & PFLAG_4_OFFSET_TABLES)
    add sp, sp, 2*52+52
    #else
    add sp, sp, 2*68+68
    #endif
    #ifdef CHECK_BUFFER_SIZE
    #if FLAGS_PROTO & PFLAG_BITS_COPY_GT_7
    pop {r4, r5, r6, r7, r8, r10, r11, pc}
    #else
    pop {r4, r5,     r7, r8, r10, r11, pc}
    #endif
    #else
    #if FLAGS_PROTO & PFLAG_BITS_COPY_GT_7
    pop {r4, r5, r6, r7, r8, pc}
    #else
    pop {r4, r5,     r7, r8, pc}
    #endif
    #endif

    /*
    get_bits
        input: r3 - number of bits to read
        output: r2 - the read bits
        scratch: r6 (only when COPY_GT_7 is active)
    */

.p2align 2
get_bits:
    movs r2, # 0 // output
    #if FLAGS_PROTO & PFLAG_BITS_COPY_GT_7
    lsr r6, r3, # 3
    and r3, r3, # 7
    #endif
    cbz r3, get_bits_loop_end
get_bits_loop:
    #if FLAGS_PROTO & PFLAG_BITS_ORDER_BE
    lsls reg_bit_buffer, reg_bit_buffer, # 1
    bne get_bits_skip_reload
    #ifdef CHECK_BUFFER_SIZE
    subs reg_in_size, # 1
    bmi exit_failure
    #endif
    ldrb reg_bit_buffer, [reg_ptr_in, #-1]!
    lsls reg_bit_buffer, reg_bit_buffer, # 25
    orr reg_bit_buffer, 1 << 24
    #else
    lsrs reg_bit_buffer, reg_bit_buffer, # 1
    bne get_bits_skip_reload
    #ifdef CHECK_BUFFER_SIZE
    subs reg_in_size, # 1
    bmi exit_failure
    #endif
    ldrb reg_bit_buffer, [reg_ptr_in, #-1]!
    lsrs reg_bit_buffer, reg_bit_buffer, # 1
    orr reg_bit_buffer, # 1 << 7
    #endif
get_bits_skip_reload:
    adcs r2, r2, r2

    subs r3, r3, # 1
    bne get_bits_loop

get_bits_loop_end:
    #if FLAGS_PROTO & PFLAG_BITS_COPY_GT_7
    cbz r6, get_bits_byte_loop_end
    lsls r2, r2, # 8
    #ifdef CHECK_BUFFER_SIZE
    subs reg_in_size, # 1
    bmi exit_failure
    #endif
    ldrb r3, [reg_ptr_in, #-1]!
    orrs r2, r2, r3
get_bits_byte_loop_end:
    #endif

    bx lr

.fnend
