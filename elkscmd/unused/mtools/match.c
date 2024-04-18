/*
 * Do shell-style pattern matching for '?', '\', '[..]', and '*' wildcards.
 * Not idiot proof!  Returns 1 if match, 0 if not.
 *
 * Ideas for this routine were taken from a program by Rich Salz, mirror!rs
 */

#include <stdio.h>

int
match(s, p)
char *s;				/* string to match */
char *p;				/* pattern */
{
	int matched, reverse;
	char first, last;

	for ( ; *p != '\0'; s++, p++) {
		switch (*p) {
					/* Literal match with next character */
		case '\\':
			p++;
		default:
			if (*s != *p)
				return(0);
			break;
					/* match any one character */
		case '?':
			if (*s == '\0')
				return(0);
			break;
					/* match everything */
		case '*':
					/* if last char in pattern */
			if (*++p == '\0')
				return(1);
					/* search for next char in pattern */
			matched = 0;
			while (*s != '\0') {
				if (*s == *p) {
					matched = 1;
					if (!strcmp(s+1,p+1))
						break;
				}
				s++;
			}
			if (!matched)
				return(0);
			s--;
			p--;
			break;
					/* match range of characters */
		case '[':
			first = '\0';
			matched = 0;
			reverse = 0;
			while (*++p != ']') {
				if (*p == '^') {
					reverse = 1;
					p++;
				}
				first = *p;
				if (first == ']' || first == '\0') {
					fprintf(stderr, "match: Malformed regular expression\n");
					return(0);
				}
					/* if 2nd char is '-' */
				if (*(p+1) == '-') {
					p++;
					/* set last to 3rd char ... */
					last = *++p;
					if (last == ']' || last == '\0') {
						fprintf(stderr, "match: Malformed regular expression\n");
						return(0);
					}
					/* test the range of values */
					if (*s>=first && *s<=last) {
						matched = 1;
						p++;
						break;
					}
					return(0);
				}
				if (*s == *p)
					matched = 1;
			}
			if (matched && reverse)
				return(0);
			if (!matched)
				return(0);
			break;
		}
	}
					/* string ended prematurely ? */
	if (*s != '\0')
		return(0);
	else
		return(1);
}
