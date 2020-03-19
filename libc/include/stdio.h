
#ifndef __STDIO_H
#define __STDIO_H

#include <features.h>
#include <sys/types.h>
#include <stdarg.h>

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#define _IOFBF		0x00	/* full buffering */
#define _IOLBF		0x01	/* line buffering */
#define _IONBF		0x02	/* no buffering */
#define __MODE_BUF	0x03	/* Modal buffering dependent on isatty */

#define __MODE_FREEBUF	0x04	/* Buffer allocated with malloc, can free */
#define __MODE_FREEFIL	0x08	/* FILE allocated with malloc, can free */

#define __MODE_READ	0x10	/* Opened in read only */
#define __MODE_WRITE	0x20	/* Opened in write only */
#define __MODE_RDWR	0x30	/* Opened in read/write */

#define __MODE_READING	0x40	/* Buffer has pending read data */
#define __MODE_WRITING	0x80	/* Buffer has pending write data */

#define __MODE_EOF	0x100	/* EOF status */
#define __MODE_ERR	0x200	/* Error status */
#define __MODE_UNGOT	0x400	/* Buffer has been polluted by ungetc */

#ifdef __MSDOS__
#define __MODE_IOTRAN	0x1000	/* MSDOS nl <-> cr,nl translation */
#else
#define __MODE_IOTRAN	0
#endif

/* when you add or change fields here, be sure to change the initialization
 * in stdio_init and fopen */
struct __stdio_file {
  unsigned char *bufpos;   /* the next byte to write to or read from */
  unsigned char *bufread;  /* the end of data returned by last read() */
  unsigned char *bufwrite; /* highest address writable by macro */
  unsigned char *bufstart; /* the start of the buffer */
  unsigned char *bufend;   /* the end of the buffer; ie the byte after the last
                              malloc()ed byte */

  int fd; /* the file descriptor associated with the stream */
  int mode;

  unsigned char unbuf[8];  /* The buffer for 'unbuffered' streams */

  struct __stdio_file * next;
};

#define EOF	(-1)
#ifndef NULL
#define NULL	((void*)0)
#endif

typedef struct __stdio_file FILE;

#ifdef __AS386_16__
#define BUFSIZ	(256)
#else
#define BUFSIZ	(2048)
#endif

extern FILE stdin[1];
extern FILE stdout[1];
extern FILE stderr[1];

#ifdef __MSDOS__
#define putc(c, fp) fputc(c, fp)
#define getc(fp) fgetc(fp)
#else
#define putc(c, stream)	\
    (((stream)->bufpos >= (stream)->bufwrite) ? fputc((c), (stream))	\
                          : (unsigned char) (*(stream)->bufpos++ = (c))	)

#define getc(stream)	\
  (((stream)->bufpos >= (stream)->bufread) ? fgetc(stream):		\
    (*(stream)->bufpos++))
#endif

#define putchar(c) putc((c), stdout)  
#define getchar() getc(stdin)

#define ferror(fp)	(((fp)->mode&__MODE_ERR) != 0)
#define feof(fp)   	(((fp)->mode&__MODE_EOF) != 0)
#define clearerr(fp)	((fp)->mode &= ~(__MODE_EOF|__MODE_ERR))
#define fileno(fp)	((fp)->fd)

/* declare functions; not like it makes much difference without ANSI */
/* RDB: The return values _are_ important, especially if we ever use
        8086 'large' model
 */

/* These two call malloc */
#define setlinebuf(__fp)             setvbuf((__fp), (char*)0, _IOLBF, 0)
extern int setvbuf __P((FILE*, char*, int, size_t));

/* These don't */
#define setbuf(__fp, __buf) setbuffer((__fp), (__buf), BUFSIZ)
extern void setbuffer __P((FILE*, char*, int));

extern int fgetc __P((FILE*));
extern int fputc __P((int, FILE*));

extern int fclose __P((FILE*));
extern int fflush __P((FILE*));
extern char *fgets __P((char*, size_t, FILE*));

extern FILE *fopen __P((char*, char*));
extern FILE *fdopen __P((int, char*));
extern FILE *freopen  __P((char*, char*, FILE*));

#ifdef __LIBC__
extern FILE *__fopen __P((char*, int, FILE*, char*));
#endif

extern int fputs __P((const char*, FILE*));
int puts (const char * s);

size_t fread __P ((void *, size_t, size_t, FILE *));
int fseek(FILE *fp, long offset, int ref);
long ftell(FILE *fp);
int fwrite(char *buf, int size, int nelm, FILE *fp);
char * gets(char *str);	/* BAD function; DON'T use it! */

int fscanf(FILE * fp, const char * fmt, ...);

extern int printf __P ((__const char*, ...));
extern int fprintf __P ((FILE*, __const char*, ...));
extern int sprintf __P ((char*, __const char*, ...));

int vfprintf (FILE * stream, const char * format, va_list ap);
int vsprintf (char * sp, const char * format, va_list ap);

#define stdio_pending(fp) ((fp)->bufread>(fp)->bufpos)

void perror (const char * s);
int rename(const char *old, const char *new);

int scanf (const char * format, ...);
int sscanf (const char * str, const char * format, ...);

int ungetc (int c, FILE *stream);
int vfscanf(register FILE *fp, register char *fmt, va_list ap);
int vscanf(__const char *fmt, va_list ap);
int vsscanf(char *sp, __const char *fmt, va_list ap);

#endif /* __STDIO_H */
