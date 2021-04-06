#ifndef INCLUDED_FLAGS
#define INCLUDED_FLAGS
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Copyright (c) 2018 Magnus Lind.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software, alter it and re-
 * distribute it freely for any non-commercial, non-profit purpose subject to
 * the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must not
 *   claim that you wrote the original software. If you use this software in a
 *   product, an acknowledgment in the product documentation would be
 *   appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must not
 *   be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any distribution.
 *
 *   4. The names of this software and/or it's copyright holders may not be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 */

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

/*
 * bit 0  Literal sequences
 * bit 1  Sequences with length 1
 * bit 2  Sequences with length > 255 where (length & 255) would have been
 *        using its own decrunch table
 */
#define TBIT_LIT_SEQ 0
#define TBIT_LEN1_SEQ 1
#define TBIT_LEN0123_SEQ_MIRRORS 2

#define TFLAG_LIT_SEQ            (1 << TBIT_LIT_SEQ)
#define TFLAG_LEN1_SEQ           (1 << TBIT_LEN1_SEQ)
#define TFLAG_LEN0123_SEQ_MIRRORS (1 << TBIT_LEN0123_SEQ_MIRRORS)

#ifdef __cplusplus
}
#endif
#endif
