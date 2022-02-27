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

static char *fs_typename[] = {
	0, "minix", "msdos", "romfs"
};

static struct dev_name_struct {
        char *name;
        dev_t num;
} devices[] = {
        /* root_dev_name needs first 5 in order*/
        { "hda",     0x0300 },
        { "hdb",     0x0320 },
        { "hdc",     0x0340 },
        { "hdd",     0x0360 },
        { "fd0",     0x0380 },
        { "fd1",     0x03a0 },
        { "ssd",     0x0200 },
        { "rd",      0x0100 },
        { NULL,           0 }
};

/*
 * Convert a block device number to name.
 */
static char *dev_name(dev_t dev)
{
        int i;
#define NAMEOFF 5
        static char name[10] = "/dev/";

        for (i=0; i<sizeof(devices)/sizeof(devices[0])-1; i++) {
                if (devices[i].num == (dev & 0xfff0)) {
                        strcpy(&name[NAMEOFF], devices[i].name);
                        if (i < 3) {
                                if (dev & 0x03) {
                                        name[NAMEOFF+3] = '0' + (dev & 3);
                                        name[NAMEOFF+4] = 0;
                                }
                        }
                        return name;
                }
        }
        return NULL;
}

static int show_mount(dev_t dev)
{
	struct statfs statfs;

	if (ustatfs(dev, &statfs) < 0)
		return -1;

	printf("%-8s (%s) blocks %6lu free %6lu mount %s\n",
		dev_name(statfs.f_dev), fs_typename[statfs.f_type], statfs.f_blocks,
		statfs.f_bfree, statfs.f_mntonname);
	return 0;
}

static void show(void)
{
	int i = 0;

	for (;;)
		if (show_mount(i++) < 0)
			break;
}

static void usage(void)
{
	write(STDERR_FILENO, "Usage: mount [-a][-q][-t type] [-o ro|remount,rw] <device> <directory>\n", 71);
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

	if (argc == 0) {
		show();
		return 0;
	}

	if (argc != 2) {
		usage();
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
