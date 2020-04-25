#include <lzss.h>

void
lzss_decode_open(struct lzss_decode_stream *f)
{
	int i;

	for (i = 0; i < lzss_N - lzss_F; i++)
		f->buffer[i] = (unsigned char)' ';

	f->r = lzss_N - lzss_F;
	f->j = 0;
	f->k = 2;
}

int
lzss_decode_get(struct lzss_decode_stream *f,
	struct lzss_decode *l,
	int (*get)(void *), void *gd)
{
	int c;

	for(;;)
	{
		if(f->k <= f->j + 1)
		{
			int c;

			c = f->buffer[(f->i + f->k) & (lzss_N - 1)];
			f->buffer[f->r++] = c;
			f->r &= (lzss_N - 1);
			f->k++;
			return c;
		}

		c = lzss_getbit(l, get, gd, 1);
		if(c < 0)
			return lzss_EOF;

		if(c)
		{
			c = lzss_getbit(l, get, gd, 8);
			if(c < 0)
				return lzss_EOF;

			f->buffer[f->r++] = c;
			f->r &= (lzss_N - 1);

			return c;
		}

		if ((f->i = lzss_getbit(l, get, gd, lzss_EI)) < 0)
			return lzss_EOF;

		if ((f->j = lzss_getbit(l, get, gd, lzss_EJ)) < 0)
			return lzss_EOF;

		f->k = 0;
	}
}
