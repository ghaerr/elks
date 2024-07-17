#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>

#include "host.h"
#include "basic.h"

unsigned char mem[MEMORY_SIZE];
static unsigned char tokenBuf[TOKEN_BUF_SIZE];

FILE *outfile;
static int intflag;
static struct termios def_termios;

void host_moveCursor(int x, int y) {
	fprintf(outfile, "\033[%d;%dH", y, x);
}

void host_showBuffer() {
	fflush(outfile);
}

void host_outputString(char *str) {
	fprintf(outfile, "%s", str);
}

void host_outputChar(char c) {
	fprintf(outfile, "%c", c);
}

void host_outputLong(long num) {
	fprintf(outfile, "%ld", num);
}

#if MATH_CORRECT_NEAR_ZERO
// experimental function to correct trig functions returning near zero values
// some hosts return -0 or +/-0.00000000000001
float adjust(float f) {
	if (FABS(f) < (float)EPSILON) {
		//printf("[%f,%f]", f, FABS(f));
		return 0;
	}
	return f;
}
#endif

// for float testing compatibility, use same FP formatting routines on host for now
// floats have approx 7 sig figs, 15 for double

#if __ia16__
#include <sys/linksym.h>
__STDIO_PRINT_FLOATS;       // link in libc printf float support

char *host_floatToStr(float f, char *buf) {
	sprintf(buf, "%.*g", MATH_PRECISION, (double)f);
    return buf;
}
#else
void dtostr(double val, int style, int preci, char *buf);
char *ecvt(double val, int ndig, int *pdecpt, int *psign);
char *fcvt(double val, int ndig, int *pdecpt, int *psign);

char *host_floatToStr(float f, char *buf) {

	dtostr(f, 'g', MATH_PRECISION, buf);
    return buf;
}
#endif

void host_outputFloat(float f) {
    char buf[32];
    host_outputString(host_floatToStr(f, buf));
}

void host_newLine() {
	fprintf(outfile, "\n");
}

static void tty_raw(void)
{
    struct termios termios;

    fflush(stdout);
    tcgetattr(0, &termios);
    //termios.c_iflag &= ~(ICRNL|IGNCR|INLCR);
    termios.c_lflag &= ~(ECHO|ECHOE|ECHONL|ICANON);
    termios.c_lflag |= ISIG;
    tcsetattr(0, TCSADRAIN, &termios);

    int nonblock = 1;
    ioctl(0, FIONBIO, &nonblock);
}

static void tty_isig(void)
{
    int nonblock = 0;
    struct termios termios;

    fflush(stdout);
    tcgetattr(0, &termios);
    //termios.c_iflag |= (ICRNL|IGNCR|INLCR);
    termios.c_lflag |= (ECHO|ECHOE|ECHONL|ICANON);
    termios.c_lflag |= ISIG;

    ioctl(0, FIONBIO, &nonblock);
    tcsetattr(0, TCSADRAIN, &termios);
}

static void tty_normal(void)
{
    int nonblock = 0;

    fflush(stdout);
    ioctl(0, FIONBIO, &nonblock);
    tcsetattr(0, TCSADRAIN, &def_termios);
}

static void catchint(int sig)
{
    intflag = 1;
    signal(SIGINT, catchint);
}

/* returning NULL will stop interpreter */
char *host_readLine() {
    static char buf[TOKEN_BUF_SIZE+1];

    tty_isig();
    while (!fgets(buf, sizeof(buf), stdin)) {
        if (ferror(stdin) && (errno == EINTR)) {
             clearerr(stdin);
             intflag = 0;
        }
        tty_raw();
        return NULL;
    }
    tty_raw();
    buf[strlen(buf)-1] = 0;
    return buf;
}

/* assumes stdin in unblocking raw mode, set before interpreter runs */
int host_getKey() {
    unsigned char buf[1];

    if (read(0, buf, 1) == 1) {
        if (buf[0] >= 32 && buf[0] <= 126)
            return buf[0];
    }
    return 0;
}

int host_breakPressed() {
    return intflag;
}

#if __ia16__
/* replacement fread to fix fgets not returning ferror/errno properly on SIGINT*/
size_t fread(void *buf, size_t size, size_t nelm, FILE *fp)
{
   int len, v;
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
   if (len >= bytes)            /* Enough buffered */
   {
      memcpy(buf, fp->bufpos, bytes);
      fp->bufpos += bytes;
      return nelm;
   }
   else if (len > 0)            /* Some buffered */
   {
      memcpy(buf, fp->bufpos, len);
      fp->bufpos += len;
      got = len;
   }

   /* Need more; do it with a direct read */
   len = read(fp->fd, (char *)buf + got, bytes - got);
   /* Possibly for now _or_ later */
#if 1   /* Fixes stdio when SIGINT received*/
   if (intflag) {
      len = -1;
      errno = EINTR;
   }
#endif
   if (len < 0)
   {
      fp->mode |= __MODE_ERR;
      len = 0;
   }
   else if (len == 0)
      fp->mode |= __MODE_EOF;

   return (got + len) / size;
}
#endif

void host_outputFreeMem(unsigned int val)
{
	printf("%u bytes free\n", val);
}

/* LONG_MAX is not exact as a double, yet LONG_MAX + 1 is */
#define LONG_MAX_P1 ((LONG_MAX/2 + 1) * 2.0)

/* small ELKS floor function using 32-bit longs, required for INT() fn */
float host_floor(float x)
{
  if (x >= (float)0.0) {
    if (x < LONG_MAX_P1) {
      return (float)(long)x;
    }
    return x;
  } else if (x < (float)0.0) {
    if (x >= LONG_MIN) {
      long ix = (long) x;
      return (ix == x) ? x : (float)(ix-1);
    }
    return x;
  }
  return x; // NAN
}

void host_sleep(long ms) {
    usleep(ms * 1000);
}

#if DISK_FUNCTIONS

#include <dirent.h>

int host_directoryListing() {
	DIR		*dirp;
	struct	dirent	*dp;

    dirp = opendir(".");
    if (dirp == NULL)
		return ERROR_FILE_ERROR;

    while ((dp = readdir(dirp)) != NULL) {
        char *dot = strrchr(dp->d_name, '.'); /* Find last '.', if there is one */
        if (dot && (strcasecmp(dot, ".bas") == 0)) {
            host_outputString(dp->d_name);
            host_newLine();
        }
    }
    closedir(dirp);

	return ERROR_NONE;
}

int host_removeFile(char *fileName) {
	char file[MAX_PATH_LEN+5];

	strcpy(file, fileName);
	if (!strstr(file, ".bas"))
		strcat(file, ".bas");

    if (unlink(file) < 0)
		return ERROR_FILE_ERROR;
    return ERROR_NONE;
}

int host_saveProgramToFile(char *fileName, int autoexec) {
	char file[MAX_PATH_LEN+5];

	strcpy(file, fileName);
	if (!strstr(file, ".bas"))
		strcat(file, ".bas");
	FILE* fh = fopen(file, "w");

	if (!fh)
		return ERROR_FILE_ERROR;

	outfile = fh;
	listProg(0, 0);
	if (autoexec)
		fprintf(outfile, "RUN\n");
	fclose(outfile);
	sync();
	outfile = stdout;

    return ERROR_NONE;
}
#endif

static char *file_readLine(FILE *fp, char *buf, int size) {
	if (!fgets(buf, size, fp))
		return NULL;
	buf[strlen(buf)-1] = 0;
	return buf;
}

// BASIC

static int loop(FILE *infile) {
    int ret = ERROR_NONE;
    char buf[TOKEN_BUF_SIZE+1];

    lineNumber = 0;
    intflag = 0;
    char *input = infile? file_readLine(infile, buf, sizeof(buf)): host_readLine();
    if (!input) return infile || feof(stdin)? ERROR_EOF: ERROR_BREAK_PRESSED;

    if (!strcasecmp(input, "mem")) {
        host_outputFreeMem(sysVARSTART - sysPROGEND);
        return ERROR_NONE;
    }
    ret = tokenize((unsigned char*)input, tokenBuf, TOKEN_BUF_SIZE);

    /* execute the token buffer */
    if (ret == ERROR_NONE) {
        intflag = 0;
        if (!infile) tty_raw();
        ret = processInput(tokenBuf);
    }

    if (ret != ERROR_NONE) {
        host_newLine();
        if (lineNumber != 0) {
            host_outputLong(lineNumber);
            host_outputChar('-');
        }
        printf("%s\n\n", errorTable[ret]);
    } else if (!infile)
        printf("Ok\n\n");
    return ret;
}

int host_loadProgramFromFile(char *fileName) {
    FILE *fp;
	int err;
	char file[MAX_PATH_LEN+5];

	strcpy(file, fileName);
	if (!strstr(file, ".bas"))
		strcat(file, ".bas");

	fp = fopen(file, "r");
	if (!fp)
		return ERROR_FILE_ERROR;

	while ((err = loop(fp)) == ERROR_NONE)
		continue;

	fclose(fp);
	if (err == ERROR_EOF) err = ERROR_NONE;
	return err;
}

int host_peekb(int offset, int segment) {
#if __ia16__
    unsigned char __far *peek;
    peek = _MK_FP(segment,offset);
    return (int) *peek;
#else
    return 0;
#endif
}

void host_pokeb(int offset, int segment, int value) {
#if __ia16__
    unsigned char __far *poke;
    poke = _MK_FP(segment,offset);
    *poke = (unsigned char) value;
#endif
}

int main(int ac, char **av) {
	outfile = stdout;

	tcgetattr(0, &def_termios);
	tty_isig();
	signal(SIGINT, catchint);
	reset();

	printf("ELKS BASIC\n");
	host_outputFreeMem(sysVARSTART - sysPROGEND);
	printf("\n");

	if (ac > 1) {
		int err = host_loadProgramFromFile(av[1]);
		if (err != ERROR_NONE) {
			printf("Can't load %s\n", av[1]);
			exit(1);
		} else printf("Ok\n\n");
	}
    
	for(;;)
		if (loop(NULL) == ERROR_EOF)
			break;
	tty_normal();
	return 0;
}
