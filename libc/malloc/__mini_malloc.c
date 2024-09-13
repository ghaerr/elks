#include <errno.h>
#include <malloc.h>
#include <unistd.h>

#include "_malloc.h"

void __wcnear *
__mini_malloc(size_t size)
{
	mem __wcnear *ptr;
	size_t sz;

	/* First time round this _might_ be odd, But we won't do that! */
	sz = (size_t)sbrk(0);
	if(sz & (sizeof(mem) - 1))
		sbrk(4 - (sz & (sizeof(mem) - 1)));

	/* Minor oops here, sbrk has a signed argument */
	if((int)size <= 0 || size > (((unsigned)-1) >> 1) - sizeof(mem) * 3)
	{
		errno = ENOMEM;
		return 0;
	}

	size += sizeof(mem) * 2 - 1;	/* Round up and leave space for size field */
	size /= sizeof(mem);

	ptr = (mem __wcnear *) sbrk(size * sizeof(mem));
	/*if((uintptr_t)ptr == (intptr_t)-1)*/  /* this is better only when not __wcnear */
    if ((int)ptr == -1)
		return 0;

	m_size(ptr) = size;
	__noise("CREATE", ptr);
	return ptr + 1;
}
