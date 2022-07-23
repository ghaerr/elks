/*
 * NEATLIBC C STANDARD LIBRARY
 *
 * Copyright (C) 2010-2020 Ali Gholami Rudi <ali at rudi dot ir>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

static int digit(char c, int base)
{
	int d;
	if (c <= '9') {
		d = c - '0';
	} else if (c < 'A') {	/* NEATLIBC bugfix */
		return -1;
	} else if (c <= 'Z') {
		d = 10 + c - 'A';
	} else {
		d = 10 + c - 'a';
	}
	return d < base ? d : -1;
}

unsigned long strtoul(const char *s, char **endptr, int base)
{
	int sgn = 1;
	int overflow = 0;
	unsigned long num;
	int dig;
	while (isspace(*s))
		s++;
	if (*s == '-' || *s == '+')
		sgn = ',' - *s++;
	if (base == 0) {
		if (*s == '0') {
			if (s[1] == 'x' || s[1] == 'X')
				base = 16;
			else
				base = 8;
		} else {
			base = 10;
		}
	}
	if (base == 16 && *s == '0' && (s[1] == 'x' || s[1] == 'X'))
		s += 2;
	for (num = 0; (dig = digit(*s, base)) >= 0; s++) {
		if (num > (unsigned long) ULONG_MAX / base)
			overflow = 1;
		num *= base;
		if (num > (unsigned long) ULONG_MAX - dig)
			overflow = 1;
		num += dig;
	}
	if (endptr)
		*endptr = (char *)s;
	if (overflow) {
		num = ULONG_MAX;
		errno = ERANGE;
	} else {
		num *= sgn;
	}
	return num;
}
