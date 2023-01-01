#ifndef _TERMCAP_H
#define _TERMCAP_H

#include <features.h>
#include <sys/types.h>

extern char PC;
extern char *UP;
extern char *BC;
extern int ospeed;

int tgetent(char *, const char *);
int tgetflag(const char *);
int tgetnum(const char *);
char *tgetstr(const char *, char **);

int tputs(const char *, int, int (*)(int));
char *tgoto(const char *, int, int);

#endif /* _TERMCAP_H */
