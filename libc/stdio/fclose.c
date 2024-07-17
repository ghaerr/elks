#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

int
fclose (FILE *fp)
{
    int   rv = 0;

    if (fp == 0)
    {
        errno = EINVAL;
        return EOF;
    }
    if (fp->fd != -1)
    {
        if (fflush(fp))
            rv = EOF;

        if (close(fp->fd))
            rv = EOF;
        fp->fd = -1;
    }

    if (fp->mode & __MODE_FREEBUF)
    {
        free(fp->bufstart);
        fp->mode &= ~__MODE_FREEBUF;
        fp->bufstart = fp->bufend = 0;
    }

    if (fp->mode & __MODE_FREEFIL)
    {
        FILE *prev = 0, *ptr;
        fp->mode = 0;

        for (ptr = __IO_list; ptr && ptr != fp; ptr = ptr->next)
            ;
        if (ptr == fp)
        {
            if (prev == 0)
                __IO_list = fp->next;
            else
                prev->next = fp->next;
        }
        free(fp);
    }
    else
        fp->mode = 0;

    return rv;
}
