/*  Copyright 1995-1999,2001-2003,2007,2009,2011 Alain Knaff.
 *  This file is part of mtools.
 *
 *  Mtools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mtools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mtools.  If not, see <http://www.gnu.org/licenses/>.
 *
 * mbadblocks.c
 * Mark bad blocks on disk
 *
 */

#include "sysincludes.h"
#include "msdos.h"
#include "mtools.h"
#include "mainloop.h"
#include "fsP.h"

#define N_PATTERN 311

static void usage(int ret) NORETURN;
static void usage(int ret)
{
	fprintf(stderr, "Mtools version %s, dated %s\n",
		mversion, mdate);
	fprintf(stderr, "Usage: %s: [-c clusterList] [-s sectorList] [-c] [-V] device\n",
		progname);
	exit(ret);
}

static void checkListTwice(char *filename) {
	if(filename != NULL) {
		fprintf(stderr, "Only one of the -c or -s options may be given\n");
		exit(1);
	}
}

/**
 * Marks given cluster as bad, but prints error instead if cluster already used
 */
static void mark(Fs_t *Fs, long offset, unsigned int badClus) {
	unsigned int old = Fs->fat_decode((Fs_t*)Fs, offset);
	if(old == 0) {
		fatEncode((Fs_t*)Fs, offset, badClus);
		return;
	}
	if(old == badClus) {
		fprintf(stderr, "Cluster %ld already marked\n", offset);
	} else {
		fprintf(stderr, "Cluster %ld is busy\n", offset);
	}
}

static char *in_buf;
static char *pat_buf;
static size_t in_len;


static void progress(unsigned int i, unsigned int total) {
	if(i % 10 == 0)
		fprintf(stderr,	"                     \r%d/%d\r", i, total);
}

static int scan(Fs_t *Fs, Stream_t *dev,
		long cluster, unsigned int badClus,
		char *buffer, int doWrite) {
	off_t start;
	off_t ret;
	off_t pos;
	int bad=0;

	if(Fs->fat_decode((Fs_t*)Fs, cluster))
		/* cluster busy, or already marked */
		return 0;
	start = (cluster - 2) * Fs->cluster_size + Fs->clus_start;
	pos = sectorsToBytes((Stream_t*)Fs, start);
	if(doWrite) {
		ret = force_write(dev, buffer, pos, in_len);
		if(ret < (off_t) in_len )
			bad = 1;
	} else {
		ret = force_read(dev, in_buf, pos, in_len);
		if(ret < (off_t) in_len )
			bad = 1;
		else if(buffer) {
			size_t i;
			for(i=0; i<in_len; i++)
				if(in_buf[i] != buffer[i]) {
					bad = 1;
					break;
				}
		}
	}

	if(bad) {
		printf("Bad cluster %ld found\n", cluster);
		fatEncode((Fs_t*)Fs, cluster, badClus);
		return 1;
	}
	return 0;
}

void mbadblocks(int argc, char **argv, int type UNUSEDP) NORETURN;
void mbadblocks(int argc, char **argv, int type UNUSEDP)
{
	unsigned int i;
	unsigned int startSector=2;
	unsigned int endSector=0;
	struct MainParam_t mp;
	Fs_t *Fs;
	Stream_t *Dir;
	int ret;
	char *filename = NULL;
	int c;
	unsigned int badClus;
	int sectorMode=0;
	int writeMode=0;

	while ((c = getopt(argc, argv, "i:s:cwS:E:")) != EOF) {
		switch(c) {
		case 'i':
			set_cmd_line_image(optarg);
			break;
		case 'c':
			checkListTwice(filename);
			filename = strdup(optarg);
			break;
		case 's':
			checkListTwice(filename);
			filename = strdup(optarg);
			sectorMode = 1;
			break;
		case 'S':
			startSector = atoui(optarg); 
			break;
		case 'E':
			endSector = atoui(optarg); 
			break;
		case 'w':
			writeMode = 1;
			break;
		case 'h':
			usage(0);
		default:
			usage(1);
		}
	}

	if (argc != optind+1 ||
	    !argv[optind][0] || argv[optind][1] != ':' || argv[optind][2]) {
		usage(1);
	}

	init_mp(&mp);

	Dir = open_root_dir(argv[optind][0], O_RDWR, NULL);
	if (!Dir) {
		fprintf(stderr,"%s: Cannot initialize drive\n", argv[0]);
		exit(1);
	}

	Fs = (Fs_t *)GetFs(Dir);
	in_len = Fs->cluster_size * Fs->sector_size;
	in_buf = malloc(in_len);
	if(!in_buf) {
		printOom();
		ret = 1;
		goto exit_0;
	}
	if(writeMode) {
		pat_buf=malloc(in_len * N_PATTERN);
		if(!pat_buf) {
			printOom();
			ret = 1;
			goto exit_0;
		}
		init_random();
		for(i=0; i < in_len * N_PATTERN; i++) {
			pat_buf[i] = random();
		}
	}
	for(i=0; i < Fs->clus_start; i++ ){
		ret = READS(Fs->Next, in_buf, 
			    sectorsToBytes((Stream_t*)Fs, i), Fs->sector_size);
		if( ret < 0 ){
			perror("early error");
			goto exit_0;
		}
		if(ret < (signed int) Fs->sector_size){
			fprintf(stderr,"end of file in file_read\n");
			ret = 1;
			goto exit_0;
		}
	}
	ret = 0;

	badClus = Fs->last_fat + 1;

	if(startSector < 2)
		startSector = 2;
	if(endSector > Fs->num_clus + 2 || endSector <= 0) 
		endSector = Fs->num_clus + 2;

	if(filename) {
		char line[80];

		FILE *f = fopen(filename, "r");
		if(f == NULL) {
			fprintf(stderr, "Could not open %s (%s)\n",
				filename, strerror(errno));
			ret = 1;
			goto exit_0;
		}
		while(fgets(line, sizeof(line), f)) {
			char *ptr = line + strspn(line, " \t");
			long offset = strtol(ptr, 0, 0);
			if(sectorMode)
				offset = (offset-Fs->clus_start)/Fs->cluster_size + 2;
			if(offset < 2) {
				fprintf(stderr, "Sector before start\n");
			} else if(offset >= Fs->num_clus) {
				fprintf(stderr, "Sector beyond end\n");
			} else {
				mark(Fs, offset, badClus);
				ret = 1;
			}
		}
	} else {
		Stream_t *dev;
		dev = Fs->Next;
		if(dev->Next)
			dev = dev->Next;

		in_len = Fs->cluster_size * Fs->sector_size;
		if(writeMode) {
			/* Write pattern */
			for(i=startSector; i< endSector; i++){
				if(got_signal)
					break;
				progress(i, Fs->num_clus);
				ret |= scan(Fs, dev, i, badClus, 
					    pat_buf + in_len * (i % N_PATTERN),
					    1);
			}

			/* Flush cache, so that we are sure we read the data
			   back from disk, rather than from the cache */
			if(!got_signal)
				DISCARD(dev);

			/* Read data back, and compare to pattern */
			for(i=startSector; i< endSector; i++){
				if(got_signal)
					break;
				progress(i, Fs->num_clus);
				ret |= scan(Fs, dev, i, badClus, 
					    pat_buf + in_len * (i % N_PATTERN),
					    0);
			}

		} else {

			for(i=startSector; i< endSector; i++){
				if(got_signal)
					break;
				progress(i, Fs->num_clus);
				ret |= scan(Fs, dev, i, badClus, NULL, 0);
			}
		}
	}
 exit_0:
	FREE(&Dir);
	exit(ret);
}
