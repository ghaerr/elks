
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "list.h"



/* INode on disk (actually in ROM) */

#define INODE_FILE 0x0001
#define INODE_DIR  0x0002

struct super_disk_s
	{
	};

struct inode_disk_s
	{
	word_t flags;
	addr_t offset;
	addr_t size;
	};

typedef struct inode_disk_s inode_disk_t;

static addr_t _offset = 0;

/* INode to build */

struct inode_build_s
	{
	list_node_t node;
	list_root_t entries;  /* list of entries for directory */
	char * path;

	word_t flags;
	addr_t offset;
	addr_t size;
	};

typedef struct inode_build_s inode_build_t;

static list_root_t _inodes;  /* list of inodes */


static inode_build_t * inode_alloc (inode_build_t * parent, char * path)
	{
	inode_build_t * inode;

	while (1)
		{
		inode = (inode_build_t *) malloc (sizeof (inode_build_t));
		assert (inode);

		list_init (&inode->entries);

		inode->path = strdup (path);
		assert (inode->path);

		list_add_tail (&_inodes, &inode->node);
		break;
		}

	return inode;
	}


static void inode_free (inode_build_t * inode)
	{
	if (inode->path) free (inode->path);
	/* TODO: empty entries */
	}


/* Entry to build */

struct entry_build_s
	{
	list_node_t node;
	inode_build_t * inode;
	char * name;
	};

typedef struct entry_build_s entry_build_t;


static entry_build_t * entry_alloc (inode_build_t * inode, char * name)
	{
	entry_build_t * entry;

	while (1)
		{
		entry = (entry_build_t *) malloc (sizeof (entry_build_t));
		assert (entry);

		entry->name = strdup (name);
		assert (entry->name);

		list_add_tail (&inode->entries, &entry->node);
		break;
		}

	return entry;
	}


static void entry_free (entry_build_t * entry)
	{
	if (entry->name) free (entry->name);
	}


/* Recursive directory parsing */

int parse_dir (inode_build_t * grand_parent_inode, inode_build_t * parent_inode, char * parent_path)
	{
	int err;

	DIR * parent_dir = NULL;

	while (1)
		{
		parent_dir = opendir (parent_path);
		if (!parent_dir)
			{
			err = errno;
			break;
			}

		while (1)
			{
			struct dirent * ent = readdir (parent_dir);
			if (!ent)
				{
				err = errno;
				break;
				}

			char * name = ent->d_name;
			entry_build_t * child_ent = entry_alloc (parent_inode, name);
			if (!child_ent)
				{
				err = -1;
				break;
				}

			char * child_path = malloc (strlen (parent_path) + 1 + strlen (name) + 1);
			strcpy (child_path, parent_path);
			strcat (child_path, "/");
			strcat (child_path, name);

			if (!strcmp (name, "."))
				{
				printf ("Pseudo: %s\n", child_path);
				child_ent->inode = parent_inode;
				}

			else if (!strcmp (name, ".."))
				{
				printf ("Pseudo: %s\n", child_path);
				child_ent->inode = grand_parent_inode;
				}

			else
				{
				inode_build_t * child_inode = inode_alloc (parent_inode, child_path);
				if (!child_inode)
					{
					err = -1;
					break;
					}

				child_ent->inode = child_inode;

				struct stat child_stat;
				err = lstat (child_path, &child_stat);
				if (err)
					{
					err = errno;
					break;
					}

				if (S_ISREG (child_stat.st_mode))
					{
					printf ("File:   %s\n", child_path);
					child_inode->flags = INODE_FILE;
					child_inode->size = child_stat.st_size;
					}

				else if (S_ISDIR (child_stat.st_mode))
					{
					printf ("Dir:    %s\n", child_path);
					child_inode->flags = INODE_DIR;
					err = parse_dir (parent_inode, child_inode, child_path);
					if (err) break;
					}

				else
					{
					assert (0);
					}
				}

			if (child_path) free (child_path);
			}

		if (err != 0)
			{
			perror ("Read directory");
			}

		break;
		}

	if (parent_dir) closedir (parent_dir);

	return err;
	}


/* Main */

void main ()
	{
	while (1)
		{
		int err;

		puts ("Parsing file system starting from . directory...");

		list_init (&_inodes);

		inode_build_t * root_node = inode_alloc (NULL, ".");
		if (!root_node) break;

		root_node->flags = INODE_DIR;

		err = parse_dir (NULL, root_node, ".");
		if (err) break;

		printf ("Total inodes: %u\n", _inodes.count);
		puts ("End of parsing.");

		puts ("Compiling file system...");

		/*err = compile_fs ();*/

		puts ("End of compilation.");
		break;
		}
	}

