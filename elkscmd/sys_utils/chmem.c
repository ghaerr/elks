/* chmem - set total memory size for execution	Author: Andy Tanenbaum
 * from original Minix 1 source in "Operating Systems: Design and Implentation", 1st ed.
 *
 * Completely rewritten for ELKS by Greg Haerr
 *	Also functions as 'size' program.
 *
* The 8088 architecture does not make it possible to catch stacks that grow big.  The
 * only way to deal with this problem is to let the stack grow down towards the data
 * segment and the data segment grow up towards the stack. Normally, a total of 64K is
 * allocated for the two of them, but if the programmer knows that a smaller amount is
 * sufficient, he can change it using chmem.
 *
 * chmem =4096 prog  sets the total space for stack + data growth to 4096 chmem +200  prog
 * increments the total space for stack + data growth by 200
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <linuxmt/minix.h>

#define TOTPOS          24	/* where is total in header */
#define SEPBIT   0x00200000	/* this bit is set for separate I/D */
#define MAGIC       0x0301	/* magic number for executable progs */
#define MAX         65520L	/* maximum allocation size */

char *progname;

int
msg(const char *s, ...)
{
	va_list		p;
	va_start(p, s);
	vfprintf(stderr, s, p);
	va_end(p);
	return 1;
}

void usage(void)
{
	msg("Usage: %s [-h heap] [-s stack] files...\n", progname);
	exit(1);
}

/* Print an error message and die */
static int
do_chmem(char *filename, int changeheap, int changestack,
	 unsigned long heap, unsigned long stack)
{
	int				fd;
	unsigned int	oldheap, dsegsize;
	unsigned long	newstack;
	struct minix_exec_hdr header;

	if ((fd = open(filename, 2)) < 0)
		return msg("Can't open %s\n", filename);

	if (read(fd, &header, sizeof(header)) != sizeof(header))
		return msg("Bad header: %s\n", filename);

	if ((header.type & 0xFFFF) != MAGIC)
		return msg("%s: not an executable\n", filename);

	if (header.version > 1)
		return msg("%s: unsupported a.out version %u\n", filename,
			   (unsigned)header.version);

	dsegsize = header.dseg + header.bseg;
	if ((header.type & SEPBIT) == 0)	/* not seperate I&D*/
		dsegsize += header.tseg;

	if (header.chmem == 0)					/* default heap*/
		oldheap = 0;
	else if (header.version == 1)
		oldheap = header.chmem;
	else
		oldheap = header.chmem - dsegsize;	/* v0 displays effective heap, not header value*/

	printf("%5u  %5u  %5u  %5u  %5u   %6lu %6lu %s%s\n",
		header.tseg, header.dseg, header.bseg, oldheap, header.minstack, (unsigned long)oldheap+dsegsize,
		(long)header.tseg+header.dseg+header.bseg+oldheap, filename, header.version == 0? " (v0 header)": "");

	if (!changeheap && !changestack) {
		close(fd);
		return 0;
	}

	if (!changeheap)
		heap = oldheap;
	if (!changestack)
		stack = header.minstack;

	newstack = stack? stack: INIT_STACK;
	if (newstack > MAX)
		return msg("%s: stack too large: %ld\n", filename, newstack);

	if ((unsigned long)dsegsize+heap+newstack > MAX) {
		heap = MAX - dsegsize - newstack;
		if (heap < dsegsize)
			return msg("%s: heap+stack too large: %ld\n", filename, heap + newstack);
		msg("Warning: heap truncated to %ld\n", heap);
	}

	printf("%5u  %5u  %5u  %5lu  %5lu   %6lu %6lu %s\n",
	       header.tseg, header.dseg, header.bseg, heap, stack, heap+dsegsize,
		   header.tseg+header.dseg+header.bseg+heap, filename);

	header.chmem = (unsigned)heap;
	header.minstack = (unsigned)stack;

	header.version = 1;

	lseek(fd, 0L, SEEK_SET);
	if (write(fd, &header, sizeof(header)) != sizeof(header))
		return msg("Can't write header: %s\n", filename);
	close(fd);

	return 0;
}

int
main(int argc, char **argv)
{
	int 			ch, err;
	int				changeheap = 0, changestack = 0;
	unsigned long	heap = 0, stack = 0;

	progname = argv[0];
	while ((ch = getopt(argc, argv, "h:s:")) != -1) {
		switch (ch) {
		case 'h':
			heap = strtoul(optarg, NULL, 0);
			changeheap = 1;
			break;
		case 's':
			stack = strtoul(optarg, NULL, 0);
			changestack = 1;
			break;
		default:
			usage();
			break;
		}
	}
	if (optind >= argc)
		usage();

	printf(" TEXT   DATA    BSS   HEAP  STACK  TOTDATA  TOTAL\n");
	while (optind < argc) {
		if (do_chmem(argv[optind], changeheap, changestack,
					   heap, stack))
			err = 1;
		argc--;
		argv++;
	}

	return err;
}
