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

/* based on musl libc's qsort.c */
#include <stdlib.h>
#include <string.h>

#define MIN(a, b)	((a) < (b) ? (a) : (b))

static void swap(char *a, char *b, int sz)
{
	char tmp[64];

	while (sz) {
		int l = MIN(sizeof(tmp), sz);
		memcpy(tmp, a, l);
		memcpy(a, b, l);
		memcpy(b, tmp, l);
		a += l;
		b += l;
		sz -= l;
	}
}

static void fix(char *a, int root, int n, int sz, int (*cmp)(void *, void *))
{
	while (2 * root <= n) {
		int max = 2 * root;
		if (max < n && cmp(a + max * sz, a + (max + 1) * sz) < 0)
			max++;
		if (max && cmp(a + root * sz, a + max * sz) < 0) {
			swap(a + root * sz, a + max * sz, sz);
			root = max;
		} else {
			break;
		}
	}
}

void qsort(void *a, size_t n, size_t width, int (*cmp)(void *, void *))
{
	int i;

	if (!n)
		return;
	for (i = (n + 1) >> 1; i; i--)
		fix(a, i - 1, n - 1, width, cmp);
	for (i = n - 1; i; i--) {
		swap(a, a + i * width, width);
		fix(a, 0, i - 1, width, cmp);
	}
}
