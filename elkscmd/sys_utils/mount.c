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
					write(STDERR_FILENO, "mount: missing file system type\n", 32);
					return 1;
				}

				option = *argv++;
				if (!strcmp(option, "minix"))
					type = FST_MINIX;
				else if (!strcmp(option, "msdos") || !strcmp(option, "fat"))
					type = FST_MSDOS;
				else if (!strcmp(option, "romfs"))
					type = FST_ROMFS;
				argc--;
				break;

			case 'o':
				if ((argc <= 0) || (**argv == '-')) {
					write(STDERR_FILENO, "mount: missing option string\n", 29);
					return 1;
				}

				option = *argv++;
				if (!strcmp(option, "ro"))
					flags |= MS_RDONLY;
				else if (!strcmp(option, "remount,rw"))
					flags |= MS_REMOUNT;
				else if (!strcmp(option, "remount,ro"))
					flags |= MS_REMOUNT|MS_RDONLY;
				else {
					write(STDERR_FILENO, "mount: bad option string\n", 25);
					return 1;
				}
				argc--;
				break;

			default:
				write(STDERR_FILENO, "mount: unknown option\n", 22);
				return 1;
		}
	}

	if (argc != 2) {
		write(STDERR_FILENO, "Usage: mount [-a][-q][-t type] [-o ro|remount,rw] <device> <directory>\n", 71);
		return 1;
	}

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
