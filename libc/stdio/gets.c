#include <stdio.h>

char *
gets(char *str)			/* BAD function; DON'T use it! */
{
   /* Auwlright it will work but of course _your_ program will crash */
   /* if it's given a too long line */
   register char *p = str;
   register int c;

   while (((c = getc(stdin)) != EOF) && (c != '\n'))
      *p++ = c;
   *p = '\0';
   return (((c == EOF) && (p == str)) ? NULL : str);	/* NULL == EOF */
}
