#include <lzss.h>

int
lzss_getbit(struct lzss_decode *l, int (*get)(void *), void *gd, int n)
{
	int i, x;

	x = 0;
	for (i = 0; i < n; i++)
	{
		if (l->mask == 0)
		{
			if ((l->buf = get(gd)) < 0)
				return -1;

			l->mask = 128;
		}

		x <<= 1;

		if (l->buf & l->mask)
			x++;

		l->mask >>= 1;
	}

	return x;
}
