/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Most simple built-in commands are here.
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mount.h>

int main(int argc, char **argv)
{
	char	*str;
	char	*type;

	argc--;
	argv++;
	type = "minix";

	while ((argc > 0) && (**argv == '-')) {
		argc--;
		str = *argv++ ;

		while (*++str) switch (*str) {
			case 't':
				if ((argc <= 0) || (**argv == '-')) {
					write(STDERR_FILENO, "mount: missing file system type\n", 32);
					return 1;
				}

				type = *argv++;
				argc--;
				break;

			default:
				write(STDERR_FILENO, "mount: unknown option\n", 22);
				return 1;
		}
	}

	if (argc != 2) {
		write(STDERR_FILENO, "Usage: mount [-t type] <device> <directory>\n", 44);
		return 1;
	}

	if (mount(argv[0], argv[1], type, 0) < 0) {
		perror("mount failed");
		return 1;
	}
	return 0;
}
