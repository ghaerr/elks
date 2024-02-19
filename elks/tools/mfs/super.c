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

/**
 * @project	mfstool
 * @module	super
 * @section	3
 * @doc	Superblock functions
 */
#include "minix_fs.h"
#include "protos.h"
#include "bitops.h"
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <time.h>


/**
 * Find a free bit in map
 * @param bmap - bitmap to scan
 * @param bsize - bitmap size in blocks
 * @return the bit number of the found block
 */
unsigned long get_free_bit(u8 *bmap,int bsize) {
  int i;
  for (i = 0; i < bsize * BLOCK_SIZE; i++) {
    if (bmap[i] != 0xff) {
      int j;
      for (j = 0; j < 8 ; j++) {
        if ((bmap[i] & (1<<j)) == 0) return (i<<3) + j;
      }
      die("Internal error!");
    }
  }
  fatalmsg("No free slots in bitmap found");
  return -1;
}

/**
 * Initializes a new filesystem
 * @param fn - file name for new filesystem
 * @param magic - superblock magic number
 * @param fsize - size of filesystem in blocks
 * @param inodes - number of inodes to allocate (0 for auto)
 * @return pointer to a minix_fs_dat structure
 * @effect the file will be created or truncated.
 */ 
struct minix_fs_dat *new_fs(const char *fn,int magic,unsigned long fsize,int inodes) {
  struct minix_fs_dat *fs = domalloc(sizeof(struct minix_fs_dat),0);
  u32 rootblkp;
  char root_block[BLOCK_SIZE];
  int i;

  if (magic != MINIX_SUPER_MAGIC && magic != MINIX_SUPER_MAGIC2 && 
	magic != MINIX2_SUPER_MAGIC && magic != MINIX2_SUPER_MAGIC2) {
    fatalmsg("invalid magic fs-type %x",magic);
  }
  fs->msb.s_magic = magic;
  if (VERSION_2(fs))
    fs->msb.s_zones = fsize;
  else
    fs->msb.s_nzones = fsize;
  fs->msb.s_state = MINIX_VALID_FS;
  /* v1 volume limit is 7+512+512*512, but limit max_size to blocks in fs*/
  fs->msb.s_max_size = VERSION_2(fs) ? 0x7fffffff : fsize * 1024;
  fs->msb.s_log_zone_size = 0;		/* zone size is always BLOCK_SIZE*/

  /* Manage inodes */
  if (!inodes) inodes = fsize / 3; /* Default number inodes to 1/3 blocks */
  /* Round up inode count */
  if (VERSION_2(fs))
    inodes = ((inodes + MINIX2_INODES_PER_BLOCK - 1) & ~(MINIX2_INODES_PER_BLOCK - 1));
  else
    inodes = ((inodes + MINIX_INODES_PER_BLOCK - 1) & ~(MINIX_INODES_PER_BLOCK - 1));
  if (inodes > 65535) inodes = 65535;
  INODES(fs) = inodes;
  if (INODE_BLOCKS(fs) > fsize * 9 / 10 + 5)
     fatalmsg("Too many inodes requested");

  /*
   * Initialise bitmaps
   */
  IMAPS(fs) =UPPER(inodes + 1,BITS_PER_BLOCK);
  ZMAPS(fs)=UPPER(fsize-(1+IMAPS(fs)), BITS_PER_BLOCK);
  FIRSTZONE(fs) = NORM_FIRSTZONE(fs);
  debug("IMAPS %u, ZMAPS %u, FIRST %u\n", IMAPS(fs), ZMAPS(fs), FIRSTZONE(fs));
  if (!VERSION_2(fs) && IMAPS(fs) > MINIX_I_MAP_SLOTS)
    fatalmsg("Too many inodes requested: max is 32736\n");

  fs->inode_bmap = domalloc(IMAPS(fs) * BLOCK_SIZE,0xff);
  fs->zone_bmap = domalloc(ZMAPS(fs) * BLOCK_SIZE,0xff);

  for (i = FIRSTZONE(fs) ; i < fsize ; i++) unmark_zone(fs,i);
  for (i = MINIX_ROOT_INO; i<=INODES(fs) ; i++) unmark_inode(fs,i);

  /*
   * Initialize inode tables
   */
  fs->ino.v1 = domalloc(INODE_BUFFER_SIZE(fs),0);
  mark_inode(fs,MINIX_ROOT_INO);
  set_inode(fs,MINIX_ROOT_INO,S_IFDIR | 0755, 2,
		dogetuid(), dogetgid(), 2 * DIRSIZE(fs),NOW,NOW,NOW,0);
  rootblkp = get_free_block(fs);
  if (VERSION_2(fs))
    INODE2(fs,MINIX_ROOT_INO)->i_zone[0] = rootblkp;
  else
    INODE(fs,MINIX_ROOT_INO)->i_zone[0] = rootblkp;
  mark_zone(fs,rootblkp);
    
  /*
   * Initialise file
   */
  fs->fp = fopen(fn,"w+b");
  if (!fs->fp) die(fn);
  if (fseek(fs->fp,fsize * BLOCK_SIZE-1,SEEK_SET)) die("fseek");
  putc(0,fs->fp);
  fflush(fs->fp);

  /*
   * Create a root block
   */
  memset(root_block,0,sizeof(root_block));
  *((short *)root_block) =MINIX_ROOT_INO;
  strcpy(root_block+2,".");
  *((short *)(root_block+DIRSIZE(fs))) = MINIX_ROOT_INO;
  strcpy(root_block+2+DIRSIZE(fs),"..");
  // printf("WRITE TO : %d %08x\n",rootblkp,rootblkp * BLOCK_SIZE);
  // printf("%3d) FIRSTZONE: %d\n",__LINE__,FIRSTZONE(fs));
  dofwrite(goto_blk(fs->fp,rootblkp),root_block,sizeof(root_block));
 
  return fs;  
}

/**
 * Returns filesystem info
 * @param fs - pointer to filesystem structure
 */ 
void cmd_sysinfo(struct minix_fs_dat *fs) {
  unsigned long fsize;
  int i, inodecount = 0, blkcount = 0;

  if (VERSION_2(fs)) {
    fsize = fs->msb.s_zones;
  } else {
    fsize = fs->msb.s_nzones;
  }
  printf("image 1K blocks: boot %d, super 1, inodemap %d, filemap %d, inode %d, data %ld\n",
  	MINIX_BOOT_BLOCKS, IMAPS(fs), ZMAPS(fs), (int)INODE_BLOCKS(fs), fsize-FIRSTZONE(fs));
  for (i = MINIX_ROOT_INO; i<=INODES(fs) ; i++)
  	if (!inode_in_use(fs,i))
		inodecount++;
  printf("inodes inuse %d, free %d, overhead %d, total %d\n",
  	INODES(fs)-inodecount, inodecount-MINIX_ROOT_INO, MINIX_ROOT_INO, INODES(fs));
  for (i = 1 ; i < fsize ; i++)
  	if (!zone_in_use(fs,i))
		blkcount++;
  printf("blocks inuse %ld, free %d, overhead %d, total %ld\n", fsize-FIRSTZONE(fs)-blkcount, blkcount, FIRSTZONE(fs), fsize);
}

/**
 * Opens a filesystem
 * @param fn - file name for filesystem
 * @param chk - stop if filesystem is not clean
 * @return pointer to a minix_fs_dat structure
 */ 
struct minix_fs_dat *open_fs(const char *fn,int chk) {
  struct minix_fs_dat *fs = domalloc(sizeof(struct minix_fs_dat),0);

  fs->fp = fopen(fn,"r+b");
  if (!fs->fp) die(fn);

  /*
   * Read super block
   */
  goto_blk(fs->fp,MINIX_SUPER_BLOCK);
  dofread(goto_blk(fs->fp,MINIX_SUPER_BLOCK),
		&(fs->msb),sizeof(struct minix_super_block));
  /*
   * Sanity checks ...
   */
  if (FSMAGIC(fs) != MINIX_SUPER_MAGIC && FSMAGIC(fs) != MINIX_SUPER_MAGIC2 && 
	FSMAGIC(fs) != MINIX2_SUPER_MAGIC && FSMAGIC(fs) != MINIX2_SUPER_MAGIC2) {
    fatalmsg("invalid magic fs-type %x",FSMAGIC(fs));
  }
  if (MINIX_VALID_FS != fs->msb.s_state) {
    if (chk) fatalmsg("Filesystem %s in an unknown state, use -f", fn);
    fprintf(stderr,"Warning: %s in an unknown state\n",fn);
  }

  /*
   * Read tables...
   */
  fs->inode_bmap = domalloc(IMAPS(fs) * BLOCK_SIZE,-1);
  fs->zone_bmap = domalloc(ZMAPS(fs) * BLOCK_SIZE,-1);
  fs->ino.v1 = domalloc(INODE_BUFFER_SIZE(fs),-1);

  dofread(goto_blk(fs->fp,MINIX_SUPER_BLOCK+1),
	  fs->inode_bmap,IMAPS(fs) * BLOCK_SIZE);
  dofread(fs->fp,fs->zone_bmap,ZMAPS(fs) * BLOCK_SIZE);
  dofread(fs->fp,fs->ino.v1,INODE_BUFFER_SIZE(fs));

  return fs;
}

/**
 * Closes filesystem
 * @param fs - pointer to filesystem structure
 * @return NULL
 */ 
struct minix_fs_dat *close_fs(struct minix_fs_dat *fs) {
  goto_blk(fs->fp,MINIX_SUPER_BLOCK);
  dofwrite(goto_blk(fs->fp,MINIX_SUPER_BLOCK),
		&fs->msb,sizeof(struct minix_super_block));
  dofwrite(goto_blk(fs->fp,MINIX_SUPER_BLOCK+1),
	  fs->inode_bmap,IMAPS(fs) * BLOCK_SIZE);
  // printf("ZMAPSIZE=%d\n",ZMAPS(fs));
  dofwrite(fs->fp,fs->zone_bmap,ZMAPS(fs) * BLOCK_SIZE);
  if (VERSION_2(fs))
    dofwrite(fs->fp,fs->ino.v2,INODE_BUFFER_SIZE(fs));
  else
    dofwrite(fs->fp,fs->ino.v1,INODE_BUFFER_SIZE(fs));

  return 0;  
}

