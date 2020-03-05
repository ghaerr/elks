/*
 * (C) Shane Kerr <kerr@wizard.net> under terms of LGPL
 */

#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#define namlen

char *mktemp(s)
char *s;
{
    char * ptr;
    static char c1 = 0;
    static char c2 = 0;
    static char uniq_ch[] = 
	 "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    struct stat stbuf;

    
    if (!s || strlen(s) < 6) {
        errno = EINVAL;
        return 0;
    }
    ptr = s + strlen(s) - 6;
    if (++c1 >= 62) {
	c1 = 0;
	if (++c2 >= 62) {
	    errno = EEXIST;
	    return 0;
	}
    }
    sprintf(ptr, "%04d%c%c", getpid(), uniq_ch[c1], uniq_ch[c2]);

    return s;
}

