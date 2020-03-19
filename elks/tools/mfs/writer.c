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
 * @module	writer
 * @section	3
 * @doc	routines for writing files
 */
#include "minix_fs.h"
#include "protos.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

/**
 * Write to a file/inode.  It makes holes along the way...
 * @param fs - File system structure
 * @param fp - Input file
 * @param inode - Inode to write to
 */
void writefile(struct minix_fs_dat *fs,FILE *fp,int inode) {
  int j,bsz,blkcnt = 0;
  u32 count = 0;
  u8 blk[BLOCK_SIZE];

  do {
    bsz = fread(blk,1,BLOCK_SIZE,fp);
    for (j=0;j<bsz;j++) if (blk[j]) break;
    if (j != bsz) { /* This is not a hole, so better write it */
      if (bsz < BLOCK_SIZE) memset(blk+bsz,0,BLOCK_SIZE-bsz);
      write_inoblk(fs,inode,blkcnt++,blk);
    } else {
      free_inoblk(fs,inode,blkcnt++);
    }
    count += bsz;
  } while (bsz == BLOCK_SIZE);
  trunc_inode(fs,inode,count);
}

/**
 * Write data to file/inode.
 * @param fs - File system structure
 * @param blk - Data to write
 * @param cnt - bytes to write
 * @param inode - Inode to write to
 */
void writedata(struct minix_fs_dat *fs,u8 *blk,u32 cnt,int inode) {
  int i,blkcnt;
  
  for (blkcnt=i=0; i < cnt; i+= BLOCK_SIZE) {
    if (i+BLOCK_SIZE < cnt) {
      write_inoblk(fs,inode,blkcnt++,blk+i);
    } else {
      u8 blk2[BLOCK_SIZE];
      memcpy(blk2,blk+i,cnt-i);
      memset(blk2+cnt-i,0,BLOCK_SIZE-cnt+i);
      write_inoblk(fs,inode,blkcnt,blk2);
    }
  }
  trunc_inode(fs,inode,cnt);
}

/**
 * Create a symlink
 * @param fs - file system structure
 * @param source - source file
 * @param lnkpath - link name
 */
void dosymlink(struct minix_fs_dat *fs,char *source,char *lnkpath) {
  int len = strlen(source);
  int inode = make_node(fs,lnkpath,0777|S_IFLNK,0,0,len,NOW,NOW,NOW,NULL);
  writedata(fs,(unsigned char *)source,len,inode);
}

/**
 * Create a hard link
 * @param fs - file system structure
 * @param source - source file
 * @param lnkpath - link name
 */
void dohardlink(struct minix_fs_dat *fs,char *source,char *lnkpath) {
  char *dir = lnkpath;
  char *lname = strrchr(lnkpath,'/');
  int dinode;
  int tinode = find_inode(fs,source);

  if (tinode == -1) fatalmsg("%s: file doesn't exist", source);

  if (VERSION_2(fs)) {
    struct minix2_inode *ino =INODE2(fs,tinode);
    if (!S_ISREG(ino->i_mode)) fatalmsg("%s: can only link regular files", source);
    ino->i_nlinks++;
  } else {
    struct minix_inode *ino =INODE(fs,tinode);
    if (!S_ISREG(ino->i_mode)) fatalmsg("%s: can only link regular files", source);
    ino->i_nlinks++;
  }  

  if (find_inode(fs,lnkpath) != -1) fatalmsg("%s: already exists",lnkpath); 
  if (lname) {
    *lname++ = 0;
  } else {
    dir = ".";
    lname = lnkpath;
  }

  dinode = find_inode(fs,dir);
  if (dinode == -1) fatalmsg("%s: not found\n",dir);
  dname_add(fs,dinode,lname,tinode);
}

/**
 * Create a link command
 * @param fs - file system structure
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_ln(struct minix_fs_dat *fs,int argc,char **argv) {
  int symlink = 0;
  if (argc > 1 && !strcmp(argv[1], "-s")) {
    symlink = 1;
	--argc; ++argv;
  }
  if (argc != 3) fatalmsg("Usage: %s [-s] [source_file] [link_name]\n",argv[0]);
  if (symlink)
    dosymlink(fs,argv[1],argv[2]);
  else dohardlink(fs,argv[1],argv[2]);
}

static int ftype(const char *strtype) {
  switch(strtype[0]) {
  case 'c': return S_IFCHR;
  case 'b': return S_IFBLK;
  //case 'd': return S_IFDIR;
  //case 's': return S_IFSOCK;
  //case 'f': return S_IFREG;
  //case 'p': return S_IFIFO;
  }
  return -1;
}

/**
 * Create a node
 * @param fs - file system structure
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_mknode(struct minix_fs_dat *fs,int argc,char **argv) {
  int type = -1, mode;
  int uid = dogetuid(),gid = dogetgid();
  int major = 0, minor = 0, count = 0,inc = 1;

  if (argc < 4 || (type = ftype(argv[2])) < 0)
    fatalmsg("Usage: %s path [c|b] major minor [count inc mode]\n", argv[0]);
  
  mode = 0644;
  major = atoi(argv[3]);
  minor = atoi(argv[4]);
  if (argc > 5) count = atoi(argv[5]);
  if (argc > 6) inc = atoi(argv[6]);
  if (argc > 7) sscanf(argv[7], "%o", &mode);
  if (inc < 1) fatalmsg("Invalid increment value: %d\n",inc);
  if (mode == -1) fatalmsg("Invalid mode %s\n", argv[7]);

  if (count > 1 && (type == S_IFBLK || type == S_IFCHR)) {
    int i = 0;
    char b[256];
    do {
      snprintf(b,sizeof b,"%s%d",argv[1],i++);
      make_node(fs,b,mode|type,uid,gid,DEVNUM(major,minor),NOW,NOW,NOW,NULL);
      minor += inc;
    } while (--count);
  } else {
      make_node(fs, argv[1], mode|type, uid, gid, DEVNUM(major,minor), NOW, NOW, NOW, NULL);
  }
}

/**
 * Copy files to an image file
 * @param fs - file system structure
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_cp(struct minix_fs_dat *fs,int argc,char **argv) {
  FILE *fp;
  struct stat sb;
  int inode;
  char filename[256];

  if (argc != 3) fatalmsg("Usage: %s [source_file] [dest_dir_or_file]\n",argv[0]);
  if (stat(argv[1],&sb)) die("stat(%s)",argv[1]);
  if (!S_ISREG(sb.st_mode)) fatalmsg("%s: not a regular file\n",argv[1]);

  fp = fopen(argv[1],"rb");
  if (!fp) die(argv[1]);

  strcpy(filename, argv[2]);
  inode = find_inode(fs, filename);
  if (inode != -1) {				/* destination exists, check if dir*/
    char *dir = argv[1];
    char *fname = strrchr(dir,'/');
    if (fname) {
      *fname++ = 0;
    } else {
      dir = ".";
      fname = argv[1];
    }
    if (VERSION_2(fs)) {
      struct minix2_inode *ino = INODE2(fs,inode);
      if (S_ISDIR(ino->i_mode)) {
	  	sprintf(filename, "%s/%s", argv[2], fname);
        inode = find_inode(fs, filename);
	  }
	} else {
      struct minix_inode *ino = INODE(fs,inode);
      if (S_ISDIR(ino->i_mode)) {
	  	sprintf(filename, "%s/%s", argv[2], fname);
        inode = find_inode(fs, filename);
	  }
    }
  }
  if (inode != -1) {				/* destination exists*/
    if (VERSION_2(fs)) {
      struct minix2_inode *ino = INODE2(fs,inode);
      if (!S_ISREG(ino->i_mode)) fatalmsg("%s: not a regular file\n",filename);
      ino->i_mode = sb.st_mode;
      ino->i_atime = sb.st_atime;
      ino->i_mtime = sb.st_mtime;
      ino->i_ctime = sb.st_ctime;
	} else {
      struct minix_inode *ino = INODE(fs,inode);
      if (!S_ISREG(ino->i_mode)) fatalmsg("%s: not a regular file\n",filename);
      ino->i_mode = sb.st_mode;
      ino->i_time = sb.st_mtime;
	}
  	trunc_inode(fs, inode, 0);		/* doesn't reset times*/
  } else {
    inode = make_node(fs,filename,sb.st_mode,
			opt_keepuid? sb.st_uid: 0,opt_keepuid? sb.st_gid: 0,
			sb.st_size,sb.st_atime,sb.st_mtime,sb.st_ctime,NULL);
  
  }
  writefile(fs,fp,inode);
  fclose(fp);
}
