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
 * Return the standard ls-like mode string from a file mode.
 * This is static and so is overwritten on each call.
 */
char *modestring(int mode)
{
    static char buf[12];

    strcpy(buf, "----------");

/* Fill in the file type */
    if (S_ISDIR(mode)) buf[0] = 'd';
    if (S_ISCHR(mode)) buf[0] = 'c';
    if (S_ISBLK(mode)) buf[0] = 'b';
    if (S_ISFIFO(mode)) buf[0] = 'p';
#ifdef S_ISLNK
    if (S_ISLNK(mode)) buf[0] = 'l';
#endif
#ifdef S_ISSOCK
    if (S_ISSOCK(mode)) buf[0] = 's';
#endif

/* Now fill in the normal file permissions */
    if (mode & S_IRUSR) buf[1] = 'r';
    if (mode & S_IWUSR) buf[2] = 'w';
    if (mode & S_IXUSR) buf[3] = 'x';
    if (mode & S_IRGRP) buf[4] = 'r';
    if (mode & S_IWGRP) buf[5] = 'w';
    if (mode & S_IXGRP) buf[6] = 'x';
    if (mode & S_IROTH) buf[7] = 'r';
    if (mode & S_IWOTH) buf[8] = 'w';
    if (mode & S_IXOTH) buf[9] = 'x';

/* Finally fill in magic stuff like suid and sticky text */
    if (mode & S_ISUID) buf[3] = ((mode & S_IXUSR) ? 's' : 'S');
    if (mode & S_ISGID) buf[6] = ((mode & S_IXGRP) ? 's' : 'S');
    if (mode & S_ISVTX) buf[9] = ((mode & S_IXOTH) ? 't' : 'T');

    return buf;
}

