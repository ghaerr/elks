/*
 * Exomizer decruncher ported to ELKS by Greg Haerr, Apr 2021
 * Copyright (c) 2005-2017 Magnus Lind.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented * you must not
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
 * This decompressor decompresses files that have been compressed
 * using the raw sub-sub command with the -b (not default) and -P39
 * (default) setting of the raw command.
 *
 * This file is shared by the kernel and sys_utils/decomp application binary.
 */

#ifdef __KERNEL__
#include <linuxmt/types.h>
#include <linuxmt/memory.h>
#include <linuxmt/kernel.h>
#include <linuxmt/debug.h>
#define read_byte()     peekb((word_t)--inp, segp)
#define ERR(...)        debug(__VA_ARGS__)
#else
#include <stdio.h>
#include <string.h>
#define read_byte()     (*--inp & 255)
#define ERR(...)        fprintf(stderr, __VA_ARGS__)
#endif

static unsigned short int base[52];
static char bits[52];
static unsigned char bit_buffer;
static const char *inp;
static unsigned int segp;

static int bitbuffer_rotate(int carry)
{
    int carry_out;
    /* rol */
    carry_out = (bit_buffer & 0x80) != 0;
    bit_buffer <<= 1;
    if (carry)
    {
        bit_buffer |= 0x01;
    }
    return carry_out;
}

static unsigned short int
read_bits(int bit_count)
{
    unsigned short int ubits = 0;
    int byte_copy = bit_count & 8;
    bit_count &= 7;

    while(bit_count-- > 0)
    {
        int carry = bitbuffer_rotate(0);
        if (bit_buffer == 0)
        {
            bit_buffer = read_byte();
            carry = bitbuffer_rotate(1);
        }
        ubits <<= 1;
        ubits |= carry;
    }
    if (byte_copy != 0)
    {
        ubits <<= 8;
        ubits |= read_byte();
    }
    return ubits;
}

static void
init_table(void)
{
    int i;
    unsigned short int b2 = 0;

    for(i = 0; i < 52; ++i)
    {
        unsigned short int b1;
        if((i & 15) == 0)
        {
            b2 = 1;
        }
        base[i] = b2;

        b1 = read_bits(3);
        b1 |= read_bits(1) << 3;
        bits[i] = b1;

        b2 += 1 << b1;
    }
}

static char *
exo_decrunch(const char *in, char *out)
{
    unsigned short int index;
    unsigned short int length;
    unsigned short int offset;
    char c;
    char literal = 1;
    char reuse_offset_state = 1;

    inp = in;
    bit_buffer = read_byte();

    init_table();

    goto implicit_literal_byte;
    for(;;)
    {
        literal = read_bits(1);
        if(literal == 1)
        {
        implicit_literal_byte:
            /* literal byte */
            length = 1;
            goto copy;
        }
        index = 0;
        while(read_bits(1) == 0)
        {
            ++index;
        }
        if(index == 16)
        {
            break;
        }
        if(index == 17)
        {
            literal = 1;
            length = read_byte() << 8;
            length |= read_byte();
            goto copy;
        }
        length = base[index];
        length += read_bits(bits[index]);

        if ((reuse_offset_state & 3) != 1 || !read_bits(1))
        {
            switch(length)
            {
            case 1:
                index = read_bits(2);
                index += 48;
                break;
            case 2:
                index = read_bits(4);
                index += 32;
                break;
            default:
                index = read_bits(4);
                index += 16;
                break;
            }
            offset = base[index];
            offset += read_bits(bits[index]);
        }
    copy:
        do
        {
            --out;
            if(literal)
            {
                c = read_byte();
            }
            else
            {
#ifdef __KERNEL__
                c = peekb((word_t)out+offset, segp);
#else
                c = out[offset];
#endif
            }
#ifdef __KERNEL__
            pokeb((word_t)out, segp, c);
#else
            *out = c;
#endif
			if ((int)(out - inp) < 0)
			{
				ERR("Decompress output overflow by %d\n", (int)(out - inp));
				return 0;
			}
        }
        while(--length > 0);

        reuse_offset_state = (reuse_offset_state << 1) | literal;
    }
    return out;
}

unsigned int decompress(char *buf, unsigned int seg, unsigned int orig_size,
	unsigned int compr_size, int safety)
{
	char *in = buf + compr_size;
	char *out = buf + orig_size + safety;

	/*ERR("decompress: seg %x orig %u compr %u safety %d\n",
		seg, orig_size, compr_size, safety);*/
	segp = seg;
	out = exo_decrunch(in, out);
	if (out - buf != safety)
	{
		ERR("Error decompressing executable\n");
		return 0;
	}
#ifdef __KERNEL__
	fmemcpyb(buf, seg, out, seg, orig_size);
#else
	memcpy(buf, out, orig_size);
#endif
	return orig_size;
}
