/*
 * Copyright (C) 2005 - Alejandro Liu Ly <alejandro_liu@hotmail.com>
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
 * @module	inode
 * @section	3
 * @doc	inode manipulation functions
 */
#include "minix_fs.h"
#include "protos.h"
#include "bitops.h"
#include <string.h>
#include <time.h>
#include <sys/stat.h>
/**
 * Calculate the zone number from the file block in inode (Minix v2)
 * @param fs - filesystem structure
 * @param ino - i-node structure (v2)
 * @param blk - file block
 */
u32 ino2_zone(struct minix_fs_dat *fs,struct minix2_inode *ino,u32 blk) {
  /* Direct block */
  if (blk < 7) return ino->i_zone[blk];

  /* Indirect block */
  blk -= 7;
  if (blk < MINIX2_ZONESZ) {
    u32 iblks[MINIX2_ZONESZ];
    if (ino->i_indir_zone == 0) return 0;
    dofread(goto_blk(fs->fp,ino->i_indir_zone),iblks,sizeof iblks);
    return iblks[blk];
  }
  /* Double indirect block. */
  blk -= MINIX2_ZONESZ;
  if (blk < MINIX2_ZONESZ * MINIX2_ZONESZ) {
    u32 iblks[MINIX2_ZONESZ];
    if (ino->i_dbl_indr_zone == 0) return 0;
    dofread(goto_blk(fs->fp,ino->i_dbl_indr_zone),iblks,sizeof iblks);
    if (iblks[blk / MINIX2_ZONESZ] == 0) return 0;
    dofread(goto_blk(fs->fp,iblks[blk / MINIX2_ZONESZ]),iblks,sizeof iblks);
    return iblks[blk % MINIX2_ZONESZ];
  }
  die("file bigger than maximum size");
  return -1;
}

/**
 * Calculate the zone number from the file block in inode (Minix v1)
 * @param fs - filesystem structure
 * @param ino - i-node structure (v1)
 * @param blk - file block
 */
int ino_zone(struct minix_fs_dat *fs,struct minix_inode *ino,int blk) {
  /* Direct block */
  if (blk < 7) return ino->i_zone[blk];

  /* Indirect block */
  blk -= 7;
  if (blk < MINIX_ZONESZ) {
    u16 iblks[MINIX_ZONESZ];
    if (ino->i_indir_zone == 0) return 0;
    dofread(goto_blk(fs->fp,ino->i_indir_zone),iblks,sizeof iblks);
    return iblks[blk];
  }
  /* Double indirect block. */
  blk -= MINIX_ZONESZ;
  if (blk < MINIX_ZONESZ * MINIX_ZONESZ) {
    int iblks[MINIX_ZONESZ];
    if (ino->i_dbl_indr_zone == 0) return 0;
    dofread(goto_blk(fs->fp,ino->i_dbl_indr_zone),iblks,sizeof iblks);
    if (iblks[blk / MINIX_ZONESZ] == 0) return 0;
    dofread(goto_blk(fs->fp,iblks[blk / MINIX_ZONESZ]),iblks,sizeof iblks);
    return iblks[blk % MINIX_ZONESZ];
  }
  die("file bigger than maximum size");
  return -1;
}


/**
 * Sets the block for a file's inode to point to specific zone (v2)
 * @param fs - filesystem structure
 * @param ino - inode to update (v2)
 * @param blk - File block to update
 * @param zone - zone to point to
 */
void ino2_setzone(struct minix_fs_dat *fs,struct minix2_inode *ino,
			u32 blk,u32 zone) {
  /* Direct block */
  if (blk < 7) {
    if (ino->i_zone[blk] && ino->i_zone[blk] != zone)
      unmark_zone(fs,ino->i_zone[blk]);
    ino->i_zone[blk] = zone;
    return;
  }
  /* Indirect block */
  blk -= 7;
  if (blk < MINIX2_ZONESZ) {
    u32 iblks[MINIX2_ZONESZ];
    if (ino->i_indir_zone == 0) {
      ino->i_indir_zone = get_free_block(fs);
      mark_zone(fs,ino->i_indir_zone);
      memset(iblks,0,sizeof iblks);
    } else {
      dofread(goto_blk(fs->fp,ino->i_indir_zone),iblks,sizeof iblks);
      if (iblks[blk] && iblks[blk] != zone) unmark_zone(fs,iblks[blk]);
    }
    iblks[blk] = zone;
    dofwrite(goto_blk(fs->fp,ino->i_indir_zone),iblks,sizeof iblks);
    return ;
  }
  /* Double indirect block. */
  blk -= MINIX2_ZONESZ;
  if (blk < MINIX2_ZONESZ * MINIX2_ZONESZ) {
    u32 iblks[MINIX2_ZONESZ];
    u32 iblk,izone;
    if (ino->i_dbl_indr_zone == 0) {
      ino->i_dbl_indr_zone = get_free_block(fs);
      mark_zone(fs,ino->i_dbl_indr_zone);
      memset(iblks,0,sizeof iblks);
    } else {
      dofread(goto_blk(fs->fp,ino->i_dbl_indr_zone),iblks,sizeof iblks);
    }
    iblk = blk / MINIX2_ZONESZ;
    blk %= MINIX2_ZONESZ;
    if (iblks[iblk] == 0) {
      izone = iblks[iblk] = get_free_block(fs);
      mark_zone(fs,iblks[iblk]);
      dofwrite(goto_blk(fs->fp,ino->i_dbl_indr_zone),iblks,sizeof iblks);
      memset(iblks,0,sizeof iblks);
    } else {
      dofread(goto_blk(fs->fp,izone = iblks[iblk]),iblks,sizeof iblks);
      if (iblks[blk] && iblks[blk] != zone) unmark_zone(fs,iblks[blk]);
    }
    iblks[blk] = zone;
    dofwrite(goto_blk(fs->fp,izone),iblks,sizeof iblks);
    return;
  }
  die("file bigger than maximum size");
}

/**
 * Sets the block for a file's inode to point to specific zone (v1)
 * @param fs - filesystem structure
 * @param ino - inode to update (v1)
 * @param blk - File block to update
 * @param zone - zone to point to
 */
void ino_setzone(struct minix_fs_dat *fs,struct minix_inode *ino,
			int blk,int zone) {
  /* Direct block */
  if (blk < 7) {
    if (ino->i_zone[blk] && ino->i_zone[blk] != zone)
      unmark_zone(fs,ino->i_zone[blk]);
    ino->i_zone[blk] = zone;
    return;
  }
  /* Indirect block */
  blk -= 7;
  if (blk < MINIX_ZONESZ) {
    u16 iblks[MINIX_ZONESZ];
    if (ino->i_indir_zone == 0) {
      ino->i_indir_zone = get_free_block(fs);
      mark_zone(fs,ino->i_indir_zone);
      memset(iblks,0,sizeof iblks);
    } else {
      dofread(goto_blk(fs->fp,ino->i_indir_zone),iblks,sizeof iblks);
      if (iblks[blk] && iblks[blk] != zone) unmark_zone(fs,iblks[blk]);
    }
    iblks[blk] = zone;
    dofwrite(goto_blk(fs->fp,ino->i_indir_zone),iblks,sizeof iblks);
    return ;
  }
  /* Double indirect block. */
  blk -= MINIX_ZONESZ;
  if (blk < MINIX_ZONESZ * MINIX_ZONESZ) {
    u16 iblks[MINIX_ZONESZ];
    u16 iblk,izone;
    if (ino->i_dbl_indr_zone == 0) {
      ino->i_dbl_indr_zone = get_free_block(fs);
      mark_zone(fs,ino->i_dbl_indr_zone);
      memset(iblks,0,sizeof iblks);
    } else {
      dofread(goto_blk(fs->fp,ino->i_dbl_indr_zone),iblks,sizeof iblks);
    }
    iblk = blk / MINIX_ZONESZ;
    blk %= MINIX_ZONESZ;
    if (iblks[iblk] == 0) {
      izone = iblks[iblk] = get_free_block(fs);
      mark_zone(fs,iblks[iblk]);
      dofwrite(goto_blk(fs->fp,ino->i_dbl_indr_zone),iblks,sizeof iblks);
      memset(iblks,0,sizeof iblks);
    } else {
      dofread(goto_blk(fs->fp,izone = iblks[iblk]),iblks,sizeof iblks);
      if (iblks[blk] && iblks[blk] != zone) unmark_zone(fs,iblks[blk]);
    }
    iblks[blk] = zone;
    dofwrite(goto_blk(fs->fp,izone),iblks,sizeof iblks);
    return;
  }
  die("file bigger than maximum size");
}

/**
 * Frees the block for a file's inode (v2)
 * @param fs - filesystem structure
 * @param ino - inode to update (v2)
 * @param blk - File block to update
 */
void ino2_freezone(struct minix_fs_dat *fs,struct minix2_inode *ino,u32 blk) {
  int i;
  /* Direct block */
  if (blk < 7) {
    if (ino->i_zone[blk]) unmark_zone(fs,ino->i_zone[blk]);
    ino->i_zone[blk] = 0;
    return;
  }
  /* Indirect block */
  blk -= 7;
  if (blk < MINIX2_ZONESZ) {
    u32 iblks[MINIX2_ZONESZ];
    if (ino->i_indir_zone == 0) return;
    dofread(goto_blk(fs->fp,ino->i_indir_zone),iblks,sizeof iblks);
    if (iblks[blk]) unmark_zone(fs,iblks[blk]);
    iblks[blk] = 0;
    for (i=0;i<MINIX2_ZONESZ;i++) {
      if (iblks[i]) {
	dofwrite(goto_blk(fs->fp,ino->i_indir_zone),iblks,sizeof iblks);
	return;
      }
    }
    unmark_zone(fs,ino->i_indir_zone);
    ino->i_indir_zone = 0;
    return ;
  }
  /* Double indirect block. */
  blk -= MINIX2_ZONESZ;
  if (blk < MINIX2_ZONESZ * MINIX2_ZONESZ) {
    u32 iblks[MINIX2_ZONESZ];
    u32 dblks[MINIX2_ZONESZ];
    u32 iblk;
    if (ino->i_dbl_indr_zone == 0) return;
    dofread(goto_blk(fs->fp,ino->i_dbl_indr_zone),iblks,sizeof iblks);

    iblk = blk / MINIX2_ZONESZ;
    blk %= MINIX2_ZONESZ;

    if (iblks[iblk] == 0) return;

    dofread(goto_blk(fs->fp,iblks[iblk]),dblks,sizeof dblks);
    if (dblks[blk]) unmark_zone(fs,dblks[blk]);
    dblks[blk] = 0;

    for (i=0; i < MINIX2_ZONESZ; i++) {
      if (dblks[blk]) {
        dofwrite(goto_blk(fs->fp,iblks[iblk]),dblks,sizeof dblks);
        return;
      }
    }
    unmark_zone(fs,iblks[iblk]);
    iblks[iblk] = 0;
    for (i=0; i < MINIX2_ZONESZ; i++) {
      if (iblks[blk]) {
        dofwrite(goto_blk(fs->fp,ino->i_dbl_indr_zone),iblks,sizeof iblks);
        return;
      }
    }
    unmark_zone(fs,ino->i_dbl_indr_zone);
    ino->i_dbl_indr_zone = 0;
    return;
  }
  die("file bigger than maximum size");
}

/**
 * Frees the block for a file's inode (v1)
 * @param fs - filesystem structure
 * @param ino - inode to update (v1)
 * @param blk - File block to update
 */
void ino_freezone(struct minix_fs_dat *fs,struct minix_inode *ino,int blk) {
  int i;
  /* Direct block */
  if (blk < 7) {
    if (ino->i_zone[blk]) unmark_zone(fs,ino->i_zone[blk]);
    ino->i_zone[blk] = 0;
    return;
  }
  /* Indirect block */
  blk -= 7;
  if (blk < MINIX_ZONESZ) {
    u16 iblks[MINIX_ZONESZ];
    if (ino->i_indir_zone == 0) return;
    dofread(goto_blk(fs->fp,ino->i_indir_zone),iblks,sizeof iblks);
    if (iblks[blk]) unmark_zone(fs,iblks[blk]);
    iblks[blk] = 0;
    for (i=0;i<MINIX_ZONESZ;i++) {
      if (iblks[i]) {
	dofwrite(goto_blk(fs->fp,ino->i_indir_zone),iblks,sizeof iblks);
	return;
      }
    }
    unmark_zone(fs,ino->i_indir_zone);
    ino->i_indir_zone = 0;
    return ;
  }
  /* Double indirect block. */
  blk -= MINIX_ZONESZ;
  if (blk < MINIX_ZONESZ * MINIX_ZONESZ) {
    u16 iblks[MINIX_ZONESZ];
    u16 dblks[MINIX_ZONESZ];
    u16 iblk;
    if (ino->i_dbl_indr_zone == 0) return;
    dofread(goto_blk(fs->fp,ino->i_dbl_indr_zone),iblks,sizeof iblks);

    iblk = blk / MINIX_ZONESZ;
    blk %= MINIX_ZONESZ;

    if (iblks[iblk] == 0) return;

    dofread(goto_blk(fs->fp,iblks[iblk]),dblks,sizeof dblks);
    if (dblks[blk]) unmark_zone(fs,dblks[blk]);
    dblks[blk] = 0;

    for (i=0; i < MINIX_ZONESZ; i++) {
      if (dblks[blk]) {
        dofwrite(goto_blk(fs->fp,iblks[iblk]),dblks,sizeof dblks);
        return;
      }
    }
    unmark_zone(fs,iblks[iblk]);
    iblks[iblk] = 0;
    for (i=0; i < MINIX_ZONESZ; i++) {
      if (iblks[blk]) {
        dofwrite(goto_blk(fs->fp,ino->i_dbl_indr_zone),iblks,sizeof iblks);
        return;
      }
    }
    unmark_zone(fs,ino->i_dbl_indr_zone);
    ino->i_dbl_indr_zone = 0;
    return;
  }
  die("file bigger than maximum size");
}

/**
 * Read an inode block.  Checks if we are actually trying to read past
 * the end-of-file.
 * @param fs - filesystem structure
 * @param inode - inode to read from
 * @param blk - file block to read
 * @param buf - buffer pointer (must be BLOCKSIZE)
 * @return	number of bytes copied in to buf (typically BLOCKSIZE) unless
 *		last block or if we are reading a hole, which will return 0.
 */
int read_inoblk(struct minix_fs_dat *fs,int inode,u32 blk,u8 *buf) {
  if (VERSION_2(fs)) {
    u32 zone;
    int bsize = BLOCK_SIZE;
    struct minix2_inode *ino = INODE2(fs,inode);
    if (blk * BLOCK_SIZE > ino->i_size) return 0;

    zone = ino2_zone(fs,ino,blk);
    if (zone == 0) return 0; /* This is a hole! */
    if (ino->i_size / BLOCK_SIZE == blk) bsize = ino->i_size % BLOCK_SIZE;
    dofread(goto_blk(fs->fp,zone),buf,bsize);
    if (bsize < BLOCK_SIZE) memset(buf+bsize,0,BLOCK_SIZE-bsize);
    return bsize;
  } else {
    int zone;
    int bsize = BLOCK_SIZE;
    struct minix_inode *ino = INODE(fs,inode);
    if (blk * BLOCK_SIZE > ino->i_size) return 0;

    zone = ino_zone(fs,ino,blk);
    if (zone == 0) return 0; /* This is a hole! */
    if (ino->i_size / BLOCK_SIZE == blk) bsize = ino->i_size % BLOCK_SIZE;
    dofread(goto_blk(fs->fp,zone),buf,bsize);
    if (bsize < BLOCK_SIZE) memset(buf+bsize,0,BLOCK_SIZE-bsize);
    return bsize;
  }
  return -1;
}

/**
 * Write an inode block.  It will allocate blocks from zone_bmap as needed.
 * It will *not* extend the filesize counter in the inode.
 * @param fs - filesystem structure
 * @param inode - inode to write to
 * @param blk - file block to write
 * @param buf - buffer pointer (must be BLOCKSIZE)
 */
void write_inoblk(struct minix_fs_dat *fs,int inode,u32 blk,u8 *buf) {
  if (VERSION_2(fs)) {
    u32 zone;
    struct minix2_inode *ino = INODE2(fs,inode);

    zone = ino2_zone(fs,ino,blk);
    if (zone == 0) {
      /* Allocate block... */
      zone = get_free_block(fs);
      mark_zone(fs,zone);
      ino2_setzone(fs,ino,blk,zone);
    }
    dofwrite(goto_blk(fs->fp,zone),buf,BLOCK_SIZE);
  } else {
    int zone;
    struct minix_inode *ino = INODE(fs,inode);

    zone = ino_zone(fs,ino,blk);
    if (zone == 0) {
      /* Allocate block... */
      zone = get_free_block(fs);
      mark_zone(fs,zone);
      ino_setzone(fs,ino,blk,zone);
    }
    dofwrite(goto_blk(fs->fp,zone),buf,BLOCK_SIZE);
  }
}
/**
 * Free an inode block.
 * @param fs - filesystem structure
 * @param inode - inode to free
 * @param blk - file block free
 */
void free_inoblk(struct minix_fs_dat *fs,int inode,u32 blk) {
  if (VERSION_2(fs))
    ino2_freezone(fs,INODE2(fs,inode),blk);
  else
    ino_freezone(fs,INODE(fs,inode),blk);
}

/**
 * Trim file to size
 * @param fs - File system structure
 * @param inode - inode to trim
 * @param sz - new file size
 */
void trunc_inode(struct minix_fs_dat *fs,int inode,u32 sz) {
  if (VERSION_2(fs)) {
    struct minix2_inode *ino = INODE2(fs,inode);
    if (S_ISCHR(ino->i_mode) || S_ISBLK(ino->i_mode)) return;
    if (sz < ino->i_size) {
      int blk = UPPER(sz,BLOCK_SIZE);
      int eblk = UPPER(ino->i_size,BLOCK_SIZE);
	while (blk < eblk) ino2_freezone(fs,ino,blk++);
    }
    ino->i_size = sz;
  } else {
    struct minix_inode *ino = INODE(fs,inode);
    if (S_ISCHR(ino->i_mode) || S_ISBLK(ino->i_mode)) return;
    if (sz < ino->i_size) {
      int blk = UPPER(sz,BLOCK_SIZE);
      int eblk = UPPER(ino->i_size,BLOCK_SIZE);
      while (blk < eblk) ino_freezone(fs,ino,blk++);
    }
    ino->i_size = sz;
  }
}


/**
 * Find an inode given the specified path
 * @param fs - filesystem structure
 * @param path - path to find
 * @return inode number or -1 on error
 */
int find_inode(struct minix_fs_dat *fs,const char *path) {
  int inode = MINIX_ROOT_INO;
  if (!strcmp(path,"/") || path[0] == 0) {
    path = ".";
  } else if (path[0] == '/')
    ++path;

  while (path) {
    inode = ilookup_name(fs,inode,path,NULL,NULL);
    if (inode == -1) return -1;
    path = strchr(path,'/');
    if (path) path++;
  }
  return inode;
}

/**
 * Change inode configuration
 * @param fs - filesystem structure
 * @param inode - inode to update
 * @param mode - mode
 * @param nlinks - number of links
 * @param uid - user id
 * @param gid - group id
 * @param size - file size or rdev type
 * @param atime - access time
 * @param mtime - modification time
 * @param ctime - change time
 * @param clr - if true, we will clear the inode first...
 */
void set_inode(struct minix_fs_dat *fs,int inode, int mode, int nlinks,
		int uid, int gid, u32 size,
		u32 atime, u32 mtime, u32 ctime, int clr) {
  time_t now = time(NULL);
  if (VERSION_2(fs)) {
    struct minix2_inode *ino = INODE2(fs,inode);
    if (clr) memset(ino,0,sizeof(struct minix2_inode));
    ino->i_mode = mode;
    ino->i_nlinks = nlinks;
    ino->i_uid = uid;
    ino->i_gid = gid;

    if (S_ISCHR(ino->i_mode) || S_ISBLK(ino->i_mode)) {
      ino->i_size = 0;
      ino->i_zone[0] = size;
    } else {
      ino->i_size = size;
    }
    ino->i_atime = atime == NOW ? now : atime;
    ino->i_mtime = mtime == NOW ? now : mtime;
    ino->i_ctime = ctime == NOW ? now : ctime;
  } else {
    struct minix_inode *ino = INODE(fs,inode);
    if (clr) memset(ino,0,sizeof(struct minix_inode));
    ino->i_mode = mode;
    ino->i_nlinks = nlinks;
    ino->i_uid = uid;
    ino->i_gid = gid;
    if (S_ISCHR(ino->i_mode) || S_ISBLK(ino->i_mode)) {
      ino->i_size = 0;
      ino->i_zone[0] = size;
    } else {
      ino->i_size = size;
    }
    ino->i_time = mtime == NOW ? now : mtime;
  }
}

/**
 * Clear inode entry
 * @param fs - filesystem structure
 * @param inode - inode to update
 */
void clr_inode(struct minix_fs_dat *fs,int inode) {
  if (VERSION_2(fs)) {
    struct minix2_inode *ino = INODE2(fs,inode);
    memset(ino,0,sizeof(struct minix2_inode));
  } else {
    struct minix_inode *ino = INODE(fs,inode);
    memset(ino,0,sizeof(struct minix_inode));
  }
  unmark_inode(fs,inode);
}
