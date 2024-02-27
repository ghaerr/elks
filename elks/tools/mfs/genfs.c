/*
 * Copyright (C) 2019 Greg Haerr <greg@censoft.com>
 *
 * mfs genfs code
 *
 * modified from original code in ELKS mkromfs.c
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
#if __linux__
#include <sys/sysmacros.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "minix_fs.h"
#include "protos.h"

typedef unsigned short u16_t;
typedef unsigned long u32_t;

struct list_node_s {
	struct list_node_s *prev;
	struct list_node_s *next;
};
typedef struct list_node_s list_node_t;

struct list_root_s {
	list_node_t	node;
	u16_t		count;
};
typedef struct list_root_s list_root_t;


/* Inode to build */
struct inode_build_s {
	list_node_t	node;
	list_root_t	entries;/* list of entries for directory */
	char       *path;
	u16_t		index;	/* inode #*/
	int			dev;	/* major & minor for device */
	u16_t		flags;
	u32_t		blocks;	/* disk blocks required*/
};
typedef struct inode_build_s inode_build_t;


/* Entry to build */
struct entry_build_s {
	list_node_t	node;
	inode_build_t  *inode;
	char           *name;
};
typedef struct entry_build_s entry_build_t;

static list_root_t inodes;	/* list of inodes */
static int	numblocks;
static int	prefix;

/* Double-linked list with near pointers */
#define LIST_LINK \
	prev->next = node; \
	node->prev = prev; \
	next->prev = node; \
	node->next = next

static void
list_init(list_root_t * root)
{
	list_node_t    *node = &root->node;
	node->prev = node;
	node->next = node;
	root->count = 1;		/* root directory inode*/
}

static void 
insert_before(list_node_t * next, list_node_t * node)
{
	list_node_t    *prev = next->prev;
	LIST_LINK;
}

static void 
list_add_tail(list_root_t * root, list_node_t * node)
{
	insert_before(&root->node, node);
	root->count++;
}

/* Memory management */
static entry_build_t *
entry_alloc(inode_build_t * inode, char *name)
{
	entry_build_t  *entry;

	while (1) {
		entry = (entry_build_t *) domalloc(sizeof(entry_build_t), -1);

		entry->name = strdup(name);

		list_add_tail(&inode->entries, &entry->node);
		break;
	}

	return entry;
}


static void 
entry_free(entry_build_t * entry)
{
	if (entry->name)
		free(entry->name);
	free(entry);
}


static inode_build_t *
inode_alloc(inode_build_t * parent, char *path)
{
	inode_build_t  *inode;

	while (1) {
		inode = (inode_build_t *) domalloc(sizeof(inode_build_t), -1);

		list_init(&inode->entries);

		inode->path = strdup(path);

		list_add_tail(&inodes, &inode->node);
		inode->index = inodes.count;	/* 0 = no inode */
		break;
	}

	return inode;
}


static void 
inode_free(inode_build_t * inode)
{
	if (inode->path)
		free(inode->path);

	entry_build_t  *entry = (entry_build_t *) inode->entries.node.next;
	while (entry != (entry_build_t *) & inode->entries.node) {
		entry_build_t  *entry_next = (entry_build_t *) entry->node.next;
		entry_free(entry);
		entry = entry_next;
	}

	free(inode);
}

/* calculate blocks used for passed file size*/
static int
blocksused(size_t filesize)
{
	int b = (filesize + BLOCK_SIZE - 1) >> BLOCK_SIZE_BITS;
	int blks = b;

	if (b > 7)
		blks++;
	if (b > 7+512)
		blks++;
	if (b > 7+512+512*512)
		blks++;
	return blks;
}


/* recursive directory parsing*/
static int 
parse_dir(inode_build_t * grand_parent_inode,
	  inode_build_t * parent_inode, char *parent_path)
{
	int		err;
	DIR		*parent_dir = NULL;

	while (1) {
		parent_dir = opendir(parent_path);
		if (!parent_dir) {
			perror("opendir");
			err = errno;
			break;
		}
		while (1) {
			struct dirent *ent = readdir(parent_dir);
			if (!ent) {
				if (errno)
					perror("readdir");
				err = errno;
				break;
			}
			char *name = ent->d_name;
			entry_build_t  *child_ent = entry_alloc(parent_inode, name);
			if (!child_ent) {
				err = -1;
				break;
			}
			char *child_path = domalloc(strlen(parent_path) + 1 + strlen(name) + 1, -1);
			strcpy(child_path, parent_path);
			strcat(child_path, "/");
			strcat(child_path, name);

			if (!strcmp(name, ".")) {
				child_ent->inode = parent_inode;
			} else if (!strcmp(name, "..")) {
				if (grand_parent_inode)
					child_ent->inode = grand_parent_inode;
				else
					child_ent->inode = parent_inode;	/* self for root */

			} else {
				inode_build_t  *child_inode = inode_alloc(parent_inode, child_path);
				if (!child_inode) {
					err = -1;
					break;
				}
				child_ent->inode = child_inode;

				struct stat	child_stat;
				err = lstat(child_path, &child_stat);
				if (err) {
					perror("lstat");
					err = errno;
					break;
				}
				mode_t		mode = child_stat.st_mode;

				if (S_ISREG(mode)) {
					child_inode->flags = S_IFREG;
					child_inode->blocks = blocksused(child_stat.st_size);
					//printf("File:   %s, size %ld, blocks %ld\n",
						//child_path, (long)child_stat.st_size, child_inode->blocks);
				} else if (S_ISDIR(mode)) {
					//printf("Dir:    %s\n", child_path);
					child_inode->flags = S_IFDIR;
					child_inode->blocks = 1;
					err = parse_dir(parent_inode, child_inode, child_path);
					if (err)
						break;
				} else if (S_ISCHR(mode)) {
					//printf("Char:   %s\n", child_path);
					child_inode->flags = S_IFCHR;
					child_inode->dev = (int)child_stat.st_rdev;
					child_inode->blocks = 0;
				} else if (S_ISBLK(mode)) {
					//printf("Block:  %s\n", child_path);
					child_inode->flags = S_IFBLK;
					child_inode->dev = (int)child_stat.st_rdev;
					child_inode->blocks = 0;
				} else if (S_ISLNK(mode)) {
					//printf("Symlnk: %s\n", child_path);
					child_inode->flags = S_IFLNK;
					child_inode->blocks = 1;
				} else {
					/* Unsupported inode type */
					fatalmsg("Unsupported: %s\n", child_path);
				}
				numblocks += child_inode->blocks;
			}

			if (child_path)
				free(child_path);
		}

		if (err != 0) {
			perror("Read directory");
		}
		break;
	}

	if (parent_dir)
		closedir(parent_dir);

	return err;
}

/* parse list of files/directories to add*/
static int 
parse_filelist(char *filelist, inode_build_t *parent_inode, char *parent_path)
{
	int		err = 0;
	FILE	*fp;
	char	filename[256];

	fp = fopen(filelist, "r");
	if (!fp) die(filelist);

	while (fgets(filename, sizeof(filename), fp))
		{
			char *name = filename;

			if (name[0] == '#')
				continue;
			name[strlen(name)-1] = '\0';	/* remove lf*/
			char *child_path = domalloc(strlen(parent_path) + 1 + strlen(name) + 1, -1);
			strcpy(child_path, parent_path);
			if (name[0] != '/') strcat(child_path, "/");
			strcat(child_path, name);

			inode_build_t  *child_inode = inode_alloc(parent_inode, child_path);
			if (!child_inode) {
				err = -1;
				break;
			}

				struct stat	child_stat;
				err = lstat(child_path, &child_stat);
				if (err)
					fatalmsg("File not found: %s", child_path);
				mode_t		mode = child_stat.st_mode;

				if (S_ISREG(mode)) {
					child_inode->flags = S_IFREG;
					child_inode->blocks = blocksused(child_stat.st_size);
					//printf("File:   %s, size %ld, blocks %ld\n",
						//child_path, (long)child_stat.st_size, child_inode->blocks);
				} else if (S_ISDIR(mode)) {
					//printf("Dir:    %s\n", child_path);
					child_inode->flags = S_IFDIR;
					child_inode->blocks = 1;
				} else if (S_ISCHR(mode)) {
					//printf("Char:   %s\n", child_path);
					child_inode->flags = S_IFCHR;
					child_inode->dev = (int)child_stat.st_rdev;
					child_inode->blocks = 0;
				} else if (S_ISBLK(mode)) {
					//printf("Block:  %s\n", child_path);
					child_inode->flags = S_IFBLK;
					child_inode->dev = (int)child_stat.st_rdev;
					child_inode->blocks = 0;
				} else if (S_ISLNK(mode)) {
					//printf("Symlnk: %s\n", child_path);
					child_inode->flags = S_IFLNK;
					child_inode->blocks = 1;
				} else {
					/* Unsupported inode type */
					fatalmsg("Unsupported: %s\n", child_path);
				}
				numblocks += child_inode->blocks;

				free(child_path);
		}
	fclose(fp);
	return err;
}


/* generate filesystem commands*/
static int 
compile_fs(struct minix_fs_dat *fs)
{
		char major[32], minor[32];
		char *av[6];

		/* Compile the inodes (directories, files, etc) */
		inode_build_t  *inode_build = (inode_build_t *) inodes.node.next;
		for ( ;inode_build != (inode_build_t *) &inodes.node;
					inode_build = (inode_build_t *) inode_build->node.next) {
			u16_t flags = inode_build->flags;

			if (flags == S_IFDIR) {
                struct stat sb;
				if (*(prefix+inode_build->path) == 0) continue; /* skip template dir*/
				if (opt_verbose) printf("mkdir %s\n", prefix+inode_build->path);
                if (stat(inode_build->path, &sb)) die("stat(%s)",inode_build->path);
				av[0] = "mkdir";
				av[1] = prefix+inode_build->path;
				av[2] = 0;
				cmd_mkdir(fs, 2, av, sb.st_mode & 0777);
			} else if (flags == S_IFREG) {
				if (opt_nocopyzero && !inode_build->blocks) {
					char *p = strrchr(inode_build->path, '/');
					if (p && *++p == '.') {
						if (opt_verbose) printf("Skipping %s\n", inode_build->path);
						continue;
					}
				}
				if (opt_verbose) printf("cp %s %s\n", inode_build->path, prefix+inode_build->path);
				av[0] = "cp";
				av[1] = inode_build->path;
				av[2] = prefix+inode_build->path;
				av[3] = 0;
				cmd_cp(fs, 3, av);
			} else if (flags == S_IFCHR) {
				if (opt_verbose) printf("mknod %s c %d %d\n", prefix+inode_build->path,
					major(inode_build->dev), minor(inode_build->dev));
				av[0] = "mknod";
				av[1] = prefix+inode_build->path;
				av[2] = "c";
				av[3] = major; sprintf(major, "%d", major(inode_build->dev));
				av[4] = minor; sprintf(minor, "%d", minor(inode_build->dev));
				av[5] = 0;
				cmd_mknode(fs, 5, av);
			} else if (flags == S_IFBLK) {
				if (opt_verbose) printf("mknod %s b %d %d\n", prefix+inode_build->path,
					major(inode_build->dev), minor(inode_build->dev));
				av[0] = "mknod";
				av[1] = prefix+inode_build->path;
				av[2] = "b";
				av[3] = major; sprintf(major, "%d", major(inode_build->dev));
				av[4] = minor; sprintf(minor, "%d", minor(inode_build->dev));
				av[5] = 0;
				cmd_mknode(fs, 5, av);
			} else if (flags == S_IFLNK) {
				char lnkname[256];
				int cnt = readlink(inode_build->path, lnkname, sizeof(lnkname));
				if (cnt < 0) fatalmsg("Can't read link: %s\n", inode_build->path);
				else {
					lnkname[cnt] = 0;
					if (opt_verbose) printf("ln -s %s %s\n", lnkname, prefix+inode_build->path);
					av[0] = "ln";
					av[1] = "-s";
					av[2] = lnkname;
					av[3] = prefix+inode_build->path;
					av[4] = 0;
					cmd_ln(fs, 4, av);
				}
			}
		}
		return 0;
}


/**
 * Generate a new filesystem
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_genfs(char *filename, int argc,char **argv) {
  int req_inos;
  int req_blks;
  int magic;
  struct minix_fs_dat *fs;
  char *p;
  int err;
  char dirname[256];

  parse_mkfs(argc,argv,&magic,&req_blks,&req_inos);
  argc -= optind;
  argv += optind;
  if (argc != 1) fatalmsg("Usage: genfs [options] <root_template>");

  strcpy(dirname, argv[0]);
  p = strrchr(dirname, '/');
  if (p)
	  p++;
  else p = dirname;
  prefix = p - dirname;                         /* keep template dir if starts with ./ */
  if (prefix != 2 || dirname[0] != '.')
        prefix = p - dirname + strlen(p);       /* skip template dir*/
  if (opt_verbose) printf("Generating filesystem from %s to /%s\n",
        dirname, dirname+prefix);
  numblocks = 2;			/* root inode and first mkdir*/
  list_init(&inodes);
  inode_build_t  *root_node = inode_alloc(NULL, dirname);
  if (!root_node)
	  fatalmsg("No memory");
  root_node->flags = S_IFDIR;
  err = parse_dir(NULL, root_node, dirname);
  if (err)
	fatalmsg("Error parsing directory tree\n");

  //printf("inodes: %d, blocks %d\n", inodes.count, numblocks);

  if (inodes.count > req_inos)
  	fatalmsg("Inodes required %d, available %d\n", inodes.count, req_inos);
  if (numblocks > req_blks)
  	fatalmsg("Blocks required %d, available %d\n", numblocks, req_blks);

  fs = new_fs(filename,magic,req_blks,req_inos);
  err = compile_fs(fs);
  if (opt_verbose) cmd_sysinfo(fs);
  close_fs(fs);

  inode_build_t  *inode = (inode_build_t *) inodes.node.next;
  while (inode != (inode_build_t *) &inodes.node) {
    inode_build_t  *inode_next = (inode_build_t *) inode->node.next;
    inode_free(inode);
    inode = inode_next;
  }
}

/**
 * Add files and directories from passed file to filesystem
 * @param argc - from command line
 * @param argv - from command line
 */
void cmd_addfs(char *filename, int argc,char **argv) {
  struct minix_fs_dat *fs;
  int err;
  char *dirname;

  argv++; argc--;
  if (argc != 2) fatalmsg("Usage: addfs <file_of_filenames> <root_template>");

  dirname = argv[1];
  prefix = 0;
  if (opt_verbose) printf("Adding files from %s to %s\n", argv[0], dirname);
  numblocks = 2;			/* root inode and first mkdir*/
  list_init(&inodes);
  inode_build_t  *root_node = inode_alloc(NULL, dirname);
  if (!root_node)
	  fatalmsg("No memory");
  root_node->flags = S_IFDIR;
  err = parse_filelist(argv[0], root_node, dirname);
  if (err)
	fatalmsg("Error processing %s\n", argv[0]);

  fs = open_fs(filename, opt_fsbad_fatal);

  if (opt_verbose) cmd_sysinfo(fs);
  printf("adding inodes: %d, blocks %d\n", inodes.count, numblocks);
  //if (inodes.count > req_inos)
	//fatalmsg("Inodes required %d, available %d\n", inodes.count, req_inos);
  //if (numblocks > req_blks)
	//fatalmsg("Blocks required %d, available %d\n", numblocks, req_blks);

  err = compile_fs(fs);
  if (opt_verbose) cmd_sysinfo(fs);
  close_fs(fs);

  inode_build_t  *inode = (inode_build_t *) inodes.node.next;
  while (inode != (inode_build_t *) &inodes.node) {
    inode_build_t  *inode_next = (inode_build_t *) inode->node.next;
    inode_free(inode);
    inode = inode_next;
  }
}
