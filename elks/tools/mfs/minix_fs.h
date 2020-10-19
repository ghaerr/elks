/*
 * Copyright (C) 2005 - Alejandro Liu Ly <alejandro_liu@hotmail.com>
 * Copyright (C) 2019 Greg Haerr <greg@censoft.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Minix file system constants and structures
 */
#include <stdio.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

/*
 * This is the original minix inode layout on disk.
 * Note the 8-bit gid and atime and ctime.
 */
struct minix_inode {
        u16 i_mode;	/* File type and permissions for file */
        u16 i_uid;	/* 16 uid */
        u32 i_size;	/* File size in bytes */
        u32 i_time;
        u8  i_gid;
        u8  i_nlinks;
        u16 i_zone[7];
        u16 i_indir_zone;
        u16 i_dbl_indr_zone;
};

/*
 * The new minix inode has all the time entries, as well as
 * long block numbers and a third indirect block (7+1+1+1
 * instead of 7+1+1). Also, some previously 8-bit values are
 * now 16-bit. The inode is now 64 bytes instead of 32.
 */
struct minix2_inode {
        u16 i_mode;	/* File type and permissions for file */
        u16 i_nlinks;
        u16 i_uid;	/* uid */
        u16 i_gid;	/* gid */
        u32 i_size;	/* File size in bytes */
        u32 i_atime;
        u32 i_mtime;
        u32 i_ctime;
        u32 i_zone[7];
        u32 i_indir_zone;
        u32 i_dbl_indr_zone;
        u32 _unused;
};

/*
 * minix super-block data on disk
 */
struct minix_super_block {
        u16 s_ninodes;	/* Number of inodes */
        u16 s_nzones;	/* device size in blocks (v1) */
        u16 s_imap_blocks;	/* inode map size in blocks */ 
        u16 s_zmap_blocks;	/* zone map size in blocks */
        u16 s_firstdatazone;	/* Where data blocks begin */
        u16 s_log_zone_size;	/* log2 of zone size*/
        u32 s_max_size;	/* Max file size supported in bytes*/
        u16 s_magic;	/* magic number... fs version */
        u16 s_state;	/* filesystem state */
        u32 s_zones;	/* device size in blocks (v2) */
        u32 s_unused[4];
};

struct minix_dir_entry {
	u16 inode;
	char name[0];
};
		
#define MINIX_BOOT_BLOCKS	1	/* number of boot blocks*/

#define BLOCK_SIZE_BITS 10		/* 1024 byte disk blocks*/
#define BLOCK_SIZE (1<<BLOCK_SIZE_BITS)
#define BITS_PER_BLOCK	(BLOCK_SIZE << 3)

#define NAME_MAX         255   /* # chars in a file name */

#define MINIX_INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct minix_inode)))
#define MINIX2_INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct minix2_inode)))

#define MINIX_VALID_FS               0x0001          /* Clean fs. */
#define MINIX_ERROR_FS               0x0002          /* fs has errors. */

#define MINIX_SUPER_MAGIC    0x137F          /* original minix fs */
#define MINIX_SUPER_MAGIC2   0x138F          /* minix fs, 30 char names */
#define MINIX2_SUPER_MAGIC   0x2468	     /* minix V2 fs */
#define MINIX2_SUPER_MAGIC2  0x2478	     /* minix V2 fs, 30 char names */

#define MINIX_SUPER_BLOCK	1
#define MINIX_ROOT_INO 1

/* Not the same as the bogus LINK_MAX in <linux/limits.h>. Oh well. */
#define MINIX_LINK_MAX	250
#define MINIX2_LINK_MAX	65530

#define MINIX_I_MAP_SLOTS	4
#define MINIX_Z_MAP_SLOTS	8

#define MINIX2_ZONESZ	(BLOCK_SIZE/sizeof(u32))
#define MINIX_ZONESZ	(BLOCK_SIZE/sizeof(u16))


struct minix_fs_dat {
	FILE *fp;
	struct minix_super_block msb;
	u8 *inode_bmap;
	u8 *zone_bmap;
	union {
		struct minix_inode *v1;
		struct minix2_inode *v2;
	} ino;
};

#define FSMAGIC(fs)	((fs)->msb.s_magic)
#define VERSION_2(fs) (FSMAGIC(fs) == MINIX2_SUPER_MAGIC || FSMAGIC(fs) == MINIX2_SUPER_MAGIC2 )
#define INODES(fs) ((fs)->msb.s_ninodes)
#define UPPER(size,n) ((size+((n)-1))/(n))
#define INODE_BLOCKS(fs) UPPER(INODES(fs), VERSION_2(fs) ? MINIX2_INODES_PER_BLOCK : MINIX_INODES_PER_BLOCK)
#define INODE_BUFFER_SIZE(fs) (INODE_BLOCKS(fs) * BLOCK_SIZE)			
#define ZONES(fs) ((unsigned long)(VERSION_2(fs) ? (fs)->msb.s_zones : (fs)->msb.s_nzones))
#define IMAPS(fs) ((fs)->msb.s_imap_blocks)
#define ZMAPS(fs) ((fs)->msb.s_zmap_blocks)
#define FIRSTZONE(fs) ((fs)->msb.s_firstdatazone)
#define NORM_FIRSTZONE(fs) (MINIX_BOOT_BLOCKS+1+IMAPS(fs)+ZMAPS(fs)+INODE_BLOCKS(fs))
#define INODE_BUF(fs) (VERSION_2(fs) ? (fs)->ino.v2 : (fs)->ino.v1)
#define DIRSIZE(fs) (FSMAGIC(fs) == MINIX_SUPER_MAGIC2 || FSMAGIC(fs) == MINIX2_SUPER_MAGIC2 ? 32 : 16)
#define BLOCKS(fs) (VERSION_2(fs)?fs->msb.s_zones : fs->msb.s_nzones)

#define INODE(fs,inode) ((fs)->ino.v1+((inode)-1))
#define INODE2(fs,inode) ((fs)->ino.v2+((inode)-1))

#define inode_in_use(fs,x) (bit((fs)->inode_bmap,(x)))
#define zone_in_use(fs,x) (bit((fs)->zone_bmap,(x)))
#define mark_inode(fs,x) (setbit((fs)->inode_bmap,(x)))
#define unmark_inode(fs,x) (clrbit((fs)->inode_bmap,(x)))
#define mark_zone(fs,x) (setbit((fs)->zone_bmap,(x)-FIRSTZONE(fs)+1))
#define unmark_zone(fs,x) (clrbit((fs)->zone_bmap,(x)-FIRSTZONE(fs)+1))

#define get_free_inode(fs) get_free_bit((fs)->inode_bmap,IMAPS(fs))
#define get_free_block(fs) (get_free_bit((fs)->zone_bmap,ZMAPS(fs))+FIRSTZONE(fs)-1)

#define DEVNUM(maj,min) (((maj)<<8)|(min))
#define NOW ((u32)-1)

