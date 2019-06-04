/* Copyright (C) 2019 TK Chia
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 */

/* Function implementation of putchar.  putchar is defined as a macro in
 * <stdio.h>, but GCC sometimes rewrites calls to printf as putchar (or puts)
 * calls, which will appear as actual function calls.
 */

#include <stdio.h>

#undef putchar

int
putchar(int c)
{
   return putc(c, stdout);
}
