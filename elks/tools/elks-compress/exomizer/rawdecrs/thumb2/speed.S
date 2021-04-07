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
 * This decruncher is small and fast (at least than the generic one)
 * The downside is that the bistream format is pretty fixed
 * - BITS_ORDER_BE must be on
 * - BITS_COPY_GT_7 must be off
 * - IMPL_1LITERAL can be chosen
 * - BITS_ALIGN_START must be set
 * - 4_OFFSET_TABLES can be chosen
 * - REUSE_OFFSET must be off
 *
 * the file has to be crunched with
 * $ exomizer raw -C -b -P13 IN -o OUT
 * you can change the P13 to anything valid, as described above: 9, 13, 25, 29
 * and as long as the value below is also changed
 */

#define FLAGS_PROTO 13

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

#if !(FLAGS_PROTO & PFLAG_BITS_ORDER_BE)
#error "PFLAG_BITS_ORDER_BE is required"
#endif

#if FLAGS_PROTO & PFLAG_BITS_COPY_GT_7
#error "PFLAG_BITS_COPY_GT_7 is not allowed"
#endif

#if !(FLAGS_PROTO & PFLAG_BITS_ALIGN_START)
#error "PFLAG_BITS_ALIGN_START is required"
#endif

#if FLAGS_PROTO & PFLAG_REUSE_OFFSET
#error "PFLAG_REUSE_OFFSET is not allowed"
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
        r6 = length of bit_buffer
        r7 = bit_buffer
        r8 = bits pointer (52 / 68 bytes)
        ..
        r10 = output size (only with CHECK_BUFFER_SIZE)
        r11 = input size (only with CHECK_BUFFER_SIZE)
        r12 = scratch
        ..
        sp = base pointer (2*52 / 2*68 bytes)
    */

    #define reg_ptr_out r0
    #define reg_ptr_in r1
    #define reg_bit_buffer_length r6
    #define reg_bit_buffer r7
    #define reg_ptr_bits r8
    #define reg_out_size r10
    #define reg_in_size r11
    #define reg_ptr_base sp

.p2align 2
exo_decrunch:
    #ifdef CHECK_BUFFER_SIZE
    push {r4, r5, r6, r7, r8, r10, r11, lr}
    #else
    push {r4, r5, r6, r7, r8, lr}
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
    movs reg_bit_buffer, # 0
    movs reg_bit_buffer_length, # 0

    /*
    init_table
        r4 = "i"
        r5 = "a"
        r2 = "b" - when get_bits does not need to be called
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

    movs r3, # 4
    bl get_bits
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
    */
    #if FLAGS_PROTO & PFLAG_IMPL_1LITERAL
    b literal_1_byte
    #else
    b main_loop
    #endif

get_offset:
    // r5 = base index, r3 = bits to add to index
    bl get_bits
    adds.w r5, r2, r5 // update index
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

main_loop:
    movs r2, # 0
    // count how many zeroes are there before the first one
gamma_loop:
    clz r3, reg_bit_buffer
    cmp r3, reg_bit_buffer_length
    bls gamma_end
    // tried to read more than available, add all avilable bits and reload buffer
    #ifdef CHECK_BUFFER_SIZE
    subs reg_in_size, # 1
    bmi exit_failure
    #endif
    adds r2, r2, reg_bit_buffer_length
    ldrb reg_bit_buffer, [reg_ptr_in, #-1]!
    lsls reg_bit_buffer, reg_bit_buffer, # 24
    movs reg_bit_buffer_length, # 8
    b gamma_loop
gamma_end:
    adds r2, r2, r3
    // update buffer: remove counted bits from it
    adds r3, r3, # 1
    subs reg_bit_buffer_length, reg_bit_buffer_length, r3
    lsls reg_bit_buffer, reg_bit_buffer, r3

    subs r2, r2, # 1
    bpl no_literal_1_byte

literal_1_byte:
    movs r2, # 1
    b copy_literal

no_literal_1_byte:
    cmp r2, # 16
    beq exit
    blo no_literal_gamma
    // index 17 -> copy literal sequence
    movs r3, # 16
    bl get_bits

    #ifdef CHECK_BUFFER_SIZE
    subs reg_in_size, r2
    bmi exit_failure
    #endif

copy_literal:
    ldrb r3, [reg_ptr_in, #-1]!
    strb r3, [reg_ptr_out, #-1]!
    subs r2, r2, # 1
    bne copy_literal
    #ifdef CHECK_BUFFER_SIZE
    b main_loop
    #else
    b.w main_loop
    #endif

no_literal_gamma:
    ldrh r4, [reg_ptr_base, r2, LSL # 1]
    ldrb r3, [reg_ptr_bits, r2]
    bl get_bits
    adds r4, r4, r2

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
    pop {r4, r5, r6, r7, r8, r10, r11, pc}
    #else
    pop {r4, r5, r6, r7, r8, pc}
    #endif

    /*
    get_bits
        input: r3 - number of bits to read
        output: r2 - the read bits
        scratch: r12
    */

.p2align 2

get_bits:
    // check if there are enough bits in the buffer
    cmp r3, reg_bit_buffer_length
    bls read_it
refill:
    // refill
    rsb r12, reg_bit_buffer_length, # 24
    #ifdef CHECK_BUFFER_SIZE
    subs reg_in_size, # 1
    bmi exit_failure
    #endif
    ldrb r2, [reg_ptr_in, #-1]!
    lsls r2, r2, r12
    orrs reg_bit_buffer, reg_bit_buffer, r2
    add reg_bit_buffer_length, reg_bit_buffer_length, # 8
    // check again, and restart if still not ehough bits
    cmp r3, reg_bit_buffer_length
    bhi refill

    // read the specified amount of bits
read_it:
    subs reg_bit_buffer_length, reg_bit_buffer_length, r3
    rsb r12, r3, # 32
    lsr r2, reg_bit_buffer, r12
    lsls reg_bit_buffer, reg_bit_buffer, r3

    bx lr

.fnend
