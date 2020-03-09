#include "lib.h"

#define	ptr(base, size, i)	((const void *)(((unsigned char *)(base)) + ((i) * (size))))

void *
bsearch(const void * key, const void * base, size_t nmemb, size_t size,
	int (* compar)(const void *, const void *))
{
	if(nmemb)
	{
		size_t i = nmemb >> 1;
		void const * p = ptr(base, size, i);
		int	k = compar(key, p);

		if(k)
		{
			if(k < 0)
				return bsearch(key, base, i, size, compar);
			else
				return bsearch(key, ((unsigned char *)p) + size, nmemb - (i + 1), size, compar);
		}

		return (void *)p;
	}

	return NULL;
}
