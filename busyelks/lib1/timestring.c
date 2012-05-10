#include "../sash.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <string.h>
#include <grp.h>
#include <time.h>

#define BUF_SIZE 1024

/*
 * Get the time to be used for a file.
 * This is down to the minute for new files, but only the date for old files.
 * The string is returned from a static buffer, and so is overwritten for
 * each call.
 */
char *
timestring(t)
long t;
{
    long  now;
    char  *str;
    static char buf[26];

    time(&now);

    str = ctime(&t);

    strcpy(buf, &str[4]);
    buf[12] = '\0';

    if ((t > now) || (t < now - 180*24*60*60L)) {
	strcpy(&buf[7], &str[20]);
	buf[11] = '\0';
    }

    return buf;
}

