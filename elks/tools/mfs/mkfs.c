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
 * @module	genfs
 * @section	3
 * @doc	routines for creating new file systems
 */
#include "minix_fs.h"
#include "protos.h"
#include <sys/stat.h>
#include <stdlib.h>

int opt_nocopyzero = 0;		/* don't copy zero-length files starting with . */

/**
 * Parse mkfs/genfs command line arguments
 * @param argc - argc from command line
 * @param argv - argv from command line
 * @param magic_p - returns filesystem magic number
 * @param nblks_p - returns size of file system
 * @param inodes_p - return number of requested inodes
 */
/* 
 * -n namelen
 * -1 -> version1
 * -2|v -> version2
 * -i nodecount
 * -s nblocks
 */
void parse_mkfs(int argc,char **argv,int *magic_p,int *nblks_p,int *inodes_p) {
  int c;
  int namelen = 14;
  int version = 1;
  *nblks_p = 0;
  *inodes_p = 0;
    
  optind = 1;
  while (1) {
    c = getoptX(argc,argv,"12vi:n:s:k");
    if (c == -1) break;
    switch (c) {
    case '1':
      version = 1;
      break;
    case '2':
    case 'v':
      version = 2;
      break;
    case 'n':
      namelen = atoi(optarg);
      if (namelen != 30 && namelen != 14)
        fatalmsg("invalid name len (%d) must be 30 or 14",namelen);
	  break;
    case 'i':
      *inodes_p = atoi(optarg);
      break;
    case 's':
      *nblks_p = atoi(optarg);
      break;
	case 'k':
		opt_nocopyzero = 1;
		break;
    default:
      usage(argv[0]);
    }
  }
  if (*nblks_p == 0)
    fatalmsg("no filesystem size specified");
  if (!*inodes_p)
    *inodes_p = *nblks_p / 3;

  //printf("MKFS v%d namelen %d inodes %d size %d\n", version, namelen, *inodes_p, *nblks_p);
  if (version == 2)
    *magic_p = namelen == 30 ? MINIX2_SUPER_MAGIC2 : MINIX2_SUPER_MAGIC;
  else
    *magic_p = namelen == 30 ? MINIX_SUPER_MAGIC2 : MINIX_SUPER_MAGIC;
}

/**
 * Create an empty filesystem
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_mkfs(char *filename, int argc,char **argv) {
  int req_inos;
  int req_blks;
  int magic;
  struct minix_fs_dat *fs;

  parse_mkfs(argc,argv,&magic,&req_blks,&req_inos);
  fs = new_fs(filename,magic,req_blks,req_inos);
  if (opt_verbose) cmd_sysinfo(fs);
  close_fs(fs);
}

/**
 * Write boot block to image file
 */
void cmd_boot(char *filename, int argc,char **argv) {
  FILE *ifp = NULL, *ofp;
  struct stat sb;
  int count;
  u8 blk[MINIX_BOOT_BLOCKS * BLOCK_SIZE];

  if (argc != 2) fatalmsg("Usage: %s <boot block image>\n",argv[0]);

	if (stat(argv[1],&sb)) die("stat(%s)",argv[1]);
	if (!S_ISREG(sb.st_mode)) fatalmsg("%s: not a regular file\n",argv[1]);
	if (sb.st_size > MINIX_BOOT_BLOCKS * BLOCK_SIZE)
		fatalmsg("%s: boot block greater than %d bytes\n", argv[1], MINIX_BOOT_BLOCKS*BLOCK_SIZE);

	ifp = fopen(argv[1],"rb");
	if (!ifp) die(argv[1]);

	ofp = fopen(filename,"r+b");
	if (!ofp) die(filename);

	count = fread(blk,1,MINIX_BOOT_BLOCKS * BLOCK_SIZE,ifp);
	if (count != sb.st_size) die("fread(%s)", argv[1]);

	if (fwrite(blk,1,count,ofp) != count) die("fwrite(%s)", argv[1]);
	fclose(ofp);
	fclose(ifp);
}
