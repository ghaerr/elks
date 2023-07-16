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
#include <linuxmt/sched.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define LINEARADDRESS(off, seg)		((off_t) (((off_t)seg << 4) + off))

int aflag;		/* show application memory*/
int fflag;		/* show free memory*/
int tflag;		/* show tty memory*/
int bflag;		/* show buffer memory*/
int allflag;	/* show all memory*/

unsigned int ds;
unsigned int heap_all;
unsigned int taskoff;
struct task_struct task_table[MAX_TASKS];

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

void process_name(int fd, unsigned int off, unsigned int seg)
{
    word_t argc, argv;
    char buf[80];

    argc = getword(fd, off, seg);

    while (argc-- > 0) {
        off += 2;
        argv = getword(fd, off, seg);
        if (!memread(fd, argv, seg, buf, sizeof(buf)))
            return;
        printf("%s ",buf);
        break;      /* display only executable name for now */
    }
}

int find_process(unsigned int seg)
{
    struct task_struct *t;
    for (t = task_table; t < &task_table[MAX_TASKS]; t++) {
        if ((unsigned)t->mm.seg_code == seg || (unsigned)t->mm.seg_data == seg) {
            return t - task_table;
        }
    }
    return -1;
}

void dump_heap(int fd)
{
	word_t total_size = 0;
	word_t total_free = 0;
	long total_segsize = 0;
	static char *heaptype[] = { "free", "SEG ", "STR ", "TTY ", "INT ", "BUFH", "PIPE" };
	static char *segtype[] = { "free", "CSEG", "DSEG", "BUF ", "RDSK", "PROG" };

	printf("  HEAP   TYPE  SIZE    SEG   TYPE    SIZE  CNT  NAME\n");

	word_t n = getword (fd, heap_all + offsetof(list_s, next), ds);
	while (n != heap_all) {
		word_t h = n - offsetof(heap_s, all);
		word_t size = getword(fd, h + offsetof(heap_s, size), ds);
		byte_t tag = getword(fd, h + offsetof(heap_s, tag), ds) & HEAP_TAG_TYPE;
		word_t mem = h + sizeof(heap_s);
		seg_t segbase;
		segext_t segsize;
		word_t segflags, ref_count;
		int free, used, tty, buffer, t;

		if (tag == HEAP_TAG_SEG)
			segflags = getword(fd, mem + offsetof(segment_s, flags), ds) & SEG_FLAG_TYPE;
		else segflags = -1;
		free = (tag == HEAP_TAG_FREE || segflags == SEG_FLAG_FREE);
		used = ((tag == HEAP_TAG_SEG) && (segflags == SEG_FLAG_CSEG || segflags == SEG_FLAG_DSEG));
		tty = (tag == HEAP_TAG_TTY);
		buffer = ((tag == HEAP_TAG_SEG) && (segflags == SEG_FLAG_EXTBUF));

		if (allflag ||
		   (fflag && free) || (aflag && used) || (tflag && tty) || (bflag && buffer)) {
			printf("  %4x   %s %5d", mem, heaptype[tag], size);
			total_size += size + sizeof(heap_s);
			if (tag == HEAP_TAG_FREE)
				total_free += size;

			switch (tag) {
			case HEAP_TAG_SEG:
				segbase = getword(fd, mem + offsetof(segment_s, base), ds);
				segsize = getword(fd, mem + offsetof(segment_s, size), ds);
				ref_count = getword(fd, mem + offsetof(segment_s, ref_count), ds);
				printf("   %4x   %s %7ld %4d  ",
                    segbase, segtype[segflags], (long)segsize << 4, ref_count);
                if (segflags == SEG_FLAG_CSEG || segflags == SEG_FLAG_DSEG) {
                    if ((t = find_process(mem)) >= 0) {
			            process_name(fd, task_table[t].t_begstack, task_table[t].t_regs.ss);
                    }
                }

				total_segsize += (long)segsize << 4;
				break;
			}
			printf("\n");
		}

		/* next in heap*/
		n = getword(fd, n + offsetof(list_s, next), ds);
	}

	printf("  Heap/free   %5d/%5d Total mem %7ld\n", total_size, total_free, total_segsize);
}

void usage(void)
{
	printf("usage: meminfo [-a][-f][-t][-b]\n");
}

int main(int argc, char **argv)
{
	int fd, c;
	struct mem_usage mu;

	if (argc < 2)
		allflag = 1;
	else while ((c = getopt(argc, argv, "aftbh")) != -1) {
		switch (c) {
			case 'a':
				aflag = 1;
				break;
			case 'f':
				fflag = 1;
				break;
			case 't':
				tflag = 1;
				break;
			case 'b':
				bflag = 1;
				break;
			case 'h':
				usage();
				return 0;
			default:
				usage();
				return 1;
		}
	}

	if ((fd = open("/dev/kmem", O_RDONLY)) < 0) {
		perror("meminfo");
		return 1;
	}
    if (ioctl(fd, MEM_GETDS, &ds) ||
        ioctl(fd, MEM_GETHEAP, &heap_all) ||
        ioctl(fd, MEM_GETTASK, &taskoff)) {
          perror("meminfo");
        return 1;
    }
    if (!memread(fd, taskoff, ds, &task_table, sizeof(task_table))) {
        perror("taskinfo");
    }
	dump_heap(fd);

	if (!ioctl(fd, MEM_GETUSAGE, &mu)) {
		/* note MEM_GETUSAGE amounts are floors, so total may display less by 1k than actual*/
		printf("  Memory usage %4dKB total, %4dKB used, %4dKB free\n",
			mu.used_memory + mu.free_memory, mu.used_memory, mu.free_memory);
	}

	return 0;
}
