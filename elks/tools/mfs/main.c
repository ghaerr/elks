/*
 * Minix-Filesytem image manipulation tool.
 *
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
 *
 * Enhancements by ghaerr:
 *  reorder program arguments to "mfs [options] filesys [cmd [cmdoptions]]"
 *	add -f, -p options. All files copied uid/gid 0/0 unless -p
 *  no arguments displays filesystem used/free counts
 *  add stat -> filesystem info
 *  add boot -> write boot block
 *  change dir -> ls [-ld]
 *  rename add -> cp
 *		copies mtime and mode from original file
 *		cp overwrites file if already exists:
 *			copies mtime from original file, keeps destination uid/gid and mode
 *			if destination is directory, copy into directory
 *			otherwise check destination for regular file
 *  rename unlink -> rm
 *	rewrite mknod type path major minor [count inc]
 *			creates mode 644 by default
 *	rename hardlink -> ln
 *	rename symlink -> ln -s
 *	add genfs <directory>
 *	use ELKS defaults of -1 -n14 -i360 -s1440 for mkfs/genfs
 *	add genfs -k option to not copy 0 length (hidden) files starting with .
 *	add addfs option to add files/dirs specified in file from directory
 *
 * Bug fixes by ghaerr:
 * fix mkfs -1, -n overwriting -i, -n14
 * fix mknod char special
 * fix unlink on v1 filesys
 * fix ln, ln -s
 * add getoptX() since Linux and OSX getopt() don't work together
 * fix -k option from always skipping zero-length files
 * fix operation with block numbers > 32767 (requires unsigned int blkno in goto_blk)
 * fix creating files using double indirect blocks (buggy ino_freezone)
 * fix bad image created with files > 775K (DIND block 255 buggy ino_freezone)
 */

#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "minix_fs.h"
#include "protos.h"

#define VERSION	"1.0"

int opt_verbose = 0;
int	opt_keepuid = 0;
int opt_fsbad_fatal = 1;
char *toolname;

void usage(const char *name) {
  printf(
  	"Usage: %s (v%s) [-fpv] imgfile cmd [options]\n"
	"	-f	force even if filesystem state unknown\n"
	"	-p	use existing uid/gid instead of 0/0\n"
	"	-v	verbose\n"
	"cmd:\n"
	"	mkfs [-1|2] [-i<#inodes>] [-n<#direntlen>] [-s<#blocks>]\n"
	"	boot <boot block image>\n"
	"	genfs [-1|2] [-i<#inodes>] [-n<#direntlen>] [-s<#blocks>] [-k] <directory>\n"
	"	addfs <file_of_filenames> <directory>\n"
	"	[stat]\n"
	"	ls [-ld] [filelist...]"
	"	cp source_file dest_dir_or_file\n"
	"	rm file_list ...\n"
	"	mkdir directory_list ...\n"
	"	rmdir directory_list ...\n"
	"	ln [-s] source_file link_name\n"
	"	mknod path [c|b] major minor [count increment]\n"
	"	extract image_file host_file\n"
  	"	cat file_list ...\n"
	"	readlink sym_link\n"
      ,name, VERSION);
  exit(0);
}

void parse_opts(int *argc_p,char ***argv_p) {
  int argc = *argc_p;
  char **argv = *argv_p;

  while (1) {
    int c = getoptX(argc,argv,"pfv");
    if (c == -1) break;
    switch (c) {
    case 'p':
      opt_keepuid = 1;
      break;
    case 'f':
      opt_fsbad_fatal = 0;
      break;
    case 'v':
      opt_verbose = 1;
      break;
    }
  }
  *argc_p = argc-optind;
  *argv_p = argv+optind;
}

void do_cmd(struct minix_fs_dat *fs,int argc,char **argv) {
  if (!strcmp(argv[0],"ls")) {
    cmd_ls(fs,argc,argv);
  } else if (!strcmp(argv[0],"mkdir")) {
    cmd_mkdir(fs,argc,argv, 0755);
  } else if (!strcmp(argv[0],"rmdir")) {
    cmd_rmdir(fs,argc,argv);
  } else if (!strcmp(argv[0],"rm")) {
    cmd_rm(fs,argc,argv);
  } else if (!strcmp(argv[0],"cp")) {
    cmd_cp(fs,argc,argv);
  } else if (!strcmp(argv[0],"cat")) {
    cmd_cat(fs,argc,argv);
  } else if (!strcmp(argv[0],"extract")) {
    cmd_extract(fs,argc,argv);
  } else if (!strcmp(argv[0],"readlink")) {
    cmd_readlink(fs,argc,argv);
  } else if (!strcmp(argv[0],"ln")) {
    cmd_ln(fs,argc,argv);
  } else if (!strcmp(argv[0],"stat")) {
    cmd_sysinfo(fs);
  } else if (!strcmp(argv[0],"mknod")) {
    cmd_mknode(fs,argc,argv);
  } else
      fprintf(stderr,"Invalid command: %s\n",argv[0]);
}

int main(int argc,char **argv) {
  char *filename;

  toolname = argv[0];
  parse_opts(&argc,&argv);
  if (argc < 1) usage(toolname);

  filename = argv[0];
  argv++,argc--;
  
  if (argc > 0 && !strcmp(argv[0],"mkfs")) {
    cmd_mkfs(filename, argc,argv);
  } else if (argc > 0 && !strcmp(argv[0],"genfs")) {
    cmd_genfs(filename, argc,argv);
  } else if (argc > 0 && !strcmp(argv[0],"addfs")) {
    cmd_addfs(filename, argc,argv);
  } else if (argc > 0 && !strcmp(argv[0],"boot")) {
    cmd_boot(filename, argc,argv);
  } else {
    struct minix_fs_dat *fs = open_fs(filename, opt_fsbad_fatal);
    if (argc == 0)
		cmd_sysinfo(fs);
    else {
		do_cmd(fs,argc,argv);
		if (opt_verbose) cmd_sysinfo(fs);
	}
    close_fs(fs);
  }
  return 0;
}
