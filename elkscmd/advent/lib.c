/*
 *	Stdio avoidance library
 */

#include "advent.h"

void nl(void)
{
	write(1, "\n", 1);
}

void writes(const char *p)
{
	write(1, p, strlen(p));
}

#if ELKS
/*
 * ELKS ICANON returns immediately on single byte reads
 * with the result that backspace doesn't work. FIXME
 */
void getinp(char *p, int l)
{
    int e;
    e = read(0, p, l);
    if (e <= 0) exit(1);
    if (p[e-1] == '\n')
        p[e-1] = 0;
}
#else
void getinp(char *p, int l)
{
	char *ep = p + l - 1;
	int e;
	while ((e = read(0, p, 1)) == 1) {
		if (*p == '\n') {
			*p = 0;
			return;
		}
		if (p < ep)
			p++;
	}
	if (e || !isatty(0))
		exit(1);
	*p = 0;
	return;
}
#endif

void writei(uint16_t v)
{
#if defined(__linux__) || defined(__APPLE__)
	char buf[16];
	sprintf(buf, "%d", (unsigned int) v);
	writes(buf);
#else
	writes(itoa(v));
#endif
}
