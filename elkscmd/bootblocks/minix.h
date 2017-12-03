/****************************************************************************/
/* Super block table.  The root file system and every mounted file system
 * has an entry here.  The entry holds information about the sizes of the bit
 * maps and inodes.  The s_ninodes field gives the number of inodes available
 * for files and directories, including the root directory.  Inode 0 is 
 * on the disk, but not used.  Thus s_ninodes = 4 means that 5 bits will be
 * used in the bit map, bit 0, which is always 1 and not used, and bits 1-4
 * for files and directories.  The disk layout is:
 *
 *      Item        # blocks
 *    boot block      1
 *    super block     1
 *    inode map     s_imap_blocks
 *    zone map      s_zmap_blocks
 *    inodes        (s_ninodes + 1 + INODES_PER_BLOCK - 1)/INODES_PER_BLOCK
 *    unused        whatever is needed to fill out the current zone
 *    data zones    (s_nzones - s_firstdatazone) << s_log_zone_size
 *
 * A super_block slot is free if s_dev == NO_DEV.
 */

#define BLOCK_SIZE      1024	/* # bytes in a disk block */

/* Flag bits for i_mode in the inode. */
#define I_TYPE          0170000	/* this field gives inode type */
#define I_REGULAR       0100000	/* regular file, not dir or special */
#define I_BLOCK_SPECIAL 0060000	/* block special file */
#define I_DIRECTORY     0040000	/* file is a directory */
#define I_CHAR_SPECIAL  0020000	/* character special file */
#define I_SET_UID_BIT   0004000	/* set effective uid on exec */
#define I_SET_GID_BIT   0002000	/* set effective gid on exec */
#define ALL_MODES       0006777	/* all bits for user, group and others */
#define RWX_MODES       0000777	/* mode bits for RWX only */
#define R_BIT           0000004	/* Rwx protection bit */
#define W_BIT           0000002	/* rWx protection bit */
#define X_BIT           0000001	/* rwX protection bit */
#define I_NOT_ALLOC     0000000	/* this inode is free */

/* Type definitions */
typedef unsigned short unshort;	/* must be 16-bit unsigned */
typedef unshort block_nr;	/* block number */
typedef unshort inode_nr;	/* inode number */
typedef unshort zone_nr;	/* zone number */
typedef unshort bit_nr;		/* if inode_nr & zone_nr both unshort,
				   then also unshort, else long */

typedef unshort sect_nr;

typedef long zone_type;		/* zone size */
typedef unshort mask_bits;	/* mode bits */
typedef unshort dev_nr;		/* major | minor device number */
typedef char links;		/* number of links to an inode */
typedef long real_time;		/* real time in seconds since Jan 1, 1970 */
typedef long file_pos;		/* position in, or length of, a file */
typedef short uid;		/* user id */
typedef char gid;		/* group id */

/* Tables sizes */
#define NR_ZONE_NUMS       9	/* # zone numbers in an inode */
#define NAME_SIZE         14	/* # bytes in a directory component */

/* Miscellaneous constants */
#define SUPER_MAGIC   0x137F	/* magic number contained in super-block */
#define SUPER_MAGIC2  0x138F	/* Secondary magic 30 char names */

#define BOOT_BLOCK  (block_nr)0	/* block number of boot block */
#define SUPER_BLOCK (block_nr)1	/* block number of super block */
#define ROOT_INODE  (inode_nr)1	/* inode number for root directory */

/* Derived sizes */
#define NR_DZONE_NUM     (NR_ZONE_NUMS-2)		/* # zones in inode */
#define INODES_PER_BLOCK (BLOCK_SIZE/sizeof(d_inode))	/* # inodes/disk blk */
#define NR_INDIRECTS     (BLOCK_SIZE/sizeof(zone_nr))	/* # zones/indir blk */
#define INTS_PER_BLOCK   (BLOCK_SIZE/sizeof(int))	/* # integers/blk */

struct super_block {
  inode_nr s_ninodes;		/* # usable inodes on the minor device */
  zone_nr s_nzones;		/* total device size, including bit maps etc */
  unshort s_imap_blocks;	/* # of blocks used by inode bit map */
  unshort s_zmap_blocks;	/* # of blocks used by zone bit map */
  zone_nr s_firstdatazone;	/* number of first data zone */
  short s_log_zone_size;	/* log2 of blocks/zone */
  file_pos s_max_size;		/* maximum file size on this device */
  short s_magic;		/* magic number to recognize super-blocks */

} ;

/* Type definitions local to the File System. */
typedef struct {		/* directory entry */
  inode_nr d_inum;		/* inode number */
  char d_name[NAME_SIZE];	/* character string */
} dir_struct;

/* Declaration of the disk inode used in rw_inode(). */
typedef struct {		/* disk inode.  Memory inode is in "inotab.h" */
  mask_bits i_mode;		/* file type, protection, etc. */
  uid i_uid;			/* user id of the file's owner */
  file_pos i_size;		/* current file size in bytes */
  real_time i_modtime;		/* when was file data last changed */
  gid i_gid;			/* group number */
  links i_nlinks;		/* how many links to this file */
  zone_nr i_zone[NR_ZONE_NUMS];	/* block nums for direct, ind, and dbl ind */
} d_inode;

