/*
 * mk_entry(), grow()
 */

#include <stdio.h>
#include <time.h>
#include "msdos.h"

extern int fd, dir_start, dir_len, clus_size, dir_entries;
extern int dir_chain[25];

/*
 * Make a directory entry.  Builds a directory entry based on the
 * name, attribute, starting cluster number, and size.  Returns a pointer
 * to the static directory structure.
 */

struct directory *
mk_entry(filename, attr, fat, size, date)
char *filename;
unsigned char attr;
int fat;
long size, date;
{
	int i;
	char *strncpy();
	static struct directory ndir;
	struct tm *now, *localtime();
	unsigned char hour, min_hi, min_low, sec;
	unsigned char year, month_hi, month_low, day;

	now = localtime(&date);
	strncpy((char *) ndir.name, filename, 8);
	strncpy((char *) ndir.ext, filename+8, 3);
	ndir.attr = attr;
	for (i=0; i<10; i++)
		ndir.reserved[i] = '\0';
	hour = now->tm_hour << 3;
	min_hi = now->tm_min >> 3;
	min_low = now->tm_min << 5;
	sec = now->tm_sec / 2;
	ndir.time[1] = hour + min_hi;
	ndir.time[0] = min_low + sec;
	year = (now->tm_year - 80) << 1;
	month_hi = (now->tm_mon+1) >> 3;
	month_low = (now->tm_mon+1) << 5;
	day = now->tm_mday;
	ndir.date[1] = year + month_hi;
	ndir.date[0] = month_low + day;
	ndir.start[1] = fat / 0x100;
	ndir.start[0] = fat % 0x100;
	ndir.size[3] = 0;		/* can't be THAT large */
	ndir.size[2] = size / 0x10000L;
	ndir.size[1] = (size % 0x10000L) / 0x100;
	ndir.size[0] = (size % 0x10000L) % 0x100;
	return(&ndir);
}

/*
 * Make a subdirectory grow in length.  Only subdirectories (not root) 
 * may grow.  Returns a 0 on success or 1 on failure (disk full).
 */

int
grow(fat)
int fat;
{
	int i, next, last, num, sector, buflen;
	unsigned char tbuf[CLSTRBUF];
	void perror(), exit(), move();

	last = nextfat(0);
	if (last == -1)
		return(1);

	while (1) {
		next = getfat(fat);
		if (next == -1) {
			fprintf(stderr, "grow: FAT problem\n");
			exit(1);
		}
					/* end of cluster chain */
		if (next >= 0xff8)
			break;
		fat = next;
	}
					/* mark the end of the chain */
	putfat(fat, (unsigned int) last);
	putfat(last, 0xfff);
					/* zero the buffer */
	buflen = clus_size * MSECSIZ;
	for (i=0; i<buflen; i++)
		tbuf[i] = '\0';

					/* write the cluster */
	sector = (last - 2) * clus_size + dir_start + dir_len;
	move(sector);
	if (write(fd, (char *) tbuf, buflen) != buflen) {
		perror("grow: write");
		exit(1);
	}
					/* fix up the globals.... */
	num = dir_entries / 16;
	dir_entries += clus_size * 16;
	dir_chain[num] = sector;
	if (clus_size == 2)
		dir_chain[num+1] = sector +1;
	return(0);
}
