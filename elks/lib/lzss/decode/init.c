#include <lzss.h>

void
lzss_decode_init(struct lzss_decode *l)
{
	l->mask = 0;
}
