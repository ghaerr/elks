/*
 * fdtest - A program to test floppy disk I/O speed in user space
 */
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <linuxmt/biosparm.h>
#include <linuxmt/memory.h>

struct biosparms bdt;

#define MAX		30		/* max # of sectors per read */
#define SECSIZE		512		/* sector size*/

unsigned char kernel_buf[MAX * SECSIZE];
int verbose = 0;
int out = 0;

int read_disk(unsigned short drive, unsigned short cylinder, unsigned short head,
	unsigned short sector, unsigned short count)
{
	if (count > MAX) count = MAX;
	if (verbose)
		fprintf(stderr, "d %d c %d h %d s %d cnt %d\n", drive, cylinder, head, sector, count);

	/* BIOS disk read into "kernel" buffer*/
	BD_AX = BIOSHD_READ | count;
	BD_BX = _FP_OFF(kernel_buf);	/* note: 64k DMA boundary ignored here*/
	BD_ES = _FP_SEG(kernel_buf);
	BD_CX = (unsigned short) (((cylinder&0xff) << 8) | ((cylinder&0x300) >> 2) | sector);
	BD_DX = (head << 8) | drive;
	if (call_bios(&bdt)) {
		fprintf(stderr, "READ ERROR at CHS %d,%d,%d\n", cylinder, head, sector);
		return(1);
	}

	if (out)
		write(1, kernel_buf, count*512);
	return(0);
}

int main(int ac, char **av)
{
	unsigned short drive, cylinder, head, sector;
	unsigned short start_dma_page, end_dma_page;
	unsigned long normalized_seg;
	int i, max, direct = 0;
	unsigned short bs = 1;
	time_t t, tt;

	/* setup starting CHS*/
	drive = 0;		/* 0-1*/
	cylinder = 3;		/* 0-79*/
	head = 0;		/* 0-1*/
	sector = 1;		/* 1-18, 18 on 1.44M floppies */
	max = 18;		/* sectors per track on device */

	while (--ac) {
		av++;
		switch ((*av)[1]) {
		case 'd':	/* select drive, 0 or 1 */
			drive = atoi(*(++av));
			ac--;
			break;
		case 'c':	/* select (starting) cylinder, 0 - 79 */
			cylinder = atoi(*(++av));
			ac--;
			break;
		case 'h':	/* select start head, 0 or 1 */
			head = atoi(*(++av));
			ac--;
			break;
		case 's':	/* select staring sector, 1 to MAX or max */
			sector = atoi(*(++av));
			ac--;
			break;
		case 't':	/* set sectors per track */
			max = atoi(*(++av));
			if (max > MAX) max = MAX;
			ac--;
			break;
		case 'D':	/* Direct: Read entore disk from the given starting point */
			direct++;
			break;
		case 'b':	/* blocksize, only in Direct Mode */
			bs = atoi(*(++av));
			ac--;
			if (bs < 1 || bs > MAX) bs = 1;
			break;
		case 'v':	/* verbose, show debug messages */
			verbose++;
			break;
		case 'o':	/* send read data to stdout */
			out++;
			break;
		case 'u':
		default:
			fprintf(stderr, "usage: fdtest [-D] [-v] [-o] [-d drive] [-c cylinder] [-h head]\n[-s sector] [-t sectors per track] [-b blocksize]\n");
			fprintf(stderr, "-D: Read entire device, -v: verbose, -o: send data to stdout\n");
			return(1);
		}	
	}

	/* dma start/end pages must be equal to ensure I/O within 64k DMA boundary for INT 13h*/
	if (verbose) {
		start_dma_page = (_FP_SEG(kernel_buf) + ((__u16) kernel_buf >> 4)) >> 12;
		end_dma_page = (_FP_SEG(kernel_buf) + ((__u16) (kernel_buf + max * 512 - 1) >> 4)) >> 12;
		normalized_seg = (((__u32)_FP_SEG(kernel_buf) + (_FP_OFF(kernel_buf) >> 4)) << 16) +
			(_FP_OFF(kernel_buf) & 15);
		fprintf(stderr, "dma start page %x, dma end page %x, buffer at %x:%x - ",
			start_dma_page, end_dma_page, _FP_SEG(normalized_seg), _FP_OFF(normalized_seg));
		fprintf(stderr, start_dma_page == end_dma_page? "OK\n": "ERROR\n");
	}

	signal(SIGINT, SIG_IGN);	/* any signal here will likely crash the system */
	if (direct) {
		int  blocks = 0;

		fprintf(stderr, "Testing entire drive %d, max %d incr %d\n", drive, max, bs);
		cylinder = 0;
		t = time(NULL);

		while (read_disk(drive, cylinder, head, sector, bs) == 0) {
			blocks++; 
			if (sector + bs <= max) sector += bs;
			else {
				if (bs <= max) {
					sector = sector + bs - max;
					if (!head)
						head++;
					else {
						head--;
						cylinder++;
					}
				} else {
					int j;
					for (j = bs; j > max; j -= max) {
						if (head) {
							head--;
							cylinder++;
						} else head++;
					}
					sector += bs%max;
					if (sector > max) {
						sector -= max;
						if (head) {
							head--;
							cylinder++;
						} else 
							head++;
					}
				}
			}
			if (!verbose) fprintf(stderr, ".");
		}
		fprintf(stderr, "\nRead %d blocks in %ld seconds\n", blocks, time(NULL) - t);
		return(0);
	}

	fprintf(stderr, "Reading %d sectors individually\n", max);
	t = time(NULL);
	for (i=0; i < max; i++)
		(void)read_disk(drive, cylinder, head, sector+i, 1);
	fprintf(stderr, "%ld secs\n", time(NULL) - t);
	
	fprintf(stderr, "Reading %d sectors as %d 1024-byte blocks\n", max, max/2);
	t = time(NULL);
	for (i=0; (sector+i) < max-1; i += 2)
		(void)read_disk(drive, cylinder, head, sector+i, 2);
	fprintf(stderr, "%ld secs\n", time(NULL) - t);
	
	fprintf(stderr, "Reading %d sectors at once\n", max);
	t = time(NULL);
	(void)read_disk(drive, cylinder, head, sector, max);
	tt = time(NULL);
	fprintf(stderr, "%ld secs\n", tt-t);
	
}
