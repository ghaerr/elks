#ifndef _T_H
#define	_T_H

struct termcap_buffer
{
	char *	beg;
	int	size;
	char	*ptr;
	int	ateof;
	int	full;
};

const char *termcap_find_capability (const char *bp, const char *cap);
void termcap_memory_out(void);
char * termcap_tgetst1(const char *ptr, char **area);
char * termcap_xmalloc(unsigned size);
char * termcap_xrealloc(char * ptr, unsigned size);
char * termcap_tparam1(const char *string, char *outstring, int len, char *up,
    char *left, int *argp);

extern	char *termcap_term_entry;

#endif
