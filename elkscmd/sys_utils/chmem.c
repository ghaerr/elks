/* chmem - set total memory size for execution	Author: Andy Tanenbaum
 * from original Minix 1 source in "Operating Systems: Design and Implentation", 1st ed.
 *
 * Completely rewritten for ELKS by Greg Haerr
 *
 * The 8088 architecture does not make it possible to catch stacks that grow big.  The
 * only way to deal with this problem is to let the stack grow down towards the data
 * segment and the data segment grow up towards the stack. Normally, a total of 4K is
 * allocated for each of them, but if the programmer knows that a smaller amount is
 * sufficient, he can change it using chmem.
 *
 * Usage: chmem [-h heap] [-s stack] files...
 *	Heap or stack values are specified in decimal by default, or hex with leading 0x.
 *	Maximum heap can be specified using -h 0xffff, which is treated specially by the kernel.
 *	When specifying heap or stack options, chmem will always write out a v1 header.
 *	Functions as 'size' program when run with no options.
 *
 * Explanations of chmem (size) output:
 *	TEXT	size of code
 *	FTEXT	size of far segment code (for medium memory model programs)
 *	DATA	size of initialized data
 *	BSS		size of uninitialized data
 * 	HEAP	header heap size (0 = default 4K, 0xFFFF = allocate maximum heap)
 *	STACK	header minimum stack size (0 = default 4K)
 *	TOTDATA	size of DATA+BSS+heap+stack
 *	TOTAL	size of TOTDATA+TEXT+FTEXT (in-memory size less environment)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <linuxmt/minix.h>

#define SEPBIT   0x00200000	/* this bit is set for separate I/D */
#define MAGIC       0x0301	/* magic number for executable progs */
#define MAX         65520L	/* maximum allocation size */

char *progname;

/* Print an error message and die */
int msg(const char *s, ...)
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

int do_chmem(char *filename, int changeheap, int changestack,
	 unsigned long heap, unsigned long stack)
{
	int				fd;
	int				rdonly = !changeheap && !changestack;
	unsigned int	oldheap, oldstack, dsegsize, displayheap, newheap;
	unsigned long	newstack, totdata, total;
	struct minix_exec_hdr header;
	struct elks_supl_hdr suplhdr;

	if ((fd = open(filename, rdonly? O_RDONLY: O_RDWR)) < 0) {
		perror(filename);
		return 1;
	}
	if (read(fd, &header, sizeof(header)) != sizeof(header)
	    || header.hlen < sizeof(header))
		return msg("%s: bad header\n", filename);

	if ((header.type & 0xFFFF) != MAGIC)
		return msg("%s: not an executable\n", filename);

	if (header.version > 1)
		return msg("%s: unsupported a.out version %u\n", filename,
			   (unsigned)header.version);

	if (header.hlen >= sizeof(header) + sizeof(suplhdr)) {
		if (read(fd, &suplhdr, sizeof(suplhdr)) != sizeof(suplhdr))
			return msg("%s: bad header\n", filename);
	} else
		memset(&suplhdr, 0, sizeof(suplhdr));

	dsegsize = (size_t)header.dseg + (size_t)header.bseg;
	if ((header.type & SEPBIT) == 0)	/* not seperate I&D*/
		dsegsize += (size_t)header.tseg;

	if (header.chmem == 0) {				/* default heap*/
		oldheap = INIT_HEAP;
		displayheap = 0;
	} else if (header.version == 1)
		displayheap = oldheap = header.chmem;
	else
		displayheap = oldheap = header.chmem - dsegsize;	/* v0 displays effective heap, not header value*/

	if (header.version == 1)
		oldstack = header.minstack? header.minstack: INIT_STACK;
	else oldstack = 0;						/* v0 doesn't display seperate stack*/

	if (header.chmem >= 0xFFF0) {
		totdata = 0xFFF0;
		total = totdata + (size_t)header.tseg + (size_t)suplhdr.esh_ftseg;
	} else {
		totdata = (unsigned long)oldheap+oldstack+dsegsize;
		total = (unsigned long)(size_t)header.tseg+(size_t)suplhdr.esh_ftseg
			+(size_t)header.dseg+(size_t)header.bseg+oldheap+oldstack;
	}
	printf("%5u  %5u  %5u  %5u  %5u  %5u   %6lu %6lu %s",
		(size_t)header.tseg, (size_t)suplhdr.esh_ftseg, (size_t)header.dseg, (size_t)header.bseg,
		displayheap, header.minstack, totdata, total, filename);

	if (header.version == 0)
		printf(" (v0 header)");
	if (suplhdr.esh_compr_tseg || suplhdr.esh_compr_dseg || suplhdr.esh_compr_ftseg)
		printf(" (%u %u %u)",
			suplhdr.esh_compr_tseg, suplhdr.esh_compr_ftseg, suplhdr.esh_compr_dseg);
	printf("\n");

	if (rdonly) {
		close(fd);
		return 0;
	}

	if (!changeheap)
		heap = header.chmem;
	if (!changestack)
		stack = header.minstack;

	if (heap > 0xFFFF)
		return msg("%s: heap too large: %ld\n", filename, heap);
	if (stack > MAX)
		return msg("%s: stack too large: %ld\n", filename, stack);
	newstack = stack? stack: INIT_STACK;
	newheap = heap? heap: INIT_HEAP;

	if (newheap < 0xFFF0 && (unsigned long)dsegsize+newheap+newstack > MAX) {
		heap = MAX - dsegsize - newstack - 128;		/* reserve 128 bytes for environment*/
		if (heap < dsegsize)
			return msg("%s: heap+stack too large: %ld\n", filename, heap + newstack);
		msg("Warning: heap truncated to %ld\n", heap);
		newheap = heap;
	}

	if (newheap >= 0xFFF0) {
		totdata = 0xFFF0;
		total = totdata + (size_t)header.tseg;
	} else {
		totdata = (unsigned long)newheap+newstack+dsegsize;
		total = (unsigned long)(size_t)header.tseg+(size_t)header.dseg+(size_t)header.bseg+newheap+newstack;
	}
	printf("%5u  %5u  %5u  %5u  %5lu  %5lu   %6lu %6lu %s\n",
	       (size_t)header.tseg, (size_t)suplhdr.esh_ftseg, (size_t)header.dseg, (size_t)header.bseg, heap, stack,
	       totdata, total, filename);

	header.chmem = (unsigned)heap;
	header.minstack = (unsigned)stack;
	header.version = 1;

	lseek(fd, 0L, SEEK_SET);
	if (write(fd, &header, sizeof(header)) != sizeof(header)) {
		perror(filename);
		close(fd);
		return 1;
	}
	close(fd);

	return 0;
}

int
main(int argc, char **argv)
{
	int 			ch, err = 0;
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

	printf(" TEXT  FTEXT   DATA    BSS   HEAP  STACK  TOTDATA  TOTAL\n");
	while (optind < argc) {
		if (do_chmem(argv[optind], changeheap, changestack,
					   heap, stack))
			err = 1;
		argc--;
		argv++;
	}

	return err;
}
