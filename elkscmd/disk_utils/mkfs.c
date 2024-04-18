/*
 * mkfs.c - make a linux (minix) file-system.
 *
 * (C) 1991 Linus Torvalds. This file may be redistributed as per
 * the Linux copyright.
 */

/***************************************************************************
 *
 * 24.11.1991 -	time began. Used the fsck sources to get started.
 *
 * 25.11.1991 -	corrected some bugs. Added support for ".badblocks"
 *		The algorithm for ".badblocks" is a bit weird, but
 *		it should work. Oh, well.
 *
 * 25.01.1992 -	Added the -l option for getting the list of bad blocks
 *              out of a named file. (Dave Rivers, rivers@ponds.uucp)
 *
 * 28.02.1992 -	added %-information when using -c.
 *
 * 28.02.1993 -	added support for other namelengths than the original
 *		14 characters so that I can test the new kernel routines..
 *
 * 09.10.1993 -	make exit status conform to that required by fsutil
 *		<faith@cs.unc.edu>
 *
 * 31.10.1993 -	added inode request feature, for backup floppies: use
 *		32 inodes, for a news partition use more.
 *		Scott Heavner <sdh@po.cwru.edu>
 *
 * 03.01.1994 -	Added support for file system valid flag.
 *		Dr. Wettstein <greg%wind.uucp@plains.nodak.edu>
 *
 ***************************************************************************
 *
 * Usage:  mkfs [-c] [-nXX] [-iXX] device size-in-blocks
 *         mkfs [-l filename ] device size-in-blocks
 *
 * The device may be a block device or a image of one, but this isn't
 * enforced (but it's not much fun on a character device :-). 
 *
 */

#include <errno.h>

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/stat.h>

#include <linuxmt/fs.h>
#include <linuxmt/minix_fs.h>

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#define MINIX_ROOT_INO 1
#define MINIX_BAD_INO 2

#define TEST_BUFFER_BLOCKS 16
#define MAX_GOOD_BLOCKS 512

#define UPPER(size,n) ((size+((long)(n)-1))/(n))
#define INODE_SIZE (sizeof(struct minix_inode))
#define INODE_BLOCKS UPPER(INODES,MINIX_INODES_PER_BLOCK)
#define INODE_BUFFER_SIZE (INODE_BLOCKS * BLOCK_SIZE)

#define BITS_PER_BLOCK (BLOCK_SIZE<<3)

static char * device_name = NULL;
static int DEV = -1;
static long BLOCKS = 0;

static int dirsize = 16;
static int magic = MINIX_SUPER_MAGIC;
static unsigned int ikl;

static char root_block[BLOCK_SIZE];

static char * inode_buffer = NULL;
#define Inode (((struct minix_inode *) inode_buffer)-1)
static char super_block_buffer[BLOCK_SIZE];
#define Super (*(struct minix_super_block *)super_block_buffer)
#define INODES (Super.s_ninodes)
#define ZONES (Super.s_nzones)
#define IMAPS (Super.s_imap_blocks)
#define ZMAPS (Super.s_zmap_blocks)
#define FIRSTZONE (Super.s_firstdatazone)
#define ZONESIZE (Super.s_log_zone_size)
#define MAXSIZE (Super.s_max_size)
#define MAGIC (Super.s_magic)
#define NORM_FIRSTZONE (2+IMAPS+ZMAPS+INODE_BLOCKS)

/* Max of 64k zones and 64k inodes */
static char inode_map[BLOCK_SIZE * 8];
static char zone_map[BLOCK_SIZE * 8];

unsigned char set_bit(unsigned int nr, void * add)
{
	unsigned int * addr = add;
	int	mask, retval;

	addr += nr >> 4;
	mask = 1 << (nr & 0x0f);
	retval = (mask & *addr) != 0;
	*addr |= mask;
	return retval;
}

unsigned char clear_bit(unsigned int nr, void * add)
{
	unsigned int * addr = add;
	int	mask, retval;

	addr += nr >> 4;
	mask = 1 << (nr & 0x0f);
	retval = (mask & *addr) != 0;
	*addr &= ~mask;
	return retval;
}

unsigned char test_bit(unsigned int nr, void * add)
{
	unsigned int * addr = add;
	int	mask;

	addr += nr >> 4;
	mask = 1 << (nr & 0x0f);
	return ((mask & *addr) != 0);
}

#define bit(addr,bit) test_bit(bit,addr)
#define setbit(addr,bit) set_bit(bit,addr)
#define clrbit(addr,bit) clear_bit(bit,addr)

#define inode_in_use(x) (bit(inode_map,(x)))
#define zone_in_use(x) (bit(zone_map,(x)-FIRSTZONE+1))

#define mark_inode(x) (setbit(inode_map,(x)))
#define unmark_inode(x) (clrbit(inode_map,(x)))

#define mark_zone(x) (setbit(zone_map,(x)-FIRSTZONE+1))
#define unmark_zone(x) (clrbit(zone_map,(x)-FIRSTZONE+1))

void fatal_error(const char * fmt_string,int status)
{
	printf(fmt_string);
	exit(status);
}

#define usage() fatal_error("Usage: mkfs /dev/name blocks (Max blocks=65535)\n",16)
#define die(str) fatal_error("mkfs: " str "\n",8)

void write_tables(void)
{
	Super.s_state |= MINIX_VALID_FS;
	Super.s_state &= ~MINIX_ERROR_FS;

	if (BLOCK_SIZE != lseek(DEV, (long)BLOCK_SIZE, SEEK_SET)) {
		die("seek failed in write_tables");
	}
	if (BLOCK_SIZE != write(DEV, super_block_buffer, BLOCK_SIZE))
		die("unable to write super-block");
	if (IMAPS*BLOCK_SIZE != write(DEV,inode_map,IMAPS*BLOCK_SIZE))
		die("Unable to write inode map");
	if (ZMAPS*BLOCK_SIZE != write(DEV,zone_map,ZMAPS*BLOCK_SIZE))
		die("Unable to write zone map");
	if (BLOCK_SIZE != write(DEV,inode_buffer,BLOCK_SIZE))
		die("Unable to write inodes");
	for (ikl=1;ikl<INODE_BLOCKS;ikl++) {
		if (BLOCK_SIZE != write(DEV,inode_buffer + BLOCK_SIZE,BLOCK_SIZE))
			die("Unable to write inodes");
	}
}
 
void write_block(int blk, char * buffer)
{
	unsigned long seek_point = (long)((long)blk * BLOCK_SIZE);

/*	printf("Seeking in user space to %ld\n",seek_point); */
/*	printf("Seeked to %ld\n",lseek(DEV, seek_point, SEEK_SET)); */
	if (seek_point != lseek(DEV, seek_point, SEEK_SET)) {
		die("seek failed in write_block");
	}
	if (BLOCK_SIZE != write(DEV, buffer, BLOCK_SIZE))
		die("write failed in write_block");
}

int get_free_block(void)
{
	int blk;

	blk = FIRSTZONE;
	while (blk < ZONES && zone_in_use(blk))
		blk++;
	if (blk >= ZONES)
		die("not enough good blocks");
	mark_zone(blk);
	return blk;
}

void make_root_inode(void)
{
	struct minix_inode * inode = &Inode[MINIX_ROOT_INO];

	mark_inode(MINIX_ROOT_INO);
	inode->i_zone[0] = get_free_block();
	inode->i_nlinks = 2;
	inode->i_time = time(NULL);
	root_block[2*dirsize] = '\0';
	root_block[2*dirsize+1] = '\0';
	inode->i_size = 2*dirsize;
	inode->i_mode = S_IFDIR + 0755;
	write_block(inode->i_zone[0],root_block);
}

void setup_tables(void)
{
	unsigned i;

	memset(inode_map,0xff,sizeof(inode_map));
	memset(zone_map,0xff,sizeof(zone_map));
	memset(super_block_buffer,0,BLOCK_SIZE);
	MAGIC = magic;
	ZONESIZE = 0;
	/* volume limit is 7+512+512L*512L but set max_size to blocks in fs*/
	MAXSIZE = BLOCKS*1024L;
	ZONES = BLOCKS;
	INODES = BLOCKS/3;
#if 0 /* FIXME: add ability to specify inode count rather than guess*/
	if ( BLOCKS > 32768L ) INODES += (BLOCKS-32768)*4/3;
	if ( INODES > 63424L ) INODES = 63424L;
	if ((INODES & 8191) > 8188)
		INODES -= 5;
	if ((INODES & 8191) < 10)
		INODES -= 20;
#endif
	if (INODES > 32736)		/* max inodes to fit in 4 imap blocks*/
		INODES = 32736;
	IMAPS = UPPER(INODES,BITS_PER_BLOCK);
	ZMAPS = 0;
	while (ZMAPS != UPPER(BLOCKS - NORM_FIRSTZONE,BITS_PER_BLOCK))
		ZMAPS = UPPER(BLOCKS - NORM_FIRSTZONE,BITS_PER_BLOCK);
	FIRSTZONE = NORM_FIRSTZONE;
	for (i = FIRSTZONE ; i<ZONES ; i++)
		unmark_zone(i);
	for (i = MINIX_ROOT_INO ; i<INODES ; i++)
		unmark_inode(i);
	inode_buffer = malloc(2048);
	if (!inode_buffer)
		die("Unable to allocate buffer for inodes");
	memset(inode_buffer,0,2048);
	printf("%u inodes\n",INODES);
	printf("%u blocks\n",ZONES);
	printf("Firstdatazone=%d (%ld)\n",FIRSTZONE,NORM_FIRSTZONE);
	printf("Zonesize=%d\n",BLOCK_SIZE<<ZONESIZE);
	printf("Maxsize=%ld\n\n",MAXSIZE);
}


int main(int argc, char ** argv)
{
	char * tmp;
	struct stat statbuf;

	if (INODE_SIZE * MINIX_INODES_PER_BLOCK != BLOCK_SIZE)
		die("bad inode size");

	if ((argc == 3) && (argv[1][0] != '-') && (argv[2][0] != '-')) {
		BLOCKS = strtol(argv[2],&tmp,0);
		if (*tmp) {
			usage();
		}
		device_name = argv[1];
	} else
		usage();
	if (!device_name || BLOCKS<10L || BLOCKS > 65535L) {
		usage();
	}
	tmp = root_block;
	tmp[0] = 1;
	tmp[1] = 0;
	strcpy(tmp+2,".");
	tmp += dirsize;
	tmp[0] = 1;
	tmp[1] = 0;
	strcpy(tmp+2,"..");
	tmp += dirsize;
	tmp[0] = 2;
	tmp[1] = 0;
	strcpy(tmp+2,".badblocks");
	DEV = open(device_name,O_RDWR );
	if (DEV<0)
		die("unable to open device");
	if (fstat(DEV,&statbuf)<0)
		die("unable to stat %s");
	/***if (statbuf.st_rdev == DEV_FD0 || statbuf.st_rdev == DEV_HDA)
		die("Will not try to make filesystem on '%s'");***/
	if ((BLOCKS << 10) > statbuf.st_size) {
		printf("Requested block count %luK exceeds device size %luK\n",
			BLOCKS, statbuf.st_size >> 10);
		exit(8);
	}
	setup_tables();
	make_root_inode();
	write_tables();
	sync();
	return 0;
}
