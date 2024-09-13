#include <errno.h>
#include <string.h>
#include <unistd.h>

int
execve(const char *fname, char **argv, char **envp)
{
	char **p;
	char **q;
	int argv_len=0, argv_count=0;
	int envp_len=0, envp_count=0;
	int stack_bytes;
	unsigned short * pip;
	char * pcp, * stk_ptr, *baseoff;
	int rv;

	/* How much space for argv */
	for(p=argv; p && *p && argv_len >= 0; p++)
	{
	   argv_count++; argv_len += strlen(*p)+1;
	}

	/* How much space for envp */
	for(q=envp; q && *q && envp_len >= 0; q++)
	{
	   envp_count++; envp_len += strlen(*q)+1;
	}

	/* tot it all up */
	stack_bytes = 2				/* argc */
	            + argv_count * 2 + 2	/* argv */
		    + argv_len
		    + envp_count * 2 + 2	/* envp */
		    + envp_len;

	/* Allocate it */
	if( argv_len < 0 || envp_len < 0 || stack_bytes <= 0
	 || (int)(stk_ptr = (char*)sbrk(stack_bytes)) == -1)
	{
	   errno = ENOMEM;
	   return -1;
	}

/* Sanity check 
	printf("Argv = (%d,%d), Envp=(%d,%d), stack=%d\n",
	        argv_count, argv_len, envp_count, envp_len, stack_bytes);
*/

	/* Now copy in the strings */
	pip=(unsigned short *) stk_ptr;
	pcp=stk_ptr+2*(1+argv_count+1+envp_count+1);

	/* baseoff = stk_ptr + stack_bytes; */
	baseoff = stk_ptr;
	*pip++ = argv_count;
	for(p=argv; p && *p; p++)
	{
	   int l;
	   *pip++ = pcp-baseoff;
	   l = strlen(*p)+1;
	   memcpy(pcp, *p, l);
	   pcp += l;
	}
	*pip++ = 0;

	for(q=envp; q && *q; q++)
	{
	   int l;
	   *pip++ = pcp-baseoff;
	   l = strlen(*q)+1;
	   memcpy(pcp, *q, l);
	   pcp += l;
	}
	*pip++ = 0;

	rv = _execve(fname, stk_ptr, stack_bytes);
	/* FIXME: This will probably have to interpret '#!' style exe's */
	sbrk(-stack_bytes);
	return rv;
}
