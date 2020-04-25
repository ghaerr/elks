/*
 * meminfo.c
 *
 * Copyright (c) 2002 Harry Kalogirou
 * harkal@gmx.net
 *
 * Enhanced by Greg Haerr 24 Apr 2020
 *
 * This file may be distributed under the terms of the GNU General Public
 * License v2, or at your option any later version.
 */
#include <linuxmt/types.h>
#include <linuxmt/mm.h>
#include <linuxmt/mem.h>
#include <linuxmt/heap.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define LINEARADDRESS(off, seg)		((off_t) (((off_t)seg << 4) + off))

unsigned int ds;
unsigned int heap_first;

int memread(int fd, word_t off, word_t seg, void *buf, int size)
{
	if (lseek(fd, LINEARADDRESS(off, seg), SEEK_SET) == -1)
		return 0;

	if (read(fd, buf, size) != size)
		return 0;

	return 1;
}

word_t getword(int fd, word_t off, word_t seg)
{
	word_t word;

	if (!memread(fd, off, seg, &word, sizeof(word)))
		return 0;
	return word;
}

void dump_heap(int fd)
{
	word_t h = heap_first;
	word_t total_size = 0;
	long total_segsize = 0;
	static char *heaptype[] = { "free", "SEG ", "STR ", "TTY " };
	static char *segtype[] = { "free", "CSEG", "DSEG", "BUF ", "RDSK" };

	if (!h)
		return;
	printf("  HEAP   TYPE  SIZE    SEG   TYPE    SIZE  CNT\n");
	do {
		word_t size = getword(fd, h + offsetof(heap_s, size), ds);
		byte_t tag = getword(fd, h + offsetof(heap_s, tag), ds) & HEAP_TAG_TYPE;
		word_t mem = h + sizeof(heap_s);
		seg_t segbase;
		segext_t segsize;
		word_t flags, ref_count;

		printf("  %4x   %s %5d", mem, heaptype[tag], size);
		total_size += size + sizeof(heap_s);

		switch (tag) {
		case HEAP_TAG_SEG:
			segbase = getword(fd, mem + offsetof(segment_s, base), ds);
			segsize = getword(fd, mem + offsetof(segment_s, size), ds);
			flags = getword(fd, mem + offsetof(segment_s, flags), ds);
			ref_count = getword(fd, mem + offsetof(segment_s, ref_count), ds);
			printf("   %4x   %s %7ld %4d", segbase, segtype[flags & 0x0F], (long)segsize << 4, ref_count);
			total_segsize += (long)segsize << 4;
			break;
		}

		printf("\n");
		h = getword(fd, (word_t)h + offsetof(heap_s, node) + offsetof(list_s, next), ds);
	} while (h != heap_first);
	printf("  Total heap  %5d     Total mem %7ld\n", total_size, total_segsize);
}

int main(int argc, char **argv)
{
	int fd;
	struct mem_usage mu;

	if ((fd = open("/dev/kmem", O_RDONLY)) < 0) {
		perror("meminfo");
		return 1;
	}
	if (ioctl(fd, MEM_GETDS, &ds) || ioctl(fd, MEM_GETHEAP, &heap_first)) {
		perror("meminfo");
		return 1;
	}

	dump_heap(fd);

	if (!ioctl(fd, MEM_GETUSAGE, &mu)) {
		/* note MEM_GETUSAGE amounts are floors, so total may display less by 1k than actual*/
		printf("  Memory usage %4dKB total, %4dKB used, %4dKB free\n",
			mu.used_memory + mu.free_memory, mu.used_memory, mu.free_memory);
	}

	return 0;
}
