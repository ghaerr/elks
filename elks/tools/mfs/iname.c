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
 * @module	iname
 * @section	3
 * @doc	directory name manipulation functions
 */
#include "minix_fs.h"
#include "protos.h"
#include "bitops.h"
#include <string.h>

/**
 * Compare filenames in a directory structure
 * @param lname - name to compare
 * @param dname - directory name to compare
 * @param nlen - length of directory structure
 */
int cmp_name(const char *lname,const char *dname,int nlen) {
  int i;
  for (i=0;i< nlen;i++) {
    if (lname[i] == '/' || lname[i] == 0) {
      if (dname[i] == 0) return 0;
      return -1;
    }
    if (lname[i] != dname[i]) return 1;
  }
  return 0;
}

/**
 * Find name in a directory
 * @param fs - filesystem structure
 * @param inode - inode for directory to search
 * @param lname - filename (nul or "/" terminated)
 * @param blkp - file block where name was found
 * @param offp - offset pointer where name was found
 * @return inode for found name, -1 on error.
 * @effect *blkp and *offp are updated if name is found.
 */
int ilookup_name(struct minix_fs_dat *fs,int inode,const char *lname,
		u32 *blkp,int *offp) {
  int i,bsz,j;
  u8 blk[BLOCK_SIZE];
  int fdirsize = VERSION_2(fs) ?
		(INODE2(fs,inode))->i_size : (INODE(fs,inode))->i_size;
  int dentsz = DIRSIZE(fs);

  for (i = 0; i < fdirsize; i += BLOCK_SIZE) {
    bsz = read_inoblk(fs,inode,i / BLOCK_SIZE,blk);
    for (j = 0; j < bsz ; j+= dentsz) {
      u16 fino = *((u16 *)(blk+j));
      if (!fino) continue;
      if (!cmp_name(lname,(char *)blk+j+2,dentsz-2)) {
        if (blkp) *blkp = i / BLOCK_SIZE;
        if (offp) *offp = j;
        return fino;
      }
    }
  }
  return -1;
}

/**
 * Add file name to a directory.
 * @param fs - filesystem pointer
 * @param dir - path to directory
 * @param name - name to add
 * @param inode - inode number
 */
void dname_add(struct minix_fs_dat *fs,int dinode,const char *name,int inode) {
  u8 blk[BLOCK_SIZE];
  int dentsz = DIRSIZE(fs);
  int dfsize = VERSION_2(fs) ?
		 INODE2(fs,dinode)->i_size : INODE(fs,dinode)->i_size;
  int i,j=0,nblk=0,bsz;

  for (i=0;i< dfsize ; i+= BLOCK_SIZE) {
    bsz = read_inoblk(fs,dinode,nblk = i/BLOCK_SIZE,blk);
    for (j = 0; j < bsz ; j += dentsz) {
      u16 fino = *((u16 *)(blk+j));
      if (!fino) goto SKIP_ALL;
    }
  }
SKIP_ALL:
  if (i >= dfsize) {
    /* Need to extend directory file */
    if (VERSION_2(fs)) {
      INODE2(fs,dinode)->i_size += dentsz;
    } else {
      INODE(fs,dinode)->i_size += dentsz;
    }
    if (j == BLOCK_SIZE) {
      nblk++;
      j = 0;
      memset(blk,0,BLOCK_SIZE);
    }
  } else {
    memset(blk+j,0,dentsz);
  }
  /* Create directory entry */
  *((u16 *)(blk+j)) = inode;
  strncpy((char *)blk+j+2,name,dentsz-2);

  /* Update directory */
  write_inoblk(fs,dinode,nblk,blk);
}

/**
 * Remove file name from a directory.
 * @param fs - filesystem pointer
 * @param dir - path to directory
 * @param name - name to add
 * @return 0 in success, -1 in error.
 */
void dname_rem(struct minix_fs_dat *fs,int dinode,const char *name) {
  u8 blk[BLOCK_SIZE];
  int dentsz = DIRSIZE(fs);
  u32 nblk;
  int off;
  int i;

  i =  ilookup_name(fs,dinode,name,&nblk,&off);
  if (i == -1) return;
  i = (VERSION_2(fs) ? INODE2(fs,dinode)->i_size : INODE(fs,dinode)->i_size)
	- dentsz;

  if (i == (nblk * BLOCK_SIZE + off)) {
    /* Need to shorten directory file */
    if (VERSION_2(fs)) {
      INODE2(fs,dinode)->i_size = i;
    } else {
      INODE(fs,dinode)->i_size = i;
    }
    if (!(i % BLOCK_SIZE)) {
      /* We should free-up the last block... */
      free_inoblk(fs,dinode,(i/BLOCK_SIZE)+1);
    }
  } else {
    read_inoblk(fs,dinode,nblk,blk);
    memset(blk+off,0,dentsz);
    write_inoblk(fs,dinode,nblk,blk);
  }
}

/**
 * Create a new inode/directory
 * @param fs - file system structure
 * @param fpath - filename path
 * @param mode - mode
 * @param uid - user id
 * @param gid - group id
 * @param size - file size
 * @param atime - access time
 * @param mtime - modification time
 * @param ctime - change time
 * @param dinode_p - pointer to the directory that contains this node
 */
int make_node(struct minix_fs_dat *fs,char *fpath, int mode,
		int uid, int gid, u32 size,
		u32 atime, u32 mtime, u32 ctime, int *dinode_p) {
  char *dir = fpath;
  char *fname = strrchr(fpath,'/');
  int dinode,ninode;

  if (find_inode(fs,fpath) != -1) fatalmsg("%s: already exists",fpath);
  if (fname) {
    *fname++ = 0;
  } else {
    dir = ".";
    fname = fpath;
  }
  dinode = find_inode(fs,dir);
  if (dinode == -1) fatalmsg("%s: not found\n",dir);
  ninode = get_free_inode(fs);
  mark_inode(fs,ninode);
  /* Initialize inode */
  set_inode(fs, ninode, mode, 1, uid, gid,size,atime,mtime,ctime,1);
  dname_add(fs,dinode,fname,ninode);
  if (dinode_p) *dinode_p = dinode;
  return ninode;
}
