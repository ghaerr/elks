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
  *nblks_p = 1440;
  *inodes_p = 360;
    
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
  if (*nblks_p == -1)
    fatalmsg("no filesystem size specified");

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

/* ELKS BPB parameters*/
#define ELKS_BPB_SecPerTrk	505		/* offset of sectors per track (byte)*/
#define ELKS_BPB_NumHeads	506		/* offset of number of heads (byte)*/

/**
 * Write boot block to image file
 */
void cmd_boot(char *filename, int argc,char **argv) {
  FILE *ifp = NULL, *ofp;
  struct stat sb;
  int count;
  int opt_new_bootblock = 0, opt_updatebpb = 0;
  int SecPerTrk, NumHeads;
  u8 blk[MINIX_BOOT_BLOCKS * BLOCK_SIZE];

  if (argc != 2 && argc != 3) fatalmsg("Usage: %s [boot file]\n",argv[0]);
  if (argv[1][0] == '-' && argv[1][1] == 'B') {
	opt_updatebpb = 1;			/* BPB update specified*/
	if (sscanf(&argv[1][2], "%d,%d", &SecPerTrk, &NumHeads) != 2)
		fatalmsg("Invalid -B<sectors>,<heads> option\n");
	printf("Updating BPB to %d sectors, %d heads\n", SecPerTrk, NumHeads);
	argv++;
	argc--;
  }
  if (argc == 2)				/* new boot block specified*/
	opt_new_bootblock = 1;

  if (opt_new_bootblock) {
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

	if (count < 512 || blk[510] != 0x55 || blk[511] != 0xaa)
		fprintf(stderr, "%s warning: may not be valid boot block\n", argv[1]);
  
	if (opt_updatebpb) {	/* update BPB before writing*/
		blk[ELKS_BPB_SecPerTrk] = (unsigned char)SecPerTrk;
		blk[ELKS_BPB_NumHeads] = (unsigned char)NumHeads;
	}
	if (fwrite(blk,1,count,ofp) != count) die("fwrite(%s)", argv[1]);
	fclose(ofp);
	fclose(ifp);
  } else {			/* perform BPB update only on existing boot block*/
	ofp = fopen(filename, "r+b");
	if (!ofp) die(filename);

	count = fread(blk,1,512,ofp);
	if (count != 512) die("fread(%s)", filename);
	blk[ELKS_BPB_SecPerTrk] = (unsigned char)SecPerTrk;
	blk[ELKS_BPB_NumHeads] = (unsigned char)NumHeads;
	if (fseek(ofp, 0L, SEEK_SET) != 0)
		die("fseek(%s)", filename);
	if (fwrite(blk,1,512,ofp) != 512) die("fwrite(%s)", filename);
	fclose(ofp);
  }
}
