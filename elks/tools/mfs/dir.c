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
 * @module	dir
 * @section	3
 * @doc	routines for reading directories
 */
#include "minix_fs.h"
#include "protos.h"
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/* ls options*/
#define LFLAG		0x01
#define DFLAG		0x02

/**
 * Print a directory entry
 * @param fp - file to write to (typically stdio)
 * @param dp - pointer to directory entry's name
 * @param namlen - length of file name
 */
void outent(const unsigned char *dp,int namlen) {
  while (*dp && namlen--) {
    putchar(*dp++);
  }
}

void printsymlink(struct minix_fs_dat *fs, int inode)
{
  int bsz;
  u8 blk[BLOCK_SIZE+1];

  bsz = read_inoblk(fs,inode,0,blk);
  if (!bsz)
  	return;
  blk[bsz] = 0;
  printf(" -> %s", blk);
}

/**
 * List contents of a directory
 * @param fs - filesystem structure
 * @param path - directory path to list
 */
void dols(struct minix_fs_dat *fs,const char *path,int flags) {
  int inode = find_inode(fs,path);
  int i,bsz,j;
  int fdirsize;
  int dentsz = DIRSIZE(fs);
  int symlink = 0;
  u8 blk[BLOCK_SIZE];

  if (inode == -1) {
    fprintf(stderr,"%s: not found\n",path);
	return;
  }
  if (flags & DFLAG) {
dflag:
	if (flags & LFLAG)
	  dostat_inode(fs, inode);
  	printf("%s", path);
	if (symlink)
		printsymlink(fs, inode);
	putchar('\n');
	return;
  }
  if (VERSION_2(fs)) {
    struct minix2_inode *ino = INODE2(fs,inode);
	symlink = S_ISLNK(ino->i_mode);
    if (!S_ISDIR(ino->i_mode)) goto dflag;
    fdirsize = ino->i_size;    
  } else {
    struct minix_inode *ino = INODE(fs,inode);
	symlink = S_ISLNK(ino->i_mode);
    if (!S_ISDIR(ino->i_mode)) goto dflag;
    fdirsize = ino->i_size;    
  }
  /* read directory*/
  for (i = 0; i < fdirsize; i += BLOCK_SIZE) {
    bsz = read_inoblk(fs,inode,i / BLOCK_SIZE,blk);
    for (j = 0; j < bsz ; j+= dentsz) {
      u16 fino = *((u16 *)(blk+j));
      if (fino == 0) continue;
	  symlink = 0;
	  if (flags & LFLAG)
	    symlink = dostat_inode(fs, fino);
      outent(blk+j+2,dentsz-2);
	  if (symlink && (flags & LFLAG))
		printsymlink(fs, fino);
	  putchar('\n');
    }
  }
}


/**
 * Commant to list contents of a directory
 * @param fs - filesystem structure
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_ls(struct minix_fs_dat *fs,int argc,char **argv) {
  int flags = 0;

  optind = 1;
  while (1) {
    int c = getoptX(argc,argv,"ald");
    if (c == -1) break;
    switch (c) {
    case 'a':
      break;
    case 'l':
      flags |= LFLAG;
      break;
    case 'd':
      flags |= DFLAG;
      break;
    default:
      fatalmsg("Usage: ls [-ld]\n");
    }
  }
  argc -= optind;
  argv += optind;

  if (argc == 0) {
    dols(fs,".", flags);
  } else if (argc == 1) {
    dols(fs,argv[0], flags);
  } else {
    int i;
    for (i=0;i<argc;i++) {
      printf("%s:\n",argv[i]);
      dols(fs,argv[i], flags);
    }
  }
}

/**
 * Print time formatted
 * @param fp - output file pointer
 * @param str - type string
 * @param usecs - time stamp
 */
void timefmt(const char *str,u32 usecs) {
  struct tm tmb;
  time_t secs = usecs;
  char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  					 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  localtime_r(&secs,&tmb);
  printf("%s %02d %4d %02d:%02d:%02d ",
		months[tmb.tm_mon], tmb.tm_mday, tmb.tm_year+1900,
		tmb.tm_hour, tmb.tm_min, tmb.tm_sec);
}

static void outmode2(int mode) {
	printf((mode & 4)? "r": "-");
	printf((mode & 2)? "w": "-");
	printf((mode & 1)? "x": "-");
}

/**
 * Print formatted mode value
 * @param fp - output file
 * @param mode - mode bits
 */
void outmode(int mode) {
  int type;
  switch (mode & S_IFMT) {
    case S_IFSOCK: type = 's'; break;
    case S_IFLNK: type = 'l'; break;
    case S_IFREG: type = '-'; break;
    case S_IFBLK: type = 'b'; break;
    case S_IFDIR: type = 'd'; break;
    case S_IFCHR: type = 'c'; break;
    case S_IFIFO: type = 'f'; break;
    default: type = '?'; break;
  }
  printf("%c", type);
  outmode2(mode >> 6);
  outmode2(mode >> 3);
  outmode2(mode);
}

int dostat_inode(struct minix_fs_dat *fs, int inode) {
  int symlink;

  if (VERSION_2(fs)) {
    struct minix2_inode *ino = INODE2(fs,inode);
	symlink = S_ISLNK(ino->i_mode);
    outmode(ino->i_mode);
    printf(" %2d %5d ",ino->i_nlinks, inode);
    printf("%3d,%3d ",ino->i_uid,ino->i_gid);
    if (S_ISCHR(ino->i_mode) || S_ISBLK(ino->i_mode))
      printf("%2d, %3d ", (ino->i_zone[0]>>8) & 0xff, ino->i_zone[0] & 0xff);
    else
      printf("%7d ",ino->i_size);
	timefmt("accessed",ino->i_atime);
	//timefmt("modified",ino->i_mtime);
    //timefmt("changed",ino->i_ctime);
  } else {
    struct minix_inode *ino = INODE(fs,inode);
	symlink = S_ISLNK(ino->i_mode);
    outmode(ino->i_mode);
    printf(" %2d %3d ",ino->i_nlinks, inode);
    printf("%3d,%3d ",ino->i_uid,ino->i_gid);
    if (S_ISCHR(ino->i_mode) || S_ISBLK(ino->i_mode))
      printf("%2d, %3d ", (ino->i_zone[0]>>8) & 0xff, ino->i_zone[0] & 0xff);
    else
      printf("%7d ",ino->i_size);
    timefmt("accessed",ino->i_time);
  }
  return symlink;
}

/**
 * Create a directory
 * @param fs - filesystem structure
 * @param newdir - directory name
 */
int domkdir(struct minix_fs_dat *fs,char *newdir, int mode) {
  int dinode;
  int ninode = make_node(fs,newdir,mode|S_IFDIR,dogetuid(),dogetgid(), 0,
  			NOW,NOW,NOW,&dinode);
  dname_add(fs,ninode,".",ninode);
  dname_add(fs,ninode,"..",dinode);
  if (VERSION_2(fs)) {
    INODE2(fs,dinode)->i_nlinks++;
    INODE2(fs,ninode)->i_nlinks++;
  } else {
    INODE(fs,dinode)->i_nlinks++;
    INODE(fs,ninode)->i_nlinks++;
  }
  return ninode;
}

/**
 * Command to create directories
 * @param fs - filesystem structure
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_mkdir(struct minix_fs_dat *fs,int argc,char **argv, int mode) {
  int i;
  for (i=1;i<argc;i++) {
    domkdir(fs,argv[i], mode);
  }
}


/**
 * Remove an <b>empty</b> directory
 * @param fs - filesystem structure
 * @param dir - directory to remove
 */
void dormdir(struct minix_fs_dat *fs,const char *dir) {
  int inode = find_inode(fs,dir);
  int i,bsz,j;
  u8 blk[BLOCK_SIZE];
  int fdirsize;
  int dentsz = DIRSIZE(fs);
  int pinode = -1;
  const char *p;

  if (inode == -1) fatalmsg("%s: not found",dir);
  if (inode == MINIX_ROOT_INO) fatalmsg("Can not remove root inode");

  /* Make sure directory is a directory */
  if (VERSION_2(fs)) {
    struct minix2_inode *ino = INODE2(fs,inode);
    if (!S_ISDIR(ino->i_mode)) fatalmsg("%s: is not a directory",dir);
    fdirsize = ino->i_size;    
  } else {
    struct minix_inode *ino = INODE(fs,inode);
    if (!S_ISDIR(ino->i_mode)) fatalmsg("%s: is not a directory",dir);
    fdirsize = ino->i_size;    
  }

  /* Do a directory scan... */
  for (i = 0; i < fdirsize; i += BLOCK_SIZE) {
    bsz = read_inoblk(fs,inode,i / BLOCK_SIZE,blk);
    for (j = 0; j < bsz ; j+= dentsz) {
      u16 fino = *((u16 *)(blk+j));
      if (blk[j+2] == '.' && blk[j+3] == 0) continue;
      if (blk[j+2] == '.' && blk[j+3] == '.'&& blk[j+4] == 0) {
        pinode = fino;
        continue;
      }
      if (fino != 0) fatalmsg("%s: not empty",dir);
    }
  }

  /* Free stuff */
  trunc_inode(fs,inode,0);
  clr_inode(fs, inode);
  if (VERSION_2(fs)) {
    INODE(fs,pinode)->i_nlinks--;
  } else {
    INODE(fs,pinode)->i_nlinks--;
  }
  p = strrchr(dir,'/');
  if (p)
    p++;
  else
    p = dir;
  dname_rem(fs,pinode,p);
}

/**
 * Remove directories
 * @param fs - filesystem structure
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_rmdir(struct minix_fs_dat *fs,int argc,char **argv) {
  int i;
  for (i=1;i<argc;i++) {
    dormdir(fs,argv[i]);
  }
}

/**
 * Remove an file (not directory)
 * @param fs - filesystem structure
 * @param fpath - file path
 */
void dounlink(struct minix_fs_dat *fs,char *fpath) {
  char *dir = fpath;
  char *fname = strrchr(fpath,'/');
  int dinode,inode;

  if (fname) {
    *fname++ = 0;
  } else {
    dir = ".";
    fname = fpath;
  }
  dinode = find_inode(fs,dir);
  if (dinode == -1) fatalmsg("%s: not found\n",dir);
  inode = ilookup_name(fs,dinode,fname,NULL,NULL);
  if (inode == -1) fatalmsg("%s: not found\n",fname);

  /* Make sure file is not a directory */
  if (VERSION_2(fs)) {
    struct minix2_inode *ino = INODE2(fs,inode);
    if (S_ISDIR(ino->i_mode)) fatalmsg("%s: is a directory",dir);
    dname_rem(fs,dinode,fname);
    if (--ino->i_nlinks) return;
  } else {
    struct minix_inode *ino = INODE(fs,inode);
    if (S_ISDIR(ino->i_mode)) fatalmsg("%s: is a directory",dir);
    dname_rem(fs,dinode,fname);
    if (--ino->i_nlinks) return;
  }
  /* Remove stuff... */
  trunc_inode(fs,inode,0);
  clr_inode(fs,inode);
}

/**
 * Remove files (but not directories)
 * @param fs - filesystem structure
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_rm(struct minix_fs_dat *fs,int argc,char **argv) {
  int i;
  for (i=1;i<argc;i++) {
    dounlink(fs,argv[i]);
  }
}
