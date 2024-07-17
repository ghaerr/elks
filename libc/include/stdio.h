#ifndef __STDIO_H
#define __STDIO_H

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

#define BUFSIZ	(1024)

extern FILE stdin[1];
extern FILE stdout[1];
extern FILE stderr[1];

#define putc(c, stream)	\
    (((stream)->bufpos >= (stream)->bufwrite) ? fputc((c), (stream))	\
                          : (unsigned char) (*(stream)->bufpos++ = (c))	)

#define getc(stream)	\
  (((stream)->bufpos >= (stream)->bufread) ? fgetc(stream):		\
    (*(stream)->bufpos++))

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
int setvbuf(FILE*, char*, int, size_t);

/* These don't */
#define setbuf(__fp, __buf) setbuffer((__fp), (__buf), BUFSIZ)
void setbuffer(FILE*, char*, int);

int fgetc(FILE*);
int fputc(int, FILE*);

int fclose(FILE*);
int fflush(FILE*);
char *fgets(char*, size_t, FILE*);

ssize_t getdelim(char ** restrict lineptr, size_t * restrict n,
		int delimiter, register FILE * restrict stream);
ssize_t getline(char ** restrict lineptr, size_t * restrict n,
		FILE * restrict stream);

FILE *fopen(const char*, const char*);
FILE *fdopen(int, const char*);
FILE *freopen(const char*, const char*, FILE*);

#ifdef __LIBC__
FILE *__fopen(const char*, int, FILE*, const char*);
void __stdio_init(void);        /* fwd decl for OWC __LINK_SYMBOL() */
extern FILE *__IO_list;
#endif

int fputs(const char*, FILE*);
int puts (const char * s);

size_t fread(void *, size_t, size_t, FILE *);
int fseek(FILE *fp, long offset, int ref);
long ftell(FILE *fp);
void rewind(FILE *fp);
int fwrite(const char *buf, int size, int nelm, FILE *fp);
char * gets(char *str);	/* BAD function; DON'T use it! */

int fscanf(FILE * fp, const char * fmt, ...);

int printf(const char*, ...);
int fprintf(FILE*, const char*, ...);
int sprintf(char*, const char*, ...);
int snprintf(char*, size_t, const char*, ...);

int vfprintf (FILE * stream, const char * format, va_list ap);
int vsprintf (char * sp, const char * format, va_list ap);
int vsnprintf (char * sp, size_t, const char * format, va_list ap);

#ifndef __HAS_NO_FLOATS__
void dtostr(double val, int style, int preci, char *buf);
/* use this macro to link in libc %e,%f,%g printf/sprintf support into user program */
#define __STDIO_PRINT_FLOATS    __LINK_SYMBOL(dtostr)
#endif

#define stdio_pending(fp) ((fp)->bufread>(fp)->bufpos)

void perror (const char * s);
int rename(const char *old, const char *new);

char *tmpnam (char *s);

int scanf (const char * format, ...);
int sscanf (const char * str, const char * format, ...);

int ungetc (int c, FILE *stream);
int vfscanf(register FILE *fp, register char *fmt, va_list ap);
int vscanf(const char *fmt, va_list ap);
int vsscanf(char *sp, const char *fmt, va_list ap);

FILE *popen(const char *, const char *);
int pclose(FILE *);

#endif /* __STDIO_H */
