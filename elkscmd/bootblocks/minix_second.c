//------------------------------------------------------------------------------
// MINIX Boot Block (first block of the filesystem)
// Sourced from DEV86 bootblocks
// Reworked for ELKS to load directly the kernel without any helper
// Sector 2 : boot loader
//------------------------------------------------------------------------------

//#include <a.out.h>
#include "minix.h"

// Global constants

#define LOADSEG 0x1000

// Global variables

//extern int sect_max;
//extern int head_max;
//extern int track_max;

static unsigned char dir_32;

static byte_t sb_block [BLOCK_SIZE];  // super block block buffer
static struct super_block *sb_data;   // super block structure

static int i_now = 0;
static int ib_first;                 // inode first block
static byte_t i_block [BLOCK_SIZE];  // inode block buffer
static struct inode_s * i_data;      // current inode structure

static byte_t z_block [LEVEL_MAX] [BLOCK_SIZE];  // zone block buffer

static file_pos f_pos;

static byte_t d_dir [BLOCK_SIZE];  // latest in program segment


//------------------------------------------------------------------------------

// Helpers from minix_first

void reboot ();

void puts (const char * s);

void word_hex (const int w);
int seg_data ();

int drive_reset (const int drive);
//int drive_get (const int drive);
int disk_read (const int sect, const int count,
	const byte_t * buf, const int seg);

int strcmp (const char * s, const char * d);

void run_prog ();

//------------------------------------------------------------------------------

/*
static void log_err (const char * s)
{
	puts ("ERROR: ");
	puts (s);
	puts (" !\r\n");
}
*/

//------------------------------------------------------------------------------

static int load_super ()
{
	int err;

	while (1) {
		err = disk_read (2, 2, sb_block, seg_data ());
		//if (err) break;

		sb_data = (struct super_block *) sb_block;
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

		break;
	}

	return err;
}

//------------------------------------------------------------------------------

static int load_inode ()
{
	int err;

	while (1) {

		// Compute inode block and load if not cached

		int ib = ib_first + i_now / INODES_PER_BLOCK;
		err = disk_read (ib << 1, 2, i_block, seg_data ());
		//if (err) break;

		// Get inode data

		i_data = (struct inode_s *) i_block + i_now % INODES_PER_BLOCK;
		break;
	}

	return err;
}

//------------------------------------------------------------------------------

static int load_zone (int level, zone_nr * z_start, zone_nr * z_end)
{
	int err;

	for (zone_nr * z = z_start; z < z_end; z++) {
		if (level == 0) {
			// FIXME: image can be > 64K
			err = disk_read ((*z) << 1, 2, i_now ? f_pos : d_dir + f_pos, i_now ? LOADSEG : seg_data ());
			f_pos += BLOCK_SIZE;
			if (f_pos >= i_data->i_size) break;
		} else {
			int next = level - 1;
			err = disk_read (*z << 1, 2, z_block [next], seg_data ());
			load_zone (next, (zone_nr *) z_block [next], (zone_nr *) (z_block [next] + BLOCK_SIZE));
		}
	}

	return err;
}

//------------------------------------------------------------------------------

static int load_file ()
{
	int err;

	while (1) {
		err = load_inode ();
		if (err) break;

		/*
		puts ("size=");
		word_hex (i_data->i_size);
		puts ("\r\n");
		*/

		f_pos = 0;

		while (1) {

			// Direct zones
			load_zone (0, &(i_data->i_zone [ZONE_IND_L0]), &(i_data->i_zone [ZONE_IND_L1]));
			if (f_pos >= i_data->i_size) break;

			// Indirect zones
			load_zone (1, &(i_data->i_zone [ZONE_IND_L1]), &(i_data->i_zone [ZONE_IND_L2]));
			if (f_pos >= i_data->i_size) break;

			// Double-indirect zones
			//load_zone (2, &(i_data->i_zone [ZONE_IND_L2]), &(i_data->i_zone [ZONE_IND_END]));

			break;
		}

		break;
	}

	return err;
}

//------------------------------------------------------------------------------

void main ()
{
	int err;

	while (1) {
		/*
		puts ("C=");
		word_hex (track_max);
		puts (" H=");
		word_hex (head_max);
		puts (" S=");
		word_hex (sect_max);
		puts ("\r\n");
		*/

		err = load_super ();
		/*
		if (err) {
			//log_err ("load_super");
			break;
		}
		*/

		/*
		puts ("inodes=");
		word_hex (sb_data->s_ninodes);
		puts (" zones=");
		word_hex (sb_data->s_nzones);
		puts ("\r\n");
		*/

		load_file ();

		for (int d = 0; d < i_data->i_size; d += DIRENT_SIZE) {
			if (!strcmp (d_dir + 2 + d, "linux")) {
				puts ("Linux found\r\n");
				i_now = (*(int *)(d_dir + d)) - 1;
				load_file ();
				run_prog ();
				break;
			}
		}

		break;
	}
}

//------------------------------------------------------------------------------
