/*  Copyright 1997-2003,2005-2007,2009 Alain Knaff.
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
 * mformat.c
 */
#define DONT_NEED_WAIT

#include "sysincludes.h"
#include "msdos.h"
#include "mtools.h"
#include "mainloop.h"
#include "fsP.h"
#include "file.h"
#include "plain_io.h"
#include "nameclash.h"
#include "buffer.h"
#include "scsi.h"
#include "partition.h"

#ifdef OS_linux
#include "linux/hdreg.h"

#define _LINUX_STRING_H_
#define kdev_t int
#include "linux/fs.h"
#undef _LINUX_STRING_H_

#endif

#define tolinear(x) \
(sector(x)-1+(head(x)+cyl(x)*used_dev->heads)*used_dev->sectors)


static __inline__ void print_hsc(hsc *h)
{
	printf(" h=%d s=%d c=%d\n",
	       head(*h), sector(*h), cyl(*h));
}

static void set_offset(hsc *h, int offset, int heads, int sectors)
{
	int head, sector, cyl;

	if(! heads || !sectors)
		head = sector = cyl = 0; /* linear mode */
	else {
		sector = offset % sectors;
		offset = offset / sectors;

		head = offset % heads;
		cyl = offset / heads;
		if(cyl > 1023) cyl = 1023;
	}

	h->head = head;
	h->sector = ((sector+1) & 0x3f) | ((cyl & 0x300)>>2);
	h->cyl = cyl & 0xff;
}

void setBeginEnd(struct partition *partTable,
		 unsigned int begin, unsigned int end,
		 unsigned int heads, unsigned int sectors,
		 int activate, int type, int fat_bits)
{
	set_offset(&partTable->start, begin, heads, sectors);
	set_offset(&partTable->end, end-1, heads, sectors);
	set_dword(partTable->start_sect, begin);
	set_dword(partTable->nr_sects, end-begin);
	if(activate)
		partTable->boot_ind = 0x80;
	else
		partTable->boot_ind = 0;
	if(!type) {
		if (fat_bits == 0) {
			/**
			 * Fat bits unknown / not specified. We look
			 * at size to get a rough estimate what FAT
			 * bits are used.  Note: this is only an
			 * estimate, the precise calculation would
			 * involve the number of clusters, which is
			 * not necessarily known here.
			 */
			/* cc977219 would have a cutoff number of 32680,
			 * corresponding to a FAT12 partition with 4K
			 * clusters, however other information hints that
			 * only partitions with less than 4096 sectors are
			 * considered */
			if(end-begin < 4096)
				fat_bits = 12;
			else
				fat_bits = 16;
		}

		/* Description of various partition types in
		 * https://en.wikipedia.org/wiki/Partition_type#List_of_partition_IDs
		 * and
		 * https://docs.microsoft.com/en-us/previous-versions/windows/it-pro/windows-2000-server/cc977219(v=technet.10)
		 */
		if (fat_bits == 32)
			/* FAT 32 partition. For now, we disregard the
			 * possibility of FAT 32 CHS partitions */
			type = 0x0C; /* Win95 FAT32, LBA */
		else if (end < 65536) {
			/* FAT 12 or FAT 16 partitions which fit entirely below
			   the 32M mark */
			/* The 32M restriction doesn't apply to logical
			   partitions within an extended partition, but for the
			   moment mpartition only makes primary partitions */
			if(fat_bits == 12)
				/* FAT 12 partition */
				type = 0x01; /* DOS FAT12, CHS */
			else if (fat_bits == 16)
				/* FAT 16 partition */
				type = 0x04; /* DOS FAT16, CHS */
		} else if (end <  sectors * heads * 1024)
			/* FAT 12 or FAT16 partition above the 32M
			 * mark but below the 1024 cylinder mark.
			 * Indeed, there can be no CHS partition
			 * beyond 1024 cylinders */
			type = 0x06; /* DOS BIG FAT16 or FAT12, CHS */
		else
			type = 0x0E; /* Win95 BIG FAT16, LBA */
	}
	partTable->sys_ind = type;
}

int consistencyCheck(struct partition *partTable, int doprint, int verbose,
		     int *has_activated, unsigned int *last_end,
		     unsigned int *j,
		     struct device *used_dev, int target_partition)
{
	int i;
	unsigned int inconsistency;

	*j = 0;
	*last_end = 1;

	/* quick consistency check */
	inconsistency = 0;
	*has_activated = 0;
	for(i=1; i<5; i++){
		if(!partTable[i].sys_ind)
			continue;
		if(partTable[i].boot_ind)
			(*has_activated)++;
		if((used_dev &&
		    (used_dev->heads != head(partTable[i].end)+1 ||
		     used_dev->sectors != sector(partTable[i].end))) ||
		   sector(partTable[i].start) != 1){
			fprintf(stderr,
				"Partition %d is not aligned\n",
				i);
			inconsistency=1;
		}

		if(*j &&
		   *last_end > BEGIN(partTable[i])) {
			fprintf(stderr,
				"Partitions %d and %d badly ordered or overlapping\n",
				*j,i);
			inconsistency=1;
		}

		*last_end = END(partTable[i]);
		*j = i;

		if(used_dev &&
		   cyl(partTable[i].start) != 1023 &&
		   tolinear(partTable[i].start) != BEGIN(partTable[i])) {
			fprintf(stderr,
				"Start position mismatch for partition %d\n",
				i);
			inconsistency=1;
		}
		if(used_dev &&
		   cyl(partTable[i].end) != 1023 &&
		   tolinear(partTable[i].end)+1 != END(partTable[i])) {
			fprintf(stderr,
				"End position mismatch for partition %d\n",
				i);
			inconsistency=1;
		}

		if(doprint && verbose) {
			if(i==target_partition)
				putchar('*');
			else
				putchar(' ');
			printf("Partition %d\n",i);

			printf("  active=%x\n", partTable[i].boot_ind);
			printf("  start:");
			print_hsc(&partTable[i].start);
			printf("  type=0x%x\n", partTable[i].sys_ind);
			printf("  end:");
			print_hsc(&partTable[i].end);
			printf("  start=%d\n", BEGIN(partTable[i]));
			printf("  nr=%d\n", _DWORD(partTable[i].nr_sects));
			printf("\n");
		}
	}
	return inconsistency;
}

/* setsize function.  Determines scsicam mapping if this cannot be inferred from
 * any existing partitions. Shamelessly snarfed from the Linux kernel ;-) */

/*
 * Function : static int setsize(unsigned long capacity,unsigned int *cyls,
 *	unsigned int *hds, unsigned int *secs);
 *
 * Purpose : to determine a near-optimal int 0x13 mapping for a
 *	SCSI disk in terms of lost space of size capacity, storing
 *	the results in *cyls, *hds, and *secs.
 *
 * Returns : -1 on failure, 0 on success.
 *
 * Extracted from
 *
 * WORKING                                                    X3T9.2
 * DRAFT                                                        792D
 *
 *
 *                                                        Revision 6
 *                                                         10-MAR-94
 * Information technology -
 * SCSI-2 Common access method
 * transport and SCSI interface module
 *
 * ANNEX A :
 *
 * setsize() converts a read capacity value to int 13h
 * head-cylinder-sector requirements. It minimizes the value for
 * number of heads and maximizes the number of cylinders. This
 * will support rather large disks before the number of heads
 * will not fit in 4 bits (or 6 bits). This algorithm also
 * minimizes the number of sectors that will be unused at the end
 * of the disk while allowing for very large disks to be
 * accommodated. This algorithm does not use physical geometry.
 */

static int setsize(unsigned long capacity,unsigned int *cyls,
		   uint16_t *hds,  uint16_t *secs) {
    unsigned int rv = 0;
    unsigned long heads, sectors, cylinders, temp;

    cylinders = 1024L;			/* Set number of cylinders to max */
    sectors = 62L;      		/* Maximize sectors per track */

    temp = cylinders * sectors;		/* Compute divisor for heads */
    heads = capacity / temp;		/* Compute value for number of heads */
    if (capacity % temp) {		/* If no remainder, done! */
    	heads++;                	/* Else, increment number of heads */
    	temp = cylinders * heads;	/* Compute divisor for sectors */
    	sectors = capacity / temp;	/* Compute value for sectors per
					       track */
    	if (capacity % temp) {		/* If no remainder, done! */
      	    sectors++;                  /* Else, increment number of sectors */
      	    temp = heads * sectors;	/* Compute divisor for cylinders */
      	    cylinders = capacity / temp;/* Compute number of cylinders */
      	}
    }
    if (cylinders == 0) rv=(unsigned)-1;/* Give error if 0 cylinders */

    *cyls = (unsigned int) cylinders;	/* Stuff return values */
    *secs = (unsigned int) sectors;
    *hds  = (unsigned int) heads;
    return(rv);
}

static void setsize0(unsigned long capacity,unsigned int *cyls,
		     uint16_t *hds, uint16_t *secs)
{
	int r;

	/* 1. First try "Megabyte" sizes */
	if(capacity < 1024 * 2048 && !(capacity % 1024)) {
		*cyls = capacity >> 11;
		*hds  = 64;
		*secs = 32;
		return;
	}

	/* then try scsicam's size */
	r = setsize(capacity,cyls,hds,secs);
	if(r || *hds > 255 || *secs > 63) {
		/* scsicam failed. Do megabytes anyways */
		*cyls = capacity >> 11;
		*hds  = 64;
		*secs = 32;
		return;
	}
}


static void usage(int ret) NORETURN;
static void usage(int ret)
{
	fprintf(stderr,
		"Mtools version %s, dated %s\n", mversion, mdate);
	fprintf(stderr,
		"Usage: %s [-pradcv] [-I] [-B bootsect-template] [-s sectors] "
			"[-t cylinders] "
		"[-h heads] [-T type] [-b begin] [-l length] "
		"drive\n", progname);
	exit(ret);
}

void mpartition(int argc, char **argv, int dummy UNUSEDP) NORETURN;
void mpartition(int argc, char **argv, int dummy UNUSEDP)
{
	Stream_t *Stream;
	unsigned int dummy2;

	unsigned int i,j;

	int sec_per_cyl;
	int doprint = 0;
	int verbose = 0;
	int create = 0;
	int force = 0;
	unsigned int length = 0;
	int do_remove = 0;
	int initialize = 0;
	unsigned int tot_sectors=0;
	int type = 0;
	int begin_set = 0;
	int size_set = 0;
	int end_set = 0;
	unsigned int last_end = 0;
	int activate = 0;
	int has_activated = 0;
	int inconsistency=0;
	unsigned int begin=0;
	unsigned int end=0;
	int sizetest=0;
	int dirty = 0;
	int open2flags = NO_OFFSET;

	int c;
	struct device used_dev;
	int argtracks, argheads, argsectors;

	char drive, name[EXPAND_BUF];
	unsigned char buf[512];
	struct partition *partTable=(struct partition *)(buf+ 0x1ae);
	struct device *dev;
	char errmsg[2100];
	char *bootSector=0;

	argtracks = 0;
	argheads = 0;
	argsectors = 0;

	/* get command line options */
	if(helpFlag(argc, argv))
		usage(0);
	while ((c = getopt(argc, argv, "i:adprcIT:t:h:s:fvpb:l:S:B:")) != EOF) {
		switch (c) {
			case 'i':
				set_cmd_line_image(optarg);
				break;
			case 'B':
				bootSector = optarg;
				break;
			case 'a':
				/* no privs, as it could be abused to
				 * make other partitions unbootable, or
				 * to boot a rogue kernel from this one */
				open2flags |= NO_PRIV;
				activate = 1;
				dirty = 1;
				break;
			case 'd':
				activate = -1;
				dirty = 1;
				break;
			case 'p':
				doprint = 1;
				break;
			case 'r':
				do_remove = 1;
				dirty = 1;
				break;
			case 'I':
				/* could be abused to nuke all other
				 * partitions */
				open2flags |= NO_PRIV;
				initialize = 1;
				dirty = 1;
				break;
			case 'c':
				create = 1;
				dirty = 1;
				break;

			case 'T':
				/* could be abused to "manually" create
				 * extended partitions */
				open2flags |= NO_PRIV;
				type = strtoi(optarg,0,0);
				break;

			case 't':
				argtracks = atoi(optarg);
				break;
			case 'h':
				argheads = atoi(optarg);
				break;
			case 's':
				argsectors = atoi(optarg);
				break;

			case 'f':
				/* could be abused by creating overlapping
				 * partitions and other such Snafu */
				open2flags |= NO_PRIV;
				force = 1;
				break;

			case 'v':
				verbose++;
				break;
			case 'S':
				/* testing only */
				/* could be abused to create partitions
				 * extending beyond the actual size of the
				 * device */
				open2flags |= NO_PRIV;
				tot_sectors = strtoui(optarg,0,0);
				sizetest = 1;
				break;
			case 'b':
				begin_set = 1;
				begin = strtoui(optarg, NULL, 10);
				break;
			case 'l':
				size_set = 1;
				length = strtoui(optarg, NULL, 10);
				break;

			default:
				usage(1);
		}
	}

	if (argc - optind != 1 ||
	    !argv[optind][0] || argv[optind][1] != ':')
		usage(1);

	drive = ch_toupper(argv[optind][0]);

	/* check out a drive whose letter and parameters match */
	sprintf(errmsg, "Drive '%c:' not supported", drive);
	Stream = 0;
	for(dev=devices;dev->drive;dev++) {
		int mode ;

		FREE(&(Stream));
		/* drive letter */
		if (dev->drive != drive)
			continue;
		if (dev->partition < 1 || dev->partition > 4) {
			sprintf(errmsg,
				"Drive '%c:' is not a partition",
				drive);
			continue;
		}
		used_dev = *dev;

		SET_INT(used_dev.tracks, argtracks);
		SET_INT(used_dev.heads, argheads);
		SET_INT(used_dev.sectors, argsectors);

		expand(dev->name, name);

		mode = dirty ? O_RDWR : O_RDONLY;
		if(initialize)
 			mode |= O_CREAT;

#ifdef USING_NEW_VOLD
		strcpy(name, getVoldName(dev, name));
#endif
		Stream = SimpleFileOpen(&used_dev, dev, name, mode,
					errmsg, open2flags, 1, 0);

		if (!Stream) {
#ifdef HAVE_SNPRINTF
			snprintf(errmsg,sizeof(errmsg)-1,
				 "init: open: %s", strerror(errno));
#else
			sprintf(errmsg,"init: open: %s", strerror(errno));
#endif
			continue;
		}


		/* try to find out the size */
		if(!sizetest)
			tot_sectors = 0;
		if(IS_SCSI(dev)) {
			unsigned char cmd[10];
			unsigned char data[10];
			cmd[0] = SCSI_READ_CAPACITY;
			memset ((void *) &cmd[2], 0, 8);
			memset ((void *) &data[0], 137, 10);
			scsi_cmd(get_fd(Stream), cmd, 10, SCSI_IO_READ,
				 data, 10, get_extra_data(Stream));

			tot_sectors = 1 +
				(data[0] << 24) +
				(data[1] << 16) +
				(data[2] <<  8) +
				(data[3]      );
			if(verbose)
				printf("%d sectors in total\n", tot_sectors);
		}

#ifdef OS_linux
		if (tot_sectors == 0) {
			ioctl(get_fd(Stream), BLKGETSIZE, &tot_sectors);
		}
#endif

		/* read the partition table */
		if (READS(Stream, (char *) buf, 0, 512) != 512 && !initialize){
#ifdef HAVE_SNPRINTF
			snprintf(errmsg, sizeof(errmsg)-1,
				"Error reading from '%s', wrong parameters?",
				name);
#else
			sprintf(errmsg,
				"Error reading from '%s', wrong parameters?",
				name);
#endif
			continue;
		}
		if(verbose>=2)
			print_sector("Read sector", buf, 512);
		break;
	}

	/* print error msg if needed */
	if ( dev->drive == 0 ){
		FREE(&Stream);
		fprintf(stderr,"%s: %s\n", argv[0],errmsg);
		exit(1);
	}

	if((used_dev.sectors || used_dev.heads) &&
	   (!used_dev.sectors || !used_dev.heads)) {
		fprintf(stderr,"You should either indicate both the number of sectors and the number of heads,\n");
		fprintf(stderr," or none of them\n");
		exit(1);
	}

	if(initialize) {
		if (bootSector) {
			int fd;
			fd = open(bootSector, O_RDONLY | O_BINARY | O_LARGEFILE);
			if (fd < 0) {
				perror("open MBR");
				exit(1);
			}
			if(read(fd, (char *) buf, 512) < 512) {
				perror("read MBR");
				exit(1);
			}
		}
		memset((char *)(partTable+1), 0, 4*sizeof(*partTable));
		set_word(((unsigned char*)buf)+510, 0xaa55);
	}

	/* check for boot signature, and place it if needed */
	if((buf[510] != 0x55) || (buf[511] != 0xaa)) {
		fprintf(stderr,"Boot signature not set\n");
		fprintf(stderr,
			"Use the -I flag to initialize the partition table, and set the boot signature\n");
		inconsistency = 1;
	}

	if(do_remove){
		if(!partTable[dev->partition].sys_ind)
			fprintf(stderr,
				"Partition for drive %c: does not exist\n",
				drive);
		if((partTable[dev->partition].sys_ind & 0x3f) == 5) {
			fprintf(stderr,
				"Partition for drive %c: may be an extended partition\n",
				drive);
			fprintf(stderr,
				"Use the -f flag to remove it anyways\n");
			inconsistency = 1;
		}
		memset(&partTable[dev->partition], 0, sizeof(*partTable));
	}

	if(create && partTable[dev->partition].sys_ind) {
		fprintf(stderr,
			"Partition for drive %c: already exists\n", drive);
		fprintf(stderr,
			"Use the -r flag to remove it before attempting to recreate it\n");
	}


	/* find out number of heads and sectors, and whether there is
	* any activated partition */
	has_activated = 0;
	for(i=1; i<5; i++){
		if(!partTable[i].sys_ind)
			continue;

		if(partTable[i].boot_ind)
			has_activated++;

		/* set geometry from entry */
		if (!used_dev.heads)
			used_dev.heads = head(partTable[i].end)+1;
		if(!used_dev.sectors)
			used_dev.sectors = sector(partTable[i].end);
		if(i<dev->partition && !begin_set)
			begin = END(partTable[i]);
		if(i>dev->partition && !end_set && !size_set) {
			end = BEGIN(partTable[i]);
			end_set = 1;
		}
	}

#ifdef OS_linux
	if(!used_dev.sectors && !used_dev.heads) {
		if(!IS_SCSI(dev)) {
			struct hd_geometry geom;
			if(ioctl(get_fd(Stream), HDIO_GETGEO, &geom) == 0) {
				used_dev.heads = geom.heads;
				used_dev.sectors = geom.sectors;
			}
		}
	}
#endif

	if(!used_dev.sectors && !used_dev.heads) {
		if(tot_sectors)
			setsize0(tot_sectors,&dummy2,&used_dev.heads,
				 &used_dev.sectors);
		else {
			used_dev.heads = 64;
			used_dev.sectors = 32;
		}
	}

	if(verbose)
		fprintf(stderr,"sectors: %d heads: %d %d\n",
			used_dev.sectors, used_dev.heads, tot_sectors);

	sec_per_cyl = used_dev.sectors * used_dev.heads;
	if(create) {
		if(!end_set && tot_sectors) {
			end = tot_sectors - tot_sectors % sec_per_cyl;
			end_set = 1;
		}

		/* if the partition starts right at the beginning of
		 * the disk, keep one track unused to allow place for
		 * the master boot record */
		if(!begin && !begin_set)
			begin = used_dev.sectors;
		if(!size_set && used_dev.tracks) {
			size_set = 2;
			length = sec_per_cyl * used_dev.tracks;

			/*  round the size in order to take
			 * into account any "hidden" sectors */

			/* do we anchor this at the beginning ?*/
			if(begin_set || dev->partition <= 2 || !end_set)
				length -= begin % sec_per_cyl;
			else if(end - length < begin)
				/* truncate any overlap */
				length = end - begin;
		}
		if(size_set) {
			if(!begin_set && dev->partition >2 && end_set)
				begin = end - length;
			else
				end = begin + length;
		} else if(!end_set) {
			fprintf(stderr,"Unknown size\n");
			exit(1);
		}

		setBeginEnd(&partTable[dev->partition], begin, end,
			    used_dev.heads, used_dev.sectors,
			    !has_activated, type,
			    dev->fat_bits);
	}

	if(activate) {
		if(!partTable[dev->partition].sys_ind) {
			fprintf(stderr,
				"Partition for drive %c: does not exist\n",
				drive);
		} else {
			switch(activate) {
				case 1:
					partTable[dev->partition].boot_ind=0x80;
					break;
				case -1:
					partTable[dev->partition].boot_ind=0x00;
					break;
			}
		}
	}


	inconsistency |= consistencyCheck(partTable, doprint, verbose,
					  &has_activated, &last_end, &j,
					  &used_dev, dev->partition);

	if(doprint && !inconsistency && partTable[dev->partition].sys_ind) {
		printf("The following command will recreate the partition for drive %c:\n",
		       drive);
		used_dev.tracks =
			(_DWORD(partTable[dev->partition].nr_sects) +
			 (BEGIN(partTable[dev->partition]) % sec_per_cyl)) /
			sec_per_cyl;
		printf("mpartition -c -t %d -h %d -s %d -b %u %c:\n",
		       used_dev.tracks, used_dev.heads, used_dev.sectors,
		       BEGIN(partTable[dev->partition]), drive);
	}

	if(tot_sectors && last_end >tot_sectors) {
		fprintf(stderr,
			"Partition %d exceeds beyond end of disk\n",
			j);
		exit(1);
	}


	switch(has_activated) {
		case 0:
			fprintf(stderr,
				"Warning: no active (bootable) partition present\n");
			break;
		case 1:
			break;
		default:
			fprintf(stderr,
				"Warning: %d active (bootable) partitions present\n",
				has_activated);
			fprintf(stderr,
				"Usually, a disk should have exactly one active partition\n");
			break;
	}

	if(inconsistency && !force) {
		fprintf(stderr,
			"inconsistency detected!\n" );
		if(dirty)
			fprintf(stderr,
				"Retry with the -f switch to go ahead anyways\n");
		exit(1);
	}

	if(dirty) {
		/* write data back to the disk */
		if(verbose>=2)
			print_sector("Writing sector", buf, 512);
		if (WRITES(Stream, (char *) buf, 0, 512) != 512) {
			fprintf(stderr,"Error writing partition table");
			exit(1);
		}
		if(verbose>=3)
			print_sector("Sector written", buf, 512);
	}
	FREE(&Stream);
	exit(0);
}
