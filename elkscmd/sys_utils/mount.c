/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Most simple built-in commands are here.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <utime.h>
#include <errno.h>

void
main(argc, argv)
	char	**argv;
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
					write(STDERR_FILENO, "Missing file system type\n", 25);
					exit(1);
				}

				type = *argv++;
				argc--;
				break;

			default:
				write(STDERR_FILENO, "Unknown option\n", 15);
				exit(1);
		}
	}

	if (argc != 2) {
		write(STDERR_FILENO, "Wrong number of arguments for mount\n", 36);
		exit(1);
	}

	if (mount(argv[0], argv[1], type, 0) < 0) {
		perror("mount failed");
		exit(1);
	}
	exit(0);
}
