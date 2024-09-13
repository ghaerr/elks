#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * fread will often be used to read in large chunks of data calling read()
 * directly can be a big win in this case. Beware also fgetc calls this
 * function to fill the buffer.
 * 
 * This ignores __MODE__IOTRAN; probably exactly what you want. (It _is_ what
 * fgetc wants)
 */
size_t fread(void *buf, size_t size, size_t nelm, FILE *fp)
{
    int v;
    ssize_t len;
    size_t bytes, got = 0;

    v = fp->mode;

    /* Want to do this to bring the file pointer up to date */
    if (v & __MODE_WRITING)
        fflush(fp);

    /* Can't read or there's been an EOF or error then return zero */
    if ((v & (__MODE_READ | __MODE_EOF | __MODE_ERR)) != __MODE_READ)
        return 0;

    /* This could be long, doesn't seem much point tho */
    bytes = size * nelm;

    len = fp->bufread - fp->bufpos;
    if ((size_t)len >= bytes) {
        /* Enough buffered */
        memcpy(buf, fp->bufpos, bytes);
        fp->bufpos += bytes;
        return nelm;
    } else if (len > 0) {
        /* Some buffered */
        memcpy(buf, fp->bufpos, len);
        fp->bufpos += len;
        got = len;
    }

    /* Need more; do it with a direct read */
    len = read(fp->fd, (char *)buf + got, bytes - got);
    /* Possibly for now _or_ later */
    if (len < 0) {
        fp->mode |= __MODE_ERR;
        len = 0;
    } else if (len == 0)
        fp->mode |= __MODE_EOF;

    return (got + len) / size;
}
