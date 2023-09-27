/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Sep 2020 - added ro and remount,rw options - ghaerr
 * Feb 2022 - add -a auto mount w/o type specifier, -q query fs type
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <linuxmt/fs.h>
#include <linuxmt/limits.h>

#define errmsg(str) write(STDERR_FILENO, str, sizeof(str) - 1)

static char *fs_typename[] = {
	0, "minix", "msdos", "romfs"
};

static int show_mount(dev_t dev)
{
	struct statfs statfs;

	if (ustatfs(dev, &statfs, UF_NOFREESPACE) < 0)
		return -1;

	if (statfs.f_type < FST_MSDOS) 
		printf("%-9s (%5s) blocks %6lu free %6lu mount %s\n",
		devname(statfs.f_dev, S_IFBLK), fs_typename[statfs.f_type], statfs.f_blocks,
		statfs.f_bfree, statfs.f_mntonname);
	else
		printf("%-9s (%5s)                           mount %s\n",
		devname(statfs.f_dev, S_IFBLK), fs_typename[statfs.f_type], statfs.f_mntonname);
	return 0;
}

static void show(void)
{
	int i;

	for (i = 0; i < NR_SUPER; i++)
		show_mount(i);
}

static int usage(void)
{
	errmsg("usage: mount [-a][-q][-t minix|fat][-o ro|remount,{rw|ro}] <device> <directory>\n");
    return 1;
}

int main(int argc, char **argv)
{
	char	*str;
	int		type = 0;		/* default fs*/
	int		flags = 0;
	int		query = 0;
	char	*option;

	argc--;
	argv++;

	while ((argc > 0) && (**argv == '-')) {
		argc--;
		str = *argv++ ;

		while (*++str) switch (*str) {
			case 'q':
				query = 1;
				/* fall through and automount */
			case 'a':
				flags |= MS_AUTOMOUNT;
				break;
			case 't':
				if ((argc <= 0) || (**argv == '-')) {
					errmsg("mount: missing file system type\n");
					return 1;
				}

				option = *argv++;
				if (!strcmp(option, "minix"))
					type = FST_MINIX;
				else if (!strcmp(option, "msdos") || !strcmp(option, "fat"))
					type = FST_MSDOS;
				else if (!strcmp(option, "romfs"))
					type = FST_ROMFS;
				else return usage();
				argc--;
				break;

			case 'o':
				if ((argc <= 0) || (**argv == '-')) {
					return usage();
				}

				option = *argv++;
				if (!strcmp(option, "ro"))
					flags |= MS_RDONLY;
				else if (!strcmp(option, "remount,rw"))
					flags |= MS_REMOUNT;
				else if (!strcmp(option, "remount,ro"))
					flags |= MS_REMOUNT|MS_RDONLY;
				else {
					return usage();
				}
				argc--;
				break;

			default:
				return usage();
		}
	}

	if (argc == 0) {
		show();
		return 0;
	}

	if (argc != 2) {
		return usage();
	}

	if (flags == 0 && type == 0)
		flags = MS_AUTOMOUNT;
	if (mount(argv[0], argv[1], type, flags) < 0) {
		if (flags & MS_AUTOMOUNT) {
			type = (!type || type == FST_MINIX)? FST_MSDOS: FST_MINIX;
			if (mount(argv[0], argv[1], type, flags) >= 0)
				goto mount_ok;
		}
		if (!query)
			perror("mount failed");
		return 1;
	}

mount_ok:
	/* if query return type: 1=fail, 2=MINIX, 3=FAT */
	if (query)
		return (!type || type == FST_MINIX)? 2: 3;
	return 0;
}
