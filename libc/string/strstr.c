/*
 * Jody Bruchon's two-way strstr() function
 *
 * This implementation attempts to exclude substrings faster by scanning
 * them from both directions at once. It is faster than naive one-by-one
 * comparison implementations but slower than larger, more complex ones
 * seen in C libraries such as glibc and musl. The big advantage over the
 * faster versions is that resource usage is very low, making it more
 * suitable for embedded, low-memory, or small-cache systems.
 *
 * Copyright (C) 2014-2015 by Jody Bruchon <jody@jodybruchon.com>
 * Released under the terms of the GNU GPL version 2
 *
 * K&R-ified for bcc compiler in Dev86
 */

#include <stdio.h>
#include <string.h>

char *
strstr(s1, s2)
const char *s1; const char *s2;
{
	register const char *haystack = s1;
	register const char *needle = s2;
	int pos = 0;
	register int cnt;
	size_t hay_len;
	size_t n_len;

	if (!*needle) return (char *) s1;
	n_len = strlen(needle);
	hay_len = strlen(haystack);

	cnt = n_len;
	do {
		/* Give up if needle is longer than remaining haystack */
		if ((pos + n_len - 1) > hay_len) return NULL;

		/* Scan for match in both directions */
		while (*needle == *haystack) {
			cnt--;
			if (!cnt) return (char *) (s1 + pos);
			if (*(needle + cnt) == *(haystack + cnt)) {
				cnt--;
				if (!cnt) return (char *) (s1 + pos);
			} else break;
			needle++;
			haystack++;
		}
		pos++;
		haystack = s1 + pos;
		needle = s2;
		cnt = n_len;
	} while (1);

}
