/*
 * Recursive cp for ELKS
 *
 * Nov 2020 Greg Haerr
 * modified from code in ELKS mfs and mkromfs.c
 *
 * Original cp Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <utime.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>

#define BUF_SIZE	BUFSIZ		/* use disk block size for stack limit and efficiency*/

int opt_recurse;	/* implicitly initialized */
int opt_verbose;
int opt_nocopyzero;
int opt_force;
int whole_disk_copy;
char *destination_dir;

char *destdir(char *file);
int do_cp(char *srcfile, char *dstfile);
int do_mkdir(char *srcdir, char *dstdir);
int do_mknod(char *srcfile, char *dstfile, int type, unsigned int major, unsigned int minor);
int do_symlink(char *symlnk, char *file); /* ln -s symlink file*/
int copyfile(char *srcname, char *destname, int setmodes);

#if __APPLE__
#define MAJOR_SHIFT	24	/* OSX: right shift dev_t to get major device # */
#else
#define MAJOR_SHIFT	8
#endif

static char buf[BUF_SIZE];

struct list_node_s {
	struct list_node_s *prev;
	struct list_node_s *next;
};
typedef struct list_node_s list_node_t;

struct list_root_s {
	list_node_t	node;
	unsigned short count;
};
typedef struct list_root_s list_root_t;


/* Inode to build */
struct inode_build_s {
	list_node_t	node;
	list_root_t	entries;/* list of entries for directory */
	char       *path;
	dev_t		dev;	/* major & minor for device */
	unsigned short flags;
	unsigned long blocks;	/* disk blocks required*/
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
static unsigned long numblocks;
static int prefix;

/* Double-linked list with near pointers */
#define LIST_LINK \
	prev->next = node; \
	node->prev = prev; \
	next->prev = node; \
	node->next = next

static void list_init(list_root_t * root)
{
	list_node_t *node = &root->node;
	node->prev = node;
	node->next = node;
	root->count = 1;		/* root directory inode*/
}

static void insert_before(list_node_t * next, list_node_t * node)
{
	list_node_t *prev = next->prev;
	LIST_LINK;
}

static void list_add_tail(list_root_t * root, list_node_t * node)
{
	insert_before(&root->node, node);
	root->count++;
}

/* Memory management */
static entry_build_t * entry_alloc(inode_build_t * inode, char *name)
{
	entry_build_t  *entry;

	entry = (entry_build_t *) malloc(sizeof(entry_build_t));
	if (!entry) {
		fprintf(stderr, "No memory\n");
		return NULL;
	}

	entry->name = strdup(name);
	list_add_tail(&inode->entries, &entry->node);
	return entry;
}

static void entry_free(entry_build_t * entry)
{
	if (entry->name)
		free(entry->name);
	free(entry);
}

static inode_build_t * inode_alloc(inode_build_t * parent, char *path)
{
	inode_build_t  *inode;

	inode = (inode_build_t *) malloc(sizeof(inode_build_t));
	if (!inode) {
		fprintf(stderr, "No memory\n");
		return NULL;
	}

	list_init(&inode->entries);
	inode->path = strdup(path);
	list_add_tail(&inodes, &inode->node);
	//inode->index = inodes.count;	/* 0 = no inode */
	return inode;
}

static void inode_free(inode_build_t * inode)
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

#define BLOCK_SIZE	1024
#define BLOCK_SIZE_BITS	10

/* calculate blocks used for passed file size*/
static unsigned long blocksused(off_t filesize)
{
	unsigned long b = (filesize + BLOCK_SIZE - 1) >> BLOCK_SIZE_BITS;
	unsigned long blks = b;

	if (b > 7)
			blks++;
	if (b > 7+512)
			blks++;
	if (b > 7+512+512L*512)
			blks++;
	return blks;
}


/* recursive directory parsing to collect filenames*/
static int parse_dir(inode_build_t * grand_parent_inode, inode_build_t * parent_inode,
	char *parent_path)
{
	int		err;
	DIR		*parent_dir = NULL;

	errno = 0;
	while (1) {
		parent_dir = opendir(parent_path);
		if (!parent_dir) {
			perror(parent_path);
			err = errno;
			break;
		}
		while (1) {
			struct dirent *ent = readdir(parent_dir);
			if (!ent) {
				if (errno)
					perror(parent_path);
				err = errno;
				break;
			}
			char *name = ent->d_name;
			entry_build_t  *child_ent = entry_alloc(parent_inode, name);
			if (!child_ent) {
				err = -1;
				break;
			}
			char *child_path = malloc(strlen(parent_path) + 1 + strlen(name) + 1);
			strcpy(child_path, parent_path);
			if (strcmp(parent_path, "/") != 0)
				strcat(child_path, "/");
			strcat(child_path, name);
//printf("parent_path %s turned into %s, %s\n", parent_path, name, child_path);

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
					perror(child_path);
					err = errno;
					break;
				}
				mode_t mode = child_stat.st_mode;

				if (S_ISREG(mode)) {
					child_inode->flags = S_IFREG;
					child_inode->blocks = blocksused(child_stat.st_size);
					//printf("File:   %s, size %ld, blocks %ld\n",
					//child_path, (long)child_stat.st_size, child_inode->blocks);
				} else if (S_ISDIR(mode)) {
					//printf("Dir:    %s\n", child_path);
					child_inode->flags = S_IFDIR;
					child_inode->blocks = 1;
//printf("REPARSE %s\n", child_path);
					/* handle special case of whole disk copy*/
					if (whole_disk_copy && destination_dir[0] == '/') {
						char *p = destination_dir;
						p = strchr(destination_dir+1, '/');
						if (p) {
							int old = *p; *p = 0;
							int same = !strcmp(child_path, destination_dir);
							*p = old;
							if (same) {
								printf("Skipping %s\n", child_path);
								child_inode->flags = 0;
								goto next_entry;
							}
						}
					}
					err = parse_dir(parent_inode, child_inode, child_path);
					if (err)
						break;
				} else if (S_ISCHR(mode)) {
					//printf("Char:   %s\n", child_path);
					child_inode->flags = S_IFCHR;
					child_inode->dev = child_stat.st_rdev;
					child_inode->blocks = 0;
				} else if (S_ISBLK(mode)) {
					//printf("Block:  %s\n", child_path);
					child_inode->flags = S_IFBLK;
					child_inode->dev = child_stat.st_rdev;
					child_inode->blocks = 0;
				} else if (S_ISLNK(mode)) {
					//printf("Symlnk: %s\n", child_path);
					child_inode->flags = S_IFLNK;
					child_inode->blocks = 1;
				} else {
					/* Unsupported inode type */
					printf("Skipping unsupported file type: %s (0%o)\n",
						child_path, mode);
					child_inode->flags = 0;
					child_inode->blocks = 0;
				}
				numblocks += child_inode->blocks;
			}
next_entry:
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

/* do actual copies, mknods, symlinks, etc*/
static int do_copies(void)
{
	int err = 0;
	inode_build_t  *inode_build = (inode_build_t *) inodes.node.next;

	for ( ;inode_build != (inode_build_t *) &inodes.node;
				inode_build = (inode_build_t *) inode_build->node.next) {
		unsigned short flags = inode_build->flags;

		if (flags == S_IFDIR) {
			if (*(prefix+inode_build->path) == 0) continue; /* skip start dir*/

				err |= do_mkdir(inode_build->path, destdir(inode_build->path));
		} else if (flags == S_IFREG) {
			if (opt_nocopyzero && !inode_build->blocks) {
				char *p = strrchr(inode_build->path, '/');
				if (p && *++p == '.') {
					if (opt_verbose)
						printf("Skipping zero length %s\n", inode_build->path);
					continue;
				}
			}

			err |= do_cp(inode_build->path, destdir(inode_build->path));
		} else if (flags == S_IFCHR) {

			err |= do_mknod(inode_build->path, destdir(inode_build->path), flags,
				inode_build->dev >> MAJOR_SHIFT, inode_build->dev & 0xff);
		} else if (flags == S_IFBLK) {

			err |= do_mknod(inode_build->path, destdir(inode_build->path), flags,
				inode_build->dev >> MAJOR_SHIFT, inode_build->dev & 0xff);
		} else if (flags == S_IFLNK) {
			char lnkname[PATH_MAX];

			int cnt = readlink(inode_build->path, lnkname, sizeof(lnkname) - 1);
			if (cnt < 0) {
				fprintf(stderr, "Can't read symlink "); fflush(stderr);
				perror(inode_build->path);
				err = 1;
			} else {
				lnkname[cnt] = 0;

				err |= do_symlink(lnkname, destdir(inode_build->path));
			}
		}
	}
	return err;
}

char *destdir(char *file)
{
	static char dir[PATH_MAX];

	strcpy(dir, destination_dir);
	if (file[prefix] != '/')
		strcat(dir, "/");
	strcat(dir, prefix+file);
	return dir;
}

int do_cp(char *srcfile, char *dstfile)
{
	if (opt_verbose) printf("Copying %s to %s\n", srcfile, dstfile);

	return copyfile(srcfile, dstfile, 1);
}

int do_mkdir(char *srcdir, char *dstdir)
{
	struct stat sbuf;

	//if (opt_verbose) printf("mkdir %s\n", dstdir);

	if (lstat(srcdir, &sbuf) < 0) {
		perror(srcdir);
		return 1;
	}

	if (mkdir(dstdir, sbuf.st_mode & 0777) < 0) {
		fprintf(stderr, "Can't make directory "); fflush(stderr);
		perror(dstdir);
		return 1;
	}
	return 0;
}

int do_mknod(char *srcfile, char *dstfile, int type, unsigned int major,
	unsigned int minor)
{
	struct stat sbuf;

	if (opt_verbose) printf("mknod %s %c %d %d\n", dstfile,
		type == S_IFCHR? 'c': 'b', major, minor);
	if (lstat(srcfile, &sbuf) < 0) {
		perror(srcfile);
		return 1;
	}

	if (mknod(dstfile, sbuf.st_mode, sbuf.st_rdev)) {
		fprintf(stderr, "Can't mknod mode 0%o dev 0x%x ", sbuf.st_mode, sbuf.st_rdev);
        fflush(stderr);
		perror(dstfile);
		return 1;
	}
	return 0;
}

/* ln -s symlnk file*/
int do_symlink(char *symlnk, char *file)
{
	if (opt_verbose) printf("Symlink %s -> %s\n", file, symlnk);

	if (symlink(symlnk, file) < 0) {
		fprintf(stderr, "Can't create symlink "); fflush(stderr);
		perror(symlnk);
		return 1;
	}
	return 0;
}

/* recursive directory copy, dest_dir may not exist*/
int copy_directory(char *source_dir, char *dest_dir)
{
	int err;
	char *p;
	struct stat sbuf;

	p = strrchr(source_dir, '/');
	if (p)
		p++;
	else p = source_dir;
	prefix = p - source_dir + strlen(p);	/* skip start dir*/
	destination_dir = dest_dir;
	whole_disk_copy = !strcmp(source_dir, "/");	/* check fails with . in / */

	/* if destination exists, must be directory*/
	if (lstat(dest_dir, &sbuf) == 0) {
		if (!S_ISDIR(sbuf.st_mode)) {
			fprintf(stderr, "%s: not a directory\n", dest_dir);
			return 1;
		}
	} else {
		/* destination doesn't exist, create directory*/
		if (mkdir(dest_dir, 0777) < 0) {
			fprintf(stderr, "Can't make directory "); fflush(stderr);
			perror(dest_dir);
			return 1;
		}
	}

	numblocks = 2;			/* root inode and first mkdir*/
	list_init(&inodes);
	inode_build_t *root_node = inode_alloc(NULL, source_dir);
	if (!root_node)
		return 1;
	root_node->flags = S_IFDIR;

	if (parse_dir(NULL, root_node, source_dir)) {
		//fprintf(stderr, "Error reading directory tree\n");
		return 1;
	}

	if (opt_verbose) printf("Copying %d files, %lu blocks from %s to %s\n", inodes.count, numblocks,
		source_dir, destination_dir);

	err = do_copies();

	inode_build_t  *inode = (inode_build_t *) inodes.node.next;
	while (inode != (inode_build_t *) &inodes.node) {
		inode_build_t  *inode_next = (inode_build_t *) inode->node.next;
		inode_free(inode);
		inode = inode_next;
	}
	return err;
}

/*
 * Build a path name from the specified directory name and file name.
 * If the directory name is NULL, then the original filename is returned.
 * The built path is in a static area, and is overwritten for each call.
 */
char *buildname(char *dirname, char *filename)
{
	char		*cp;
	static	char	buf[PATH_MAX];

	if ((dirname == NULL) || (*dirname == '\0'))
		return filename;

	cp = strrchr(filename, '/');
	if (cp) filename = cp + 1;

	strcpy(buf, dirname);
	strcat(buf, "/");
	strcat(buf, filename);

	return buf;
}


/*
 * Return 1 if a filename is a directory.
 * Nonexistant files return 0.
 */
int isadir(char *name)
{
	struct	stat	statbuf;

	if (stat(name, &statbuf) < 0) return 0;
	return S_ISDIR(statbuf.st_mode);
}

/*
 * Copy one file to another, while possibly preserving its modes, times,
 * and modes.  Returns 0 if successful, or 1 on a failure with an
 * error message output.  (Failure is not indicated if the attributes cannot
 * be set.)
 */
int copyfile(char *srcname, char *destname, int setmodes)
{
	int		rfd;
	int		wfd;
	int		rcc;
	int		wcc;
	char		*bp;
	struct	stat	statbuf1;
	struct	stat	statbuf2;
	struct	utimbuf	times;

	if (stat(srcname, &statbuf1) < 0) {
		perror(srcname);
		return 1;
	}

	if (stat(destname, &statbuf2) < 0) {
		statbuf2.st_ino = -1;
		statbuf2.st_dev = -1;
	}

	if ((statbuf1.st_dev == statbuf2.st_dev) &&
		(statbuf1.st_ino == statbuf2.st_ino))
	{
		fprintf(stderr, "Copying file \"%s\" to itself\n", srcname);
		return 1;
	}

	rfd = open(srcname, 0);
	if (rfd < 0) {
		perror(srcname);
		return 1;
	}

	wfd = creat(destname, statbuf1.st_mode);
	if (wfd < 0) {
		if (opt_force) {
			if ((wfd = unlink(destname)) >= 0)
				wfd = creat(destname, statbuf1.st_mode);
		}
		if (wfd < 0) {
			perror(destname);
			close(rfd);
			return 1;
		}
	}

	while ((rcc = read(rfd, buf, BUF_SIZE)) > 0) {
		bp = buf;
		while (rcc > 0) {
			wcc = write(wfd, bp, rcc);
			if (wcc < 0) {
				perror(destname);
				goto error_exit;
			}
			bp += wcc;
			rcc -= wcc;
		}
	}

	if (rcc < 0) {
		perror(srcname);
		goto error_exit;
	}

	close(rfd);
	close(wfd);

	if (setmodes) {
		(void) chmod(destname, statbuf1.st_mode);
		(void) chown(destname, statbuf1.st_uid, statbuf1.st_gid);

		times.actime = statbuf1.st_atime;
		times.modtime = statbuf1.st_mtime;

		(void) utime(destname, &times);
	}

	return 0;

error_exit:
	close(rfd);
	close(wfd);

	return 1;
}

void usage(void)
{
	fprintf(stderr, "usage: cp [-R][-v] source [...] target_file_or_directory\n");
	exit(1);
}

int main(int argc, char **argv)
{
	int	dirflag, err = 0;
	char	*p;
	char	*srcname;
	char	*destname;
	char	*lastarg;

	while (argv[1] && argv[1][0] == '-') {
		for (p = &argv[1][1]; *p; p++) {
		    if (*p == 'R')
			    opt_recurse = 1;
		    else if (*p == 'v')
			    opt_verbose = 1;
		    else if (*p == 'f')
			    opt_force = 1;
		    else usage();
        }
		argv++;
		argc--;
	}
	if (argc < 3) usage();

	lastarg = argv[argc - 1];
	dirflag = isadir(lastarg);

	if (argc > 3 && !dirflag) {
		fprintf(stderr, "%s: not a directory\n", lastarg);
		return 1;
	}

	while (argc-- > 2) {
		srcname = argv[1];
		destname = lastarg;
		if (opt_recurse) {
			err |= copy_directory(srcname, destname);
		} else {
			if (dirflag) destname = buildname(destname, srcname);
			if (copyfile(*++argv, destname, 0)) {
				fprintf(stderr, "Failed to copy %s -> %s\n", srcname, destname);
				return 1;
			}
		}
	}
	sync();
	return err;
}
