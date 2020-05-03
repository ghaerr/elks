extern int
tgetent(char *bp, char *name);

extern int 
tgetnum(char *id);

extern int
tgetflag(char *id);

extern char *
tgetstr(char *id, char **bp);

extern char *
tgoto(char *cm, int destcol, int destrow);

extern void
tputs(char *cp, int affcnt, int (*outfn)());
