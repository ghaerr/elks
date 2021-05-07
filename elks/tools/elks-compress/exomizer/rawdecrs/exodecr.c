/*
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
 */

/**
 * This decompressor decompresses files that have been compressed
 * using the raw sub-sub command with the -b (not default) and -P39
 * (default) setting of the raw command.
 */
#include "exodecr.h"

static unsigned short int base[52];
static char bits[52];
static unsigned char bit_buffer;

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

static unsigned char read_byte(const char **inp)
{
    unsigned char val = *--(*inp) & 0xff;
    return val;
}

static unsigned short int
read_bits(const char **inp, int bit_count)
{
    unsigned short int bits = 0;
    int byte_copy = bit_count & 8;
    bit_count &= 7;

    while(bit_count-- > 0)
    {
        int carry = bitbuffer_rotate(0);
        if (bit_buffer == 0)
        {
            bit_buffer = read_byte(inp);
            carry = bitbuffer_rotate(1);
        }
        bits <<= 1;
        bits |= carry;
    }
    if (byte_copy != 0)
    {
        bits <<= 8;
        bits |= read_byte(inp);
    }
    return bits;
}

static void
init_table(const char **inp)
{
    int i;
    unsigned short int b2;

    for(i = 0; i < 52; ++i)
    {
        unsigned short int b1;
        if((i & 15) == 0)
        {
            b2 = 1;
        }
        base[i] = b2;

        b1 = read_bits(inp, 3);
        b1 |= read_bits(inp, 1) << 3;
        bits[i] = b1;

        b2 += 1 << b1;
    }
}

char *
exo_decrunch(const char *in, char *out)
{
    unsigned short int index;
    unsigned short int length;
    unsigned short int offset;
    char c;
    char literal = 1;
    char reuse_offset_state = 1;

    bit_buffer = read_byte(&in);

    init_table(&in);

    goto implicit_literal_byte;
    for(;;)
    {
        literal = read_bits(&in, 1);
        if(literal == 1)
        {
        implicit_literal_byte:
            /* literal byte */
            length = 1;
            goto copy;
        }
        index = 0;
        while(read_bits(&in, 1) == 0)
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
            length = read_byte(&in) << 8;
            length |= read_byte(&in);
            goto copy;
        }
        length = base[index];
        length += read_bits(&in, bits[index]);

        if ((reuse_offset_state & 3) != 1 || !read_bits(&in, 1))
        {
            switch(length)
            {
            case 1:
                index = read_bits(&in, 2);
                index += 48;
                break;
            case 2:
                index = read_bits(&in, 4);
                index += 32;
                break;
            default:
                index = read_bits(&in, 4);
                index += 16;
                break;
            }
            offset = base[index];
            offset += read_bits(&in, bits[index]);
        }
    copy:
        do
        {
            --out;
            if(literal)
            {
                c = read_byte(&in);
            }
            else
            {
                c = out[offset];
            }
            *out = c;
        }
        while(--length > 0);

        reuse_offset_state = (reuse_offset_state << 1) | literal;
    }
    return out;
}
