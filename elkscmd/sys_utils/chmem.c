/* chmem - set total memory size for execution	Author: Andy Tanenbaum */
/* from original Minix 1 source in "Operating Systems: Design and Implentation", 1st ed.*/
/* ported to ELKS by Greg Haerr*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#define HLONG            8	/* header size in longs */
#define TEXT             2	/* where is text size in header */
#define DATA             3	/* where is data size in header */
#define BSS              4	/* where is bss size in header */
#define TOT              6	/* where in header is total allocation */
#define TOTPOS          24	/* where is total in header */
#define SEPBIT   0x00200000	/* this bit is set for separate I/D */
#define MAGIC       0x0301	/* magic number for executable progs */
#define MAX         65520L	/* maximum allocation size */

/* Print an error message and die*/
static void fatalmsg(const char *s,...)
{
	va_list p;
	va_start(p,s);
	vfprintf(stderr,s,p);
	va_end(p);
	putc('\n',stderr);
	exit(-1);
}

static void usage(void)
{
	fatalmsg("Usage: %s {=+-}<# bytes dynamic data> <executable>\n");
}

int main(int argc, char **argv)
{
/* The 8088 architecture does not make it possible to catch stacks that grow
 * big.  The only way to deal with this problem is to let the stack grow down
 * towards the data segment and the data segment grow up towards the stack.
 * Normally, a total of 64K is allocated for the two of them, but if the
 * programmer knows that a smaller amount is sufficient, he can change it
 * using chmem.
 *
 * chmem =4096 prog  sets the total space for stack + data growth to 4096
 * chmem +200  prog  increments the total space for stack + data growth by 200
 */


  char *p;
  int fd, separate;
  unsigned long lsize, olddynam, newdynam = 0, newtot, overflow, dsegsize;
  unsigned long header[HLONG];

  p = argv[1];
  if (argc != 3) 
	fatalmsg("Usage: %s {=+-}<# bytes dynamic data> <executable>\n\
       or\n       %s -l <executable> to list the header\n", argv[0], argv[0]);
  if (*p != '=' && *p != '+' && *p != '-') usage();
  lsize = atol(p+1);
  if (lsize > MAX) fatalmsg("chmem: %lu too large, max %lu\n", lsize, MAX);

  fd = open(argv[2], 2);
  if (fd < 0) fatalmsg("chmem: can't open ", argv[2], "\n");

  if (read(fd, header, sizeof(header)) != sizeof(header))
	fatalmsg("chmem: ", argv[2], "bad header\n");
  if ( (header[0] & 0xFFFF) != MAGIC)
	fatalmsg("chmem: ", argv[2], " not executable\n");
  separate = (header[0] & SEPBIT ? 1 : 0);
  dsegsize = header[DATA] + header[BSS];
  if (header[TOT] == 0)		/* handle ELKS default INIT_STACK case*/
	olddynam = 0;
  else olddynam = header[TOT] - dsegsize;
  if (separate == 0) olddynam -= header[TEXT];
  
  if (*(p+1)=='l') {
        printf("Header of '%s': TEXT %lu DATA %lu BSS %ld TOT %lu DYNMEM %lu [TOT-(DATA-BSS)]\n",
	    argv[2], header[TEXT], header[DATA], header[BSS], header[TOT], olddynam);
        exit(0);
    }

  printf("Old %s: DATA %lu BSS %ld TOT %lu DYNMEM %lu\n",
	argv[2], header[DATA], header[BSS], header[TOT], olddynam);

  if (*p == '=') newdynam = lsize;
  else if (*p == '+') newdynam = olddynam + lsize;
  else if (*p == '-') newdynam = olddynam - lsize;
  newtot = dsegsize + newdynam;
  overflow = (newtot > MAX ? newtot - MAX : 0);	/* 64K max */
  newdynam -= overflow;
  newtot -= overflow;

  if (newtot == dsegsize)	/* handle ELKS default INIT_STACK case*/
	newtot = 0;
  printf("New %s: DATA %lu BSS %ld TOT %lu DYNMEM %lu\n",
	argv[2], header[DATA], header[BSS], newtot, newdynam);

  if (separate == 0) newtot += header[TEXT];
  lseek(fd, (long) TOTPOS, SEEK_SET);
  if (write(fd, &newtot, 4) < 0)
	fatalmsg("chmem: can't modify ", argv[2], "\n");
  printf("%s: Stack+malloc area changed from %lu to %lu bytes.\n",
			 argv[2], olddynam, newdynam);
  return 0;
}
