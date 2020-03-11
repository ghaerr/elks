/*
 * (C) Shane Kerr <kerr@wizard.net> under terms of LGPL
 */

#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#ifndef P_tmpdir
#define P_tmpdir "/tmp"
#endif 

#ifndef L_tmpnam
#define L_tmpnam 20
#endif 

char *tmpnam(s)
char *s;
{
    static char ret_val[L_tmpnam];
    static char c1 = 0;
    static char c2 = 0;
    static char c3 = 0;
    static char uniq_ch[] = 
	 "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    struct stat stbuf;

    do {
        sprintf(ret_val, "%s/%05d%c%c%c", P_tmpdir, getpid(), 
	     uniq_ch[c1], uniq_ch[c2], uniq_ch[c3]);
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

