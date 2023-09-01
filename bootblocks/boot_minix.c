//------------------------------------------------------------------------------
// MINIX Boot Block (first block of the filesystem)
// Sourced from DEV86 bootblocks
// Reworked for ELKS to load directly the kernel without any helper
// Sector 2 : boot loader
//------------------------------------------------------------------------------

#include "linuxmt/config.h"
#include "minix.h"

// Global constants

#define LOADSEG DEF_INITSEG
#define OPTSEG	DEF_OPTSEG             // bootopts copied here

// Global variables

// Uninitialized data is not set to zero by default
// as the context is not a user process
// but code loaded in uninitialized memory

static byte_t sb_block [BLOCK_SIZE];  // super block block buffer
#define sb_data	((struct super_block *)sb_block)

static int i_now;
static int i_boot;
static int loadaddr;
static int ib_first;                 // inode first block

static byte_t i_block [BLOCK_SIZE];  // inode block buffer
static struct inode_s * i_data;      // current inode structure

static byte_t z_block [LEVEL_MAX] [BLOCK_SIZE];  // zone block buffer

static file_pos f_pos;

static byte_t d_dir [BLOCK_SIZE];    // root directory buffer


//------------------------------------------------------------------------------

// Helpers from boot sector

void except (char code);

void puts (const char * s);

int seg_data ();

void disk_read (const int sect, const int count, const byte_t * buf, const int seg);

void run_prog ();

static int strcmp (const char * s, const char * d);
static void load_super ();
static void load_inode ();
static void load_zone (int level, zone_nr * z_start, zone_nr * z_end);
static void load_file ();


//------------------------------------------------------------------------------

#define seg_data()	__builtin_ia16_near_data_segment ()
#define _MK_FP(seg,off)	((void __far *)((((unsigned long)(seg)) << 16) | (off)))

//------------------------------------------------------------------------------

// This must occur right at the start of the payload.  Currently this is done
// by specifying -fno-toplevel-reorder in the Makefile, which forces GCC to
// output all functions, variables, and __asm's in the same order as in the
// source code.  FIXME: find a better way.
// REALLY running out of space here [HS] 805/2023

void load_prog ()
{
	// Avoid reuse of an old copy of /bootopts in memory
	// if we're rebooting with no /bootopts
	int __far *optseg = _MK_FP(OPTSEG, 0);
	*optseg = i_boot = i_now = 0;

	load_super();
	load_file ();

	for (int d = 0; d < BLOCK_SIZE /*(int)i_data->i_size*/; d += DIRENT_SIZE) {
		if (!strcmp ((char *)(d_dir + 2 + d), "linux")) {
			i_boot = i_now = (*(int *)(d_dir + d)) - 1;
			if (i_boot == -1) continue;
			puts ("/linux");
			loadaddr = LOADSEG << 4;
			load_file();
			continue;
		}
		if (!strcmp ((char *)(d_dir + 2 + d), "bootopts")) {
			i_now = (*(int *)(d_dir + d)) - 1;
			if (i_now != -1) {
				loadaddr = OPTSEG << 4;
				//puts("opts ");
				load_file();
			}
			continue;
		}
	}
	if (i_boot > 0)	// 'linux' may be deleted (i_boot = -1) or non-existent (=0)
		run_prog();
}

//------------------------------------------------------------------------------

static int strcmp (const char * s, const char * d)
{
	const char * p1 = s;
	const char * p2 = d;

	int c1, c2;

	while ((c1 = *p1++) == (c2 = *p2++) && c1 /* && c2*/);
	return c1 - c2;
}

//------------------------------------------------------------------------------

static void load_super ()
{
	disk_read (2, 2, sb_block, seg_data ());

	/*
	if (sb_data->s_log_zone_size) {
		//log_err ("zone size");
		err = -1;
		break;
	}
	*/

	ib_first = 2 + sb_data->s_imap_blocks + sb_data->s_zmap_blocks;

	/*
	if (sb_data->s_magic == SUPER_MAGIC) {
		dir_32 = 0;
	} else if (sb_data->s_magic == SUPER_MAGIC2) {
		dir_32 = 1;
	} else {
		//log_err ("super magic");
		err = -1;
	}
	*/
}

//------------------------------------------------------------------------------

static void load_inode ()
{
	// Compute inode block and load if not cached

	int ib = ib_first + i_now / INODES_PER_BLOCK;
	disk_read (ib << 1, 2, i_block, seg_data ());

	// Get inode data

	i_data = (struct inode_s *) i_block + i_now % INODES_PER_BLOCK;
}

//------------------------------------------------------------------------------

static void load_zone (int level, zone_nr * z_start, zone_nr * z_end)
{
	for (zone_nr * z = z_start; z < z_end; z++) {
		if (level == 0) {
			if (i_now) {
				long lin_addr = loadaddr + f_pos;
				disk_read ((*z) << 1, 2, (byte_t *) (unsigned) lin_addr, (unsigned) (lin_addr >> 4) & 0xf000);
			} else {
				if (!f_pos) disk_read ((*z) << 1, 2, d_dir /*+ f_pos*/, seg_data ());
			}
			f_pos += BLOCK_SIZE;
			if (f_pos >= i_data->i_size) break;
		} else {
			int next = level - 1;
			disk_read ((*z) << 1, 2, z_block [next], seg_data ());
			load_zone (next, (zone_nr *) z_block [next], (zone_nr *) (z_block [next] + BLOCK_SIZE));
		}
	}
}

//------------------------------------------------------------------------------

static void load_file ()
{
	load_inode ();

	/*
	puts ("size=");
	word_hex (i_data->i_size);
	puts ("\r\n");
	*/
	f_pos = 0;

	// Direct zones
	load_zone (0, &(i_data->i_zone [ZONE_IND_L0]), &(i_data->i_zone [ZONE_IND_L1]));
	if (f_pos >= i_data->i_size) return;

	// Indirect zones
	load_zone (1, &(i_data->i_zone [ZONE_IND_L1]), &(i_data->i_zone [ZONE_IND_L2]));
	if (f_pos >= i_data->i_size) return;

	// Double-indirect zones
	//load_zone (2, &(i_data->i_zone [ZONE_IND_L2]), &(i_data->i_zone [ZONE_IND_END]));
}

//------------------------------------------------------------------------------
