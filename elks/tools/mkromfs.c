
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


typedef unsigned short word_t;


/* Node on disk (actually in ROM) */

#define NODE_FILE 0x0001
#define NODE_DIR  0x0002

struct node_disk_s
	{
	word_t flags;
	word_t offset;
	word_t size;
	};


/* Node to build */

struct node_build_s
	{
	char * path;
	word_t offset;
	word_t size;
	};

typedef struct node_build_s node_build_t;


/* Array of build nodes */

#define MAX_NODES 1000

static node_build_t _nodes [MAX_NODES];
static word_t _node_count = 0;


static node_build_t * node_alloc (node_build_t * parent, char * name)
	{
	node_build_t * node = NULL;

	while (1)
		{
		if (_node_count >= MAX_NODES) break;

		node = &_nodes [_node_count++];

		int len = strlen (name) + 1;
		if (parent) len += strlen (parent->path) + 1;

		node->path = malloc (len);
		assert (node->path);

		if (parent)
			{
			strcpy (node->path, parent->path);
			strcat (node->path, "/");
			strcat (node->path, name);
			}
		else
			{
			strcpy (node->path, name);
			}

		break;
		}

	return node;
	}

static void node_free (node_build_t * node)
	{
	if (node->path) free (node->path);
	}



/* Recursive directory parsing */

int parse_dir (node_build_t * parent_node, char * parent_path)
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

			node_build_t * child_node = node_alloc (parent_node, ent->d_name);
			if (!child_node)
				{
				err = -1;
				break;
				}

			if (!strcmp (ent->d_name, "."))
				{
				}

			else if (!strcmp (ent->d_name, ".."))
				{
				}

			else
				{
				char child_path [256];
				strcpy (child_path, parent_path);
				strcat (child_path, "/");
				strcat (child_path, ent->d_name);

				struct stat child_stat;
				err = lstat (child_path, &child_stat);
				if (err)
					{
					err = errno;
					break;
					}

				if (S_ISREG (child_stat.st_mode))
					{
					printf ("file: %s\n", child_path);
					}

				else if (S_ISDIR (child_stat.st_mode))
					{
					printf ("dir: %s\n", child_path);
					err = parse_dir (child_node, child_path);
					if (err) break;
					}

				else
					{
					assert (0);
					}
				}
			}

		if (err != 0)
			{
			perror ("Read directory");
			break;
			}

		err = 0;
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
		node_build_t * root_node = node_alloc (NULL, "");
		if (!root_node) break;

		parse_dir (root_node, ".");
		break;
		}
	}

