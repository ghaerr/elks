/*
 * (C) Shane Kerr <kerr@wizard.net> under terms of LGPL
 */

#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <paths.h>

#ifndef P_tmpdir
#define P_tmpdir _PATH_TMP
#endif

#ifndef L_tmpnam
#define L_tmpnam 20
#endif

char *tmpnam(char *s)
{
    static char ret_val[L_tmpnam];
    static char c1 = 0;
    static char c2 = 0;
    static char c3 = 0;
    static char uniq_ch[] =
	 "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    struct stat stbuf;
    pid_t pid = getpid();

    do {
        sprintf(ret_val, "%s/%05d%c%c%c", P_tmpdir, pid,
	     uniq_ch[(int)c1], uniq_ch[(int)c2], uniq_ch[(int)c3]);
        if (++c1 >= 62) {
  	    c1 = 0;
	    if (++c2 >= 62) {
	        c2 = 0;
	        if (++c3 >= 62) {
		    errno = EEXIST;
		    return 0;
	        }
	    }
        }
    } while (stat(ret_val, &stbuf) == 0);

    if (s != 0) {
	strcpy(s, ret_val);
	return s;
    } else {
	return ret_val;
    }
}

