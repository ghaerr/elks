/*
 * fsck.c - a file system consistency checker for Linux.
 *
 * (C) 1991, 1992 Linus Torvalds. This file may be redistributed
 * as per the GNU copyleft.
 */

/*
 * 09.11.91  -  made the first rudimetary functions
 *
 * 10.11.91  -  updated, does checking, no repairs yet.
 *		Sent out to the mailing-list for testing.
 *
 * 14.11.91  -	Testing seems to have gone well. Added some
 *		correction-code, and changed some functions.
 *
 * 15.11.91  -  More correction code. Hopefully it notices most
 *		cases now, and tries to do something about them.
 *
 * 16.11.91  -  More corrections (thanks to Mika Jalava). Most
 *		things seem to work now. Yeah, sure.
 *
 *
 * 19.04.92  -	Had to start over again from this old version, as a
 *		kernel bug ate my enhanced fsck in february.
 *
 * 28.02.93  -	added support for different directory entry sizes..
 *
 * Sat Mar  6 18:59:42 1993, faith@cs.unc.edu: Output namelen with
 *                           super-block information
 *
 * Sat Oct  9 11:17:11 1993, faith@cs.unc.edu: make exit status conform
 *                           to that required by fsutil
 *
 * Mon Jan  3 11:06:52 1994 - Dr. Wettstein (greg%wind.uucp@plains.nodak.edu)
 *			      Added support for file system valid flag.  Also
 *			      added program_version variable and output of
 *			      program name and version number when program
 *			      is executed.
 *
 * Tue Sep  1 2020 - Greg Haerr fixed fsck to work on ELKS
 * Fri Aug 25 2023 - Greg Haerr improved formatting for -lvv and -lvvv output
 *
 * I've had no time to add comments - hopefully the function names
 * are comments enough. As with all file system checkers, this assumes
 * the file system is quiescent - don't use it on a mounted device
 * unless you can be sure nobody is writing to it (and remember that the
 * kernel can write to it when it searches for files).
 *
 * Usuage: fsck [-larvsm] device
 *	-l for a listing of all the filenames
 *	-a for automatic repairs
 *	-r for repairs (interactive)
 *	-v for verbose (tells how many files)
 *	-s for super-block info
 *	-m for minix-like "mode not cleared" warnings
 *	-f force filesystem check even if filesystem marked as valid
 *
 * The device may be a block device or a image of one, but this isn't
 * enforced (but it's not much fun on a character device :-).
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/stat.h>

#include <linuxmt/fs.h>
#include <linuxmt/minix_fs.h>

#define printd(...)
/*#define printd		printf*/

/* FIXME remove all commented out code with RUBOUT in it */

#define ROOT_INO 1

#define UPPER(size,n) ((size+((n)-1))/(n))

#define BITS_PER_BLOCK (BLOCK_SIZE<<3)

static char * program_name = "fsck.minix";
/*static char * program_version = "1.0.1 - 09/01/20";*/
static char * device_name = NULL;
static int IN;
static int repair=0, automatic=0, verbose=0, list=0, show=0, warn_mode=0,
	force=0;
static int directory=0, regular=0, blockdev=0, chardev=0, links=0,
		symlinks=0, total=0;

static int changed = 0; /* flags if the filesystem has been changed */
static int errors_uncorrected = 0; /* flag if some error was not corrected */
/* static int dirsize = 16; RUBOUT */
/* static int namelen = 14; RUBOUT */

#define DIRSIZE 16
#define NAMELEN 14

/* File-name data */
#define MAX_DEPTH 50
static int name_depth = 0;
static char name_list[MAX_DEPTH][NAMELEN+1];


/*static char * inode_buffer = NULL; */
static char inode_buffer[BLOCK_SIZE];
/*#define Inode (((struct minix_inode *) inode_buffer)-1)*/
static char super_block_buffer[BLOCK_SIZE];
struct minix_super_block * SupeP = (struct minix_super_block *)super_block_buffer;
#define Super (*(struct minix_super_block *)super_block_buffer)
/* #define SupeP ((struct minix_super_block *)super_block_buffer) */
#define INODES (SupeP->s_ninodes)
#define ZONES (SupeP->s_nzones)
#define IMAPS (SupeP->s_imap_blocks)
#define ZMAPS (SupeP->s_zmap_blocks)
#define FIRSTZONE (SupeP->s_firstdatazone)
#define ZONESIZE (SupeP->s_log_zone_size)
#define MAXSIZE (SupeP->s_max_size)
#define MAGIC (SupeP->s_magic)
#define INODE_SIZE (sizeof(struct minix_inode))
#define INODE_BLOCKS UPPER(INODES,MINIX_INODES_PER_BLOCK)
#define INODE_BUFFER_SIZE (INODE_BLOCKS * BLOCK_SIZE)
#define NORM_FIRSTZONE (2+IMAPS+ZMAPS+INODE_BLOCKS)

static char inode_map[BLOCK_SIZE * MINIX_I_MAP_SLOTS];
static char zone_map[BLOCK_SIZE * MINIX_Z_MAP_SLOTS];

static unsigned char * inode_count = NULL;
static unsigned char __far * zone_count = NULL;

void recursive_check(unsigned int ino);

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
	unsigned int	mask;
	int retval;

	addr += nr >> 4;
	mask = 1 << (nr & 0x0f);
	retval = ((mask & *addr) != 0L);
	printd("test_bit %u at %x, word no %u is %x testing bit %u, result %d\n",
		nr, (unsigned int)add, (nr >> 4), *addr, (nr & 0x0f), retval);
	return retval;
}

/*
#define bit(addr,bit) (test_bit((bit)^15,(addr)) != 0)
#define setbit(addr,bit) (set_bit((bit)^15,(addr)) != 0)
#define clrbit(addr,bit) (clear_bit((bit)^15,(addr)) != 0)
*/

#define bit(addr,bit) (test_bit((bit),(addr)) != 0)
#define setbit(addr,bit) set_bit((bit),(addr))
#define clrbit(addr,bit) clear_bit((bit),(addr))


#define inode_in_use(x) (bit(inode_map,(x)))
#define zone_in_use(x) (bit(zone_map,(x)-FIRSTZONE+1))

#define mark_inode(x) (setbit(inode_map,(x)),changed=1)
#define unmark_inode(x) (clrbit(inode_map,(x)),changed=1)

#define mark_zone(x) (setbit(zone_map,(x)-FIRSTZONE+1),changed=1)
#define unmark_zone(x) (clrbit(zone_map,(x)-FIRSTZONE+1),changed=1)

/*
 * Volatile to let gcc know that this doesn't return. When trying
 * to compile this under minix, volatile gives a warning, as
 * exit() isn't defined as volatile under minix.
 */
/*volatile*/ void fatal_error(const char * fmt_string, int status)
{
	fprintf(stderr,fmt_string,program_name,device_name);
	exit(status);
}

#define usage() fatal_error("Usage: %s [-larvsmf] /dev/name\n",16)
#define die(str) fatal_error("%s: " str "\n",8)

int mapped_block = -1; /* Block in the inode area that is mapped */
int mapped_dirty = 0;  /* non-zero indicates inode block has been altered */

void unmap_inode_buffer()
/* Write the inode block in inode_buffer back to disk if it is dirty.
 */
{
	printd("Unmap requested: ");
	if (mapped_dirty == 0) {	/* Nothing to do. */
		printd("not dirty\n");
		return;
	}
	if (mapped_block < 0) {	/* Nothing to do. */
		printd("none mapped\n");
		return;
	}
	/* Seek to after the Superblock, inode maps and zone maps */
	lseek(IN, (off_t)((2 + IMAPS + ZMAPS) * BLOCK_SIZE), SEEK_SET);
	/* Seek to the block we have to write */
	if (mapped_block > 1) {
		lseek(IN, (off_t)((mapped_block) * BLOCK_SIZE), SEEK_CUR);
	}
	if (BLOCK_SIZE != write(IN, inode_buffer, BLOCK_SIZE)) {
		die("Unable to write inodes");
	}
	mapped_dirty = 0;
	printd("Block %d unmapped.", mapped_block);
}

void inode_dump(struct minix_inode * ptr)
{
	int i;

	printf(" %5ld %d/%d Z", ptr->i_size, ptr->i_uid, ptr->i_gid);
	for (i = 0; i < 9; i++) {
		if (ptr->i_zone[i] || verbose > 2)
			printf(S_ISBLK(ptr->i_mode) || S_ISCHR(ptr->i_mode)? " 0x%04x": " %u",
				ptr->i_zone[i]);
		else break;
	}
}


struct minix_inode * map_inode(unsigned int nr)
/* Take an inode number and map the 1024 byte block on disk containing this
 * inode into the inode memory area inode_buffer, returning a pointer to
 * the inode.
 */
{
	int inode_block; 	/* Block in the inode area we have to map */
	int inode_offset;	/* Inode offset in current block */
	struct minix_inode * inode_ptr;

	if (!nr || nr >= INODES)
		return NULL;
	nr--;
	inode_block = nr >> 5;
	inode_offset = nr & 31;
	printd("Map of inode %d requested which is no %d in block %d\n",
		nr, inode_offset, inode_block);
	inode_ptr = ((struct minix_inode *)inode_buffer) + inode_offset;
	if (mapped_block != -1) {
		/* CHECK IF IT IS THE BLOCK WE WANT */
		if (inode_block == mapped_block) {
			/* If so return pointer */
			printd("Block was already mapped.\n");
			return inode_ptr;
		}
		/* WRITE OLD INODE BLOCK BACK TO DISK */
		unmap_inode_buffer();
	}
	/* Seek to after the Superblock, inode maps and zone maps */
	lseek(IN, (off_t)((2 + IMAPS + ZMAPS) * BLOCK_SIZE), SEEK_SET);
	/* Seek to the block we have to write */
	if (inode_block > 0) {
		lseek(IN, (off_t)((inode_block) * BLOCK_SIZE), SEEK_CUR);
	}
	/* READ In NEW BLOCK FROM DISK */
	if (BLOCK_SIZE != read(IN, inode_buffer, BLOCK_SIZE)) {
		die("Unable to read inodes");
	}
	/* SET mapped */
	mapped_block = inode_block;
	/* RETURN POINTER */
	return inode_ptr;
}

/*
 * This simply goes through the file-name data and prints out the
 * current file.
 */

int print_current_name(void)
{
	int i, count = 0;

	for(i = 0; i < name_depth;)
		count += printf("/%.14s",name_list[i++]);
	return count;
}

int ask(const char * string, int def)
{
	int c;

	if (!repair) {
		printf("\n");
		errors_uncorrected = 1;
		return 0;
	}
	if (automatic) {
		printf("\n");
		if (!def)
		      errors_uncorrected = 1;
		return def;
	}
	printf(def?"%s (y/n)? ":"%s (n/y)? ",string);
	for (;;) {
		fflush(stdout);
		if ((c = getchar()) == EOF) {
		        if (!def)
			      errors_uncorrected = 1;
			return def;
		}
		c=toupper(c);
		if (c == 'Y') {
			def = 1;
			break;
		} else if (c == 'N') {
			def = 0;
			break;
		} else if (c == 'A') {	/* Hidden option; switch to automatic */
			automatic = 1;
			def = 1;
			break;
		} else if (c == ' ' || c == '\n')
			break;
	}
	if (def)
		printf("y\n");
	else {
		printf("n\n");
		errors_uncorrected = 1;
	     }
	return def;
}

/*
 * check_zone_nr checks to see that *nr is a valid zone nr. If it
 * isn't, it will possibly be repaired. Check_zone_nr sets *corrected
 * if an error was corrected, and returns the zone (0 for no zone
 * or a bad zone-number).
 */
int check_zone_nr(unsigned short * nr, int * corrected)
{
	if (!*nr)
		return 0;
	if (*nr < FIRSTZONE)
		printf("Zone nr < FIRSTZONE in file `");
	else if (*nr >= ZONES)
		printf("Zone nr >= ZONES in file `");
	else
		return *nr;
	print_current_name();
	printf("'.");
	if (ask("Remove block", 1)) {
		*nr = 0;
		*corrected = 1;
	}
	return 0;
}

/*
 * read-block reads block nr into the buffer at addr.
 */
void read_block(unsigned int nr, char * addr)
{
	int i;
	off_t dest = (off_t)BLOCK_SIZE * nr;

	printd("read_block %x %lx ", nr, dest);
	if (!nr) {
		memset(addr,0,BLOCK_SIZE);
		return;
	}
	if (dest != lseek(IN, dest, SEEK_SET)) {
		printf("Read error: unable to seek to block in file '");
		print_current_name();
		printf("'\n");
		memset(addr,0,BLOCK_SIZE);
		errors_uncorrected = 1;
	} else {
		int n = read(IN, addr, BLOCK_SIZE);
		if (n != BLOCK_SIZE) {
			printf("Read error: bad block %d error %d in file '", nr, n);
			print_current_name();
			printf("'\n");
			memset(addr,0,BLOCK_SIZE);
			errors_uncorrected = 1;
		}
	}
	for (i = 0; i < 16; i++) {
		printd("%x ", addr[i]);
	}
	printd("\n");
}

/*
 * write_block writes block nr to disk.
 */
void write_block(unsigned int nr, char * addr)
{
	off_t dest = (off_t)BLOCK_SIZE * nr;
	if (!nr)
		return;
	if (nr < FIRSTZONE || nr >= ZONES) {
		printf("Internal error: trying to write bad block\n"
		"Write request ignored\n");
		errors_uncorrected = 1;
		return;
	}
	if (dest != lseek(IN, dest, SEEK_SET))
		die("seek failed in write_block");
	if (BLOCK_SIZE != write(IN, addr, BLOCK_SIZE)) {
		printf("Write error: bad block in file '");
		print_current_name();
		printf("'\n");
		errors_uncorrected = 1;
	}
}

/*
 * map-block calculates the absolute block nr of a block in a file.
 * It sets 'changed' if the inode has needed changing, and re-writes
 * any indirect blocks with errors.
 */
unsigned int map_block(struct minix_inode * inode, unsigned int blknr)
{
	unsigned short ind[BLOCK_SIZE>>1];
	unsigned short dind[BLOCK_SIZE>>1];
	int blk_chg, block, result;
	int i;

	printd("map_block %x %u (", (unsigned int)inode, blknr);
	for (i = 0; i < 7; i++) {
		printd("%x ", inode->i_zone[i]);
	}
	if (blknr < 7) {
		printd(")direct\n");
		return check_zone_nr(inode->i_zone + blknr, &changed);
	}
	blknr -= 7;
	if (blknr < 512) {
		printd(")ind\n");
		block = check_zone_nr(inode->i_zone + 7, &changed);
		read_block(block, (char *) ind);
		blk_chg = 0;
		result = check_zone_nr(blknr + ind, &blk_chg);
		if (blk_chg)
			write_block(block, (char *) ind);
		return result;
	}
	printd(")dind\n");
	blknr -= 512;
	block = check_zone_nr(inode->i_zone + 8, &changed);
	read_block(block, (char *) dind);
	blk_chg = 0;
	result = check_zone_nr(dind + (blknr/512), &blk_chg);
	if (blk_chg)
		write_block(block, (char *) dind);
	block = result;
	read_block(block, (char *) ind);
	blk_chg = 0;
	result = check_zone_nr(ind + (blknr%512), &blk_chg);
	if (blk_chg)
		write_block(block, (char *) ind);
	return result;
}

void write_super_block(void)
{
	/*
	 * Set the state of the filesystem based on whether or not there
	 * are uncorrected errors.  The filesystem valid flag is
	 * unconditionally set if we get this far.
	 */
	SupeP->s_state |= MINIX_VALID_FS;
	if (errors_uncorrected)
		SupeP->s_state |= MINIX_ERROR_FS;
	else
		SupeP->s_state &= ~MINIX_ERROR_FS;

	if ((off_t)BLOCK_SIZE != lseek(IN, (off_t)BLOCK_SIZE, SEEK_SET))
		die("seek failed in write_super_block");
	if (BLOCK_SIZE != write(IN, super_block_buffer, BLOCK_SIZE))
		die("unable to write super-block");

	return;
}

void write_tables(void)
{
	write_super_block();

	if ((IMAPS * BLOCK_SIZE) != write(IN, inode_map, (IMAPS * BLOCK_SIZE)))
		die("Unable to write inode map");
	if ((ZMAPS * BLOCK_SIZE) != write(IN, zone_map, (ZMAPS * BLOCK_SIZE)))
		die("Unable to write zone map");
	unmap_inode_buffer();
/*	if (INODE_BUFFER_SIZE != write(IN, inode_buffer, (INODE_BUFFER_SIZE))
		die("Unable to write inodes"); RUBOUT */
}

void read_tables(void)
{
/*	unsigned int inode_buffer_size = INODE_BUFFER_SIZE; RUBOUT */

	memset(inode_map,0,sizeof(inode_map));
	memset(zone_map,0,sizeof(zone_map));
	if ((off_t)BLOCK_SIZE != (off_t)lseek(IN, (off_t)BLOCK_SIZE, SEEK_SET))
		die("seek failed");
	if (BLOCK_SIZE != read(IN, super_block_buffer, BLOCK_SIZE))
		die("unable to read super block");
	if (MAGIC == MINIX_SUPER_MAGIC) {
/*		namelen = 14; RUBOUT */
/*		dirsize = 16; RUBOUT */
	} else {
		die("not a MINIX filesystem (perhaps DOS?)");
	}
	/*else if (MAGIC == MINIX_SUPER_MAGIC2) { RUBOUT
		namelen = 30;
		dirsize = 32;
	} else */
	if (ZONESIZE != 0/* || BLOCK_SIZE != 1024*/)
		die("Only 1k blocks/zones supported");
	if (!IMAPS || IMAPS > MINIX_I_MAP_SLOTS)
		die("bad s_imap_blocks field in super-block");
	if (!ZMAPS || ZMAPS > MINIX_Z_MAP_SLOTS)
		die("bad s_zmap_blocks field in super-block");
/*	inode_buffer = malloc(BLOCK_SIZE); RUBOUT
	if (!inode_buffer)
		die("Unable to allocate buffer for inodes"); RUBOUT */

	printd("inodes %d zones %d\n", INODES, ZONES);
	printd("imaps %d zmaps %d\n", IMAPS, ZMAPS);
	inode_count = malloc(INODES);
	if (!inode_count)
		die("Unable to allocate buffer for inode count");
	zone_count = fmemalloc(ZONES);  /* ZONES <= 64K */
	if (!zone_count) {
		fprintf(stderr, "ZONES: %u -- ", ZONES);
		die("Unable to allocate main memory for zone count");
	}
	if ((IMAPS * BLOCK_SIZE) != read(IN, inode_map, (IMAPS * BLOCK_SIZE)))
		die("Unable to read inode map");
	if ((ZMAPS * BLOCK_SIZE) != read(IN, zone_map, (ZMAPS * BLOCK_SIZE)))
		die("Unable to read zone map");
/*	if (inode_buffer_size != (foo = read(IN, inode_buffer, inode_buffer_size))) {
		die("Unable to read inodes");
	} RUBOUT */
	if (NORM_FIRSTZONE != FIRSTZONE) {
		printf("Warning: Firstzone != Norm_firstzone\n");
		errors_uncorrected = 1;
	}
	if (show) {
		printf("%u inodes\n",INODES);
		printf("%u blocks\n",ZONES);
		printf("Pre-zone blocks 2+%d+%d+%d %d\n", IMAPS, ZMAPS, INODE_BLOCKS, NORM_FIRSTZONE);
		printf("Firstdatazone=%d (%d)\n",FIRSTZONE,NORM_FIRSTZONE);
		printf("Zonesize=%d\n",BLOCK_SIZE<<ZONESIZE);
		printf("Maxsize=%ld bytes\n",MAXSIZE);
		printf("Filesystem state=%d\n", SupeP->s_state);
		printf("namelen=14\n\n"); /* RUBOUT */
	}
}

struct minix_inode * get_inode(unsigned int nr)
{
	struct minix_inode * inode;

	if (!nr || nr >= INODES)
		return NULL;
	total++;
/*	inode = Inode + nr; RUBOUT */
	inode = map_inode(nr);
	if (!inode_count[nr]) {
		if (!inode_in_use(nr)) {
			printf("Inode %d marked not used, but used for file '",
				nr);
			print_current_name();
			printf("'\n");
			if (repair) {
				if (ask("Mark in use",1))
					mark_inode(nr);
				else
			        errors_uncorrected = 1;
			}
		}
		if (S_ISDIR(inode->i_mode))
			directory++;
		else if (S_ISREG(inode->i_mode))
			regular++;
		else if (S_ISCHR(inode->i_mode))
			chardev++;
		else if (S_ISBLK(inode->i_mode))
			blockdev++;
		else if (S_ISLNK(inode->i_mode))
			symlinks++;
		else if (S_ISSOCK(inode->i_mode))
			;
		else if (S_ISFIFO(inode->i_mode))
			;
		else {
                        print_current_name();
                        printf(" has mode %06o\n",inode->i_mode);
                }

	} else
		links++;
	if (!++inode_count[nr]) {
		printf("Warning: inode count too big.\n");
		inode_count[nr]--;
		errors_uncorrected = 1;
	}
	return inode;
}

void check_root(void)
{
/*	struct minix_inode * inode = Inode + ROOT_INO; RUBOUT */
	struct minix_inode * inode = map_inode(ROOT_INO);

	if (!inode || !S_ISDIR(inode->i_mode))
		die("root inode isn't a directory");
	printd("Root inode is a directory.\n");
}

static int add_zone(unsigned short * znr, int * corrected)
{
	int block;

	printd("add_zone %d\n", * znr);
	block = check_zone_nr(znr, corrected);
	if (!block)
		return 0;
	if (zone_count[block]) {
		printf("Block %u has been used before. Now in file `", block);
		print_current_name();
		printf("'.");
		if (ask("Clear", 1)) {
			*znr = 0;
			block = 0;
			*corrected = 1;
		}
	}
	if (!block)
		return 0;
	if (!zone_in_use(block)) {
		printf("Block %u in file `", block);
		print_current_name();
		printf("' is marked not in use. ");
		if (ask("Correct",1))
			mark_zone(block);
	}
	if (!++zone_count[block])
		zone_count[block]--;
	return block;
}

static void add_zone_ind(unsigned short * znr, int * corrected)
{
	static char blk[BLOCK_SIZE];
	int i, chg_blk=0;
	int block;

	printd("add_zone_ind %d\n", * znr);
	block = add_zone(znr, corrected);
	if (!block)
		return;
	read_block(block, blk);
	for (i=0 ; i < (BLOCK_SIZE>>1) ; i++)
		add_zone(i + (unsigned short *) blk, &chg_blk);
	if (chg_blk)
		write_block(block, blk);
}

static void add_zone_dind(unsigned short * znr, int * corrected)
{
	static char blk[BLOCK_SIZE];
	int i, blk_chg=0;
	int block;

	printd("add_zone_dind %d\n", * znr);
	block = add_zone(znr, corrected);
	if (!block)
		return;
	read_block(block, blk);
	for (i=0 ; i < (BLOCK_SIZE>>1) ; i++)
		add_zone_ind(i + (unsigned short *) blk, &blk_chg);
	if (blk_chg)
		write_block(block, blk);
}

void check_zones(unsigned int i)
{
	struct minix_inode * inode;

	printd("check_zones\n");
	if (!i || i >= INODES)
		return;
	if (inode_count[i] > 1)	/* have we counted this file already? */
		return;
/*	inode = Inode + i; RUBOUT */
	inode = map_inode(i);
	if (!S_ISDIR(inode->i_mode) && !S_ISREG(inode->i_mode) &&
	    !S_ISLNK(inode->i_mode))
		return;
	for (i=0 ; i<7 ; i++)
		add_zone(i + inode->i_zone, &changed);
	add_zone_ind(7 + inode->i_zone, &changed);
	add_zone_dind(8 + inode->i_zone, &changed);
	printd("check_zones_end\n");
}

void check_file(struct minix_inode * dir, unsigned long offset)
{
	static char blk[BLOCK_SIZE];
	struct minix_inode * inode;
	int ino, width;
	char * name;
	unsigned int block;

	block = map_block(dir,(unsigned int)(offset >> 10));
	read_block(block, blk);
	name = blk + (offset % BLOCK_SIZE);
	ino = * (unsigned short *) (name);
	name += 2;
/*	ino = * (unsigned short *) blk; */
	printd("check_file %x %lx ",ino, offset);
	printd("\n");
	if (ino >= INODES) {
		print_current_name();
		printf(" contains a bad inode number %u for file '", ino);
		printf("%.*s'.",14,name); /* FIXME 14 can be integrated */
		if (ask(" Remove",1)) {
			*(unsigned short *)(name-2) = 0;
			write_block(block, blk);
		}
		ino = 0;
	}
	if (name_depth < MAX_DEPTH)
		strncpy(name_list[name_depth],name,NAMELEN);
	name_depth++;
	inode = get_inode(ino);
	name_depth--;
	if (offset == 0L) {
		if (!inode || strcmp(".",name)) {
			print_current_name();
			printf(": bad directory: '.' isn't first\n");
			errors_uncorrected = 1;
		} else return;
	}
	if (offset == (unsigned long)DIRSIZE) {
		if (!inode || strcmp("..",name)) {
			print_current_name();
			printf(": bad directory: '..' isn't second\n");
			errors_uncorrected = 1;
		} else return;
	}
	if (!inode)
		return;
	if (name_depth < MAX_DEPTH)
		strncpy(name_list[name_depth],name,NAMELEN);
	name_depth++;
	if (list) {
		if (verbose)
			printf("%5d %06o %2d ", ino, inode->i_mode, inode->i_nlinks);
		width = print_current_name();
		if (verbose > 1) {
			while (width++ < 24) printf(" ");
			inode_dump(inode);
		}
		if (S_ISDIR(inode->i_mode) && verbose < 2)
			printf(":\n");
		else
			printf("\n");
	}
	check_zones(ino);
	if (inode && S_ISDIR(inode->i_mode))
		recursive_check(ino);
	name_depth--;
	return;
}

void recursive_check(unsigned int ino)
{
	struct minix_inode * dir;
	struct minix_inode mdir;
	unsigned long offset;

	printd("recursive_check %d\n", ino);
/*	dir = Inode + ino; */
	dir = map_inode(ino);
	memcpy(&mdir, dir, sizeof(struct minix_inode));
	dir = &mdir;
	if (!S_ISDIR(dir->i_mode))
		die("internal error");
	if (dir->i_size < 32L) {
		print_current_name();
		printf(": bad directory: size<32");
		errors_uncorrected = 1;
	}
	for (offset = 0 ; offset < dir->i_size ; offset += DIRSIZE)
		check_file(dir,offset);
}

int bad_zone(int i)
{
	char buffer[1024];
	off_t dest = (off_t)BLOCK_SIZE * i;

	printd("bad_zone %d\n", i);
	if (dest != lseek(IN, dest, SEEK_SET))
		die("seek failed in bad_zone");
	return (BLOCK_SIZE != read(IN, buffer, BLOCK_SIZE));
}

void check_counts(void)
{
	int i;

	printd("check_counts\n");
	for (i=1 ; i < INODES ; i++) {
		struct minix_inode * inode = map_inode(i);
		if (!inode_in_use(i) && inode->i_mode && warn_mode) {
			printf("Inode %d mode not cleared.",i);
			if (ask(" Clear",1)) {
				inode->i_mode = 0;
				mapped_dirty++;
				changed = 1;
			}
		}
		if (!inode_count[i]) {
			if (!inode_in_use(i))
				continue;
			printf("Inode %d not used, marked used in the bitmap.",i);
			if (ask(" Clear",1))
				unmark_inode(i);
			continue;
		}
		if (!inode_in_use(i)) {
			printf("Inode %d used, marked unused in the bitmap.",
				i);
			if (ask(" Set",1))
				mark_inode(i);
		}
		if (inode->i_nlinks != inode_count[i]) {
			printf("Inode %d (mode = %07o), i_nlinks=%d, counted=%d.",
				i,inode->i_mode,inode->i_nlinks,inode_count[i]);
			if (ask(" Set i_nlinks to count",1)) {
				inode->i_nlinks=inode_count[i];
				mapped_dirty++;
				changed=1;
			}
		}
	}
	for (i=FIRSTZONE ; i < ZONES ; i++) {
		if (zone_in_use(i) == zone_count[i])
			continue;
		if (!zone_count[i]) {
			if (bad_zone(i))
				continue;
			printf("Zone %d: marked in use, no file uses it.",i);
			if (ask(" Unmark",1))
				unmark_zone(i);
			continue;
		}
		printf("Zone %d: %sin use, counted=%d\n",
		i,zone_in_use(i)?"":"not ",zone_count[i]);
	}
}

void check(void)
{
	printd("check\n");
	memset(inode_count,0,INODES*sizeof(*inode_count));
	fmemset(zone_count,0,ZONES*sizeof(*zone_count));
	check_zones(ROOT_INO);
	if (verbose > 1) {
		struct minix_inode *ptr = map_inode(ROOT_INO);
		printf("%5d %06o %2d /%23s", ROOT_INO, ptr->i_mode, ptr->i_nlinks, " ");
		inode_dump(ptr);
		printf("\n");
	}
	recursive_check(ROOT_INO);
	check_counts();
}

int main(int argc, char ** argv)
{
	int count;
	int retcode = 0;
	struct termios termios,tmp;

	if (argc && *argv)
		program_name = *argv;
	if (INODE_SIZE * MINIX_INODES_PER_BLOCK != BLOCK_SIZE)
		die("bad inode size");
	while (argc-- > 1) {
		argv++;
		if (argv[0][0] != '-')
			if (device_name)
				usage();
			else
				device_name = argv[0];
		else while (*++argv[0])
			switch (argv[0][0]) {
				case 'l': list=1; break;
				case 'a': automatic=1; repair=1; break;
				case 'r': repair=1; break;
				case 'v': verbose++; break;
				case 's': show=1; break;
				case 'm': warn_mode=1; break;
				case 'f': force=1; break;
				default: usage();
			}
	}
	if (!device_name)
		usage();
	if (repair && !automatic) {
		if (!isatty(0) || !isatty(1))
			die("need terminal for interactive repairs");
		tcgetattr(0,&termios);
		tmp = termios;
		tmp.c_lflag &= ~(ICANON|ECHO);
		tcsetattr(0,TCSANOW,&tmp);
	}
	IN = open(device_name,repair?O_RDWR:O_RDONLY);
	if (IN < 0)
		die("unable to open '%s'");
	/*printf("%s %s\n", program_name, program_version);*/
	for (count=0 ; count<3 ; count++)
		sync();
	read_tables();

	/*
	 * Determine whether or not we should continue with the checking.
	 * This is based on the status of the filesystem valid and error
	 * flags and whether or not the -f switch was specified on the
	 * command line.
	 */
	if (!(SupeP->s_state & MINIX_ERROR_FS) &&
		(SupeP->s_state & MINIX_VALID_FS) && !force) {
		if (repair || verbose)
			printf("%s is clean, no check.\n", device_name);
		if (repair && !automatic)
			tcsetattr(0, TCSANOW, &termios);
		return retcode;
	}
	else if (force)
		printf("Forcing filesystem check on %s.\n", device_name);
	else if (repair)
		printf("Filesystem on %s is dirty, needs checking.\n",\
			device_name);

	check_root();
	check();
	if (verbose) {
		int i;
        unsigned int free;

		for (i=1,free=0 ; i < INODES ; i++)
			if (!inode_in_use(i))
				free++;
		printf("\n%6u inodes used (%2d%%) %6u total\n",(INODES-free-1),
			100*(INODES-free-1)/(INODES-1), INODES-1);
		for (i=FIRSTZONE,free=0 ; i < ZONES ; i++)
			if (!zone_in_use(i))
				free++;
		printf("%6u  zones used (%2d%%) %6u total\n", ZONES - free ,
			(int)(100*((long)ZONES-free)/ZONES), ZONES);
		printf("\n%6u regular files\n"
		"%6u directories\n"
		"%6d character device files\n"
		"%6u block device files\n"
		"%6u links\n"
		"%6u symbolic links\n"
		"------\n"
		"%6u files\n",
		regular,directory,chardev,blockdev,
		links-2*directory+1,symlinks,total-2*directory+1);
	}
	if (changed) {
		write_tables();
		printf(	"----------------------------\n"
			"FILE SYSTEM HAS BEEN CHANGED\n"
			"----------------------------\n");
		for (count=0 ; count<3 ; count++)
			sync();
	}
	else if (repair)
		write_super_block();

	if (repair && !automatic)
		tcsetattr(0,TCSANOW,&termios);

	if (changed)
	      retcode += 3;
	if (errors_uncorrected)
	      retcode += 4;
	return retcode;
}
