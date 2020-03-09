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
 * @module	readers
 * @section	3
 * @doc	routines for reading files
 */
#include "minix_fs.h"
#include "protos.h"
#include <sys/stat.h>
#include <utime.h>
#include <unistd.h>

/**
 * Read a file from file system
 * @param fs - File system structure
 * @param fp - Output file
 * @param path - File to read
 * @param type - Read type: S_IFREG or S_IFLNK
 * @param ispipe - If true we do not use fseek to skip holes
 */
int readfile(struct minix_fs_dat *fs,FILE *fp,const char *path,
		int type,int ispipe) {
  int inode = find_inode(fs,path);
  int i,bsz,j;
  u8 blk[BLOCK_SIZE];
  int fdirsize;

  if (inode == -1) fatalmsg("%s: not found",path);
  if (VERSION_2(fs)) {
    struct minix2_inode *ino = INODE2(fs,inode);
    if (type == S_IFREG && !S_ISREG(ino->i_mode))
      fatalmsg("%s: is not a regular file",path);
    else if (type == S_IFLNK && !S_ISLNK(ino->i_mode))
      fatalmsg("%s: is not a symbolic link",path);
    fdirsize = ino->i_size;    
  } else {
    struct minix_inode *ino = INODE(fs,inode);
    if (type == S_IFREG && !S_ISREG(ino->i_mode))
      fatalmsg("%s: is not a regular file",path);
    else if (type == S_IFLNK && !S_ISLNK(ino->i_mode))
      fatalmsg("%s: is not a symbolic link",path);
    fdirsize = ino->i_size;    
  }
  for (i = 0; i < fdirsize; i += BLOCK_SIZE) {
    bsz = read_inoblk(fs,inode,i / BLOCK_SIZE,blk);
    if (bsz) {
      dofwrite(fp,blk,bsz);
    } else {
      bsz = fdirsize - i > BLOCK_SIZE ? BLOCK_SIZE : fdirsize % BLOCK_SIZE;
      if (ispipe) {
	for (j=0;j<bsz;j++) putc(0,fp);
      } else
        fseek(fp,bsz,SEEK_CUR);
    }
  }
  return inode;
}


/**
 * Similar to UNIX cat command
 * @param fs - file system structure
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_cat(struct minix_fs_dat *fs,int argc,char **argv) {
  int i;
  for (i=1;i<argc;i++) {
    readfile(fs,stdout,argv[i],S_IFREG,1);
  }
}

/**
 * Extract image files to a normal file
 * @param fs - file system structure
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_extract(struct minix_fs_dat *fs,int argc,char **argv) {
  FILE *fp;
  struct utimbuf tb;
  int uid,gid,mode,inode;
  if (argc != 3) fatalmsg("Usage: %s [image file] [output file]\n",argv[0]);
  fp = fopen(argv[2],"wb");
  if (!fp) die(argv[2]);
  inode = readfile(fs,fp,argv[1],S_IFREG,0);
  /* We want to copy also the modes ... */
  if (VERSION_2(fs)) {
    struct minix2_inode *ino = INODE2(fs,inode);
    tb.actime = ino->i_atime;
    tb.modtime = ino->i_mtime;
    uid = ino->i_uid;
    gid = ino->i_gid;
    mode = ino->i_mode & 07777;
  } else {
    struct minix_inode *ino = INODE(fs,inode);
    tb.modtime = tb.actime = ino->i_time;
    uid = ino->i_uid;
    gid = ino->i_gid;
    mode = ino->i_mode & 07777;
  }
  fclose(fp);
  if (opt_keepuid) chown(argv[2],uid,gid);
  chmod(argv[2],mode);
  utime(argv[2],&tb);
} 

/**
 * Read contents of a symlink
 * @param fs - file system structure
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_readlink(struct minix_fs_dat *fs,int argc,char **argv) {
  if (argc == 1) {
    fatalmsg("Usage: %s [links ...]\n",argv[0]);
  } else if (argc == 2) {
    readfile(fs,stdout,argv[1],S_IFLNK,1);
    putc('\n',stdout);
  } else {
    int i;
    for (i=1;i<argc;i++) {
      printf("%s: ",argv[i]);
      readfile(fs,stdout,argv[i],S_IFLNK,1);
      putc('\n',stdout);
    }
  }
}
