/* See LICENSE file for copyright and license details. */
#include <stdlib.h>
#include <string.h>

#include "util.h"
#define isdigit(c)  ((c) >= '0' && (c) <= '9')

int
strverscmp(const char *str1, const char *str2)
{
	size_t len1 = strlen(str1);
	size_t len2 = strlen(str2);
	size_t i1 = 0;
	size_t i2 = 0;
	for (; i1 < len1 && i2 < len2; i1++, i2++) {
		unsigned char c1 = str1[i1];
		unsigned char c2 = str2[i2];
		if (isdigit(c1) && isdigit(c2)) {
			unsigned long num1;
			unsigned long num2;
			char *end1;
			char *end2;
			num1 = strtoul(str1 + i1, &end1, 10);
			num2 = strtoul(str2 + i2, &end2, 10);
			if (num1 < num2)
				return -1;
			if (num1 > num2)
				return 1;
			i1 = end1 - str1 - 1;
			i2 = end2 - str2 - 1;
			if (i1 < i2)
				return -1;
			if (i1 > i2)
				return 1;
		} else {
			if (c1 < c2)
				return -1;
			if (c1 > c2)
				return 1;
		}
	}
	if (len1 < len2)
		return -1;
	if (len1 > len2)
		return 1;
	return 0;
}
