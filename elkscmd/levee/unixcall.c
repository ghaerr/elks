/*
 * LEVEE, or Captain Video;  A vi clone
 *
 * Copyright (c) 1982-1997 David L Parsons
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by David L Parsons (orc@pell.chi.il.us).  My name may not be used
 * to endorse or promote products derived from this software without
 * specific prior written permission.  THIS SOFTWARE IS PROVIDED
 * AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * Unix interface for levee
 */
#include "levee.h"
#include "extern.h"
#include <termios.h>
#ifdef GCC
#include <ioctls.h>
#endif
#include <string.h>

int
min(a,b)
int a, b;
{
    return (a>b) ? b : a;
}

int
max(a,b)
int a, b;
{
    return (a<b) ? b : a;
}

strput(s)
register char *s;
{
    if (s)
	write(1, s, strlen(s));
}


#ifndef GCC
char *
basename(s)
char *s;
{
    char *rindex();
    register char *p;

    if (p=strrchr(s,'/'))
	return 1+p;
    return s;
}
#endif


static int ioset = 0;
static struct termios old;

initcon()
{
    struct termios new;

    if (!ioset) {
	tcgetattr(0, &old);
        /*ioctl(0, TCGETS, &old);*/	/* get editing keys */

        erasechar = old.c_cc[VERASE];
        eraseline = old.c_cc[VKILL];

        new = old;

	new.c_iflag &= ~(IXON|IXOFF|IXANY|ICRNL|INLCR);
	new.c_lflag &= ~(ICANON|ISIG|ECHO);
	new.c_oflag = 0;

	tcsetattr(0, TCSANOW, &new);
        /*ioctl(0, TCSETS, &new);*/
        ioset=1;
    }
}

fixcon()
{
    if (ioset) {
	 tcsetattr(0, TCSANOW, &old);
         /*ioctl(0, TCSETS, &old);*/
/*   More or less blind attempt to fix console corruption.
     T. Huld 1998-05-19
         ioctl(0, TCSETA, &old);
*/
         ioset = 0;
    }
}

getKey()
{
    unsigned char c;

    read(0,&c,1);
    return c;
}
