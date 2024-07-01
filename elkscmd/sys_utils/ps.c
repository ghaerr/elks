/*
 * ps.c
 * Copyright 1998 Alistair Riddoch
 * ajr@ecs.soton.ac.uk
 *
 * This file may be distributed under the terms of the GNU General Public
 * License v2, or at your option any later version.
 *
 * This is a small version of ps for use in the ELKS project.
 * Enhanced by Greg Haerr 17 Apr 2020
 */
#define __KERNEL__
#include <linuxmt/ntty.h>       /* for struct tty */
#undef __KERNEL__

#include <autoconf.h>           /* for CONFIG_ options */
#include <linuxmt/mm.h>
#include <linuxmt/mem.h>
#include <linuxmt/major.h>
#include <linuxmt/sched.h>
#include <linuxmt/fixedpt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <getopt.h>
#include <paths.h>
#include <libgen.h>

#define LINEARADDRESS(off, seg)		((off_t) (((off_t)seg << 4) + off))

static int maxtasks;

int memread(int fd, word_t off, word_t seg, void *buf, int size)
{
	if (lseek(fd, LINEARADDRESS(off, seg), SEEK_SET) == -1)
		return 0;

	if (read(fd, buf, size) != size)
		return 0;

	return 1;
}

word_t getword(int fd, word_t off, word_t seg)
{
	word_t word;

	if (!memread(fd, off, seg, &word, sizeof(word)))
		return 0;
	return word;
}

void process_name(int fd, unsigned int off, unsigned int seg)
{
	word_t argc, argv;
	char buf[80];

	argc = getword(fd, off, seg);

	while (argc-- > 0) {
		off += 2;
		argv = getword(fd, off, seg);
		if (!memread(fd, argv, seg, buf, sizeof(buf)))
			return;
		printf("%s ",buf);
	}
}

/* fast cached version of devname() */
char *dev_name(unsigned int minor)
{
	struct dirent *d;
	dev_t ttydev = MKDEV(TTY_MAJOR, minor);
	static dev_t prevdev = -1;
	static DIR *fp = NULL;
	struct stat st;
	static char path[MAXNAMLEN+6] = _PATH_DEVSL;    /* /dev/ */
#define NAMEOFF     (sizeof(_PATH_DEVSL) - 1)

	if (prevdev == ttydev) return path+NAMEOFF+3;
	if (!fp) {
		if (!(fp = opendir(_PATH_DEV)))
			return "??";
	} else rewinddir(fp);

	while ((d = readdir(fp)) != 0) {
		if (d->d_name[0] == '.')
			continue;
		if (strncmp(d->d_name, "tty", 3))
			continue;
		strcpy(&path[NAMEOFF], d->d_name);
		if (!stat(path, &st) && st.st_rdev == ttydev) {
			prevdev = ttydev;
			return path+NAMEOFF+3;
		}
	}
	return "?";
}

char *tty_name(int fd, unsigned int off, unsigned int seg)
{
	off_t addr = ((off_t)seg << 4) + off;
	struct tty tty;

	if (off == 0)
		return "";
	if (lseek(fd, addr, SEEK_SET) == -1) return "?";

	if (read(fd, &tty, sizeof(tty)) != sizeof(tty)) return "?";

	return dev_name(tty.minor);
}

int main(int argc, char **argv)
{
	int c, fd;
	unsigned int j, ds, off;
	word_t cseg, dseg;
	struct passwd * pwent;
    int f_listall = 0;
    char *progname;
    int f_uptime;
    struct task_struct task_table;

    if ((progname = strrchr(argv[0], '/')) != NULL)
        progname++;
    else progname = argv[0];
    f_uptime = !strcmp(progname, "uptime");
    while ((c = getopt(argc, argv, "lu")) != -1) {
        switch (c) {
        case 'l':       /* list all - CSEG/DSEG */
            f_listall = 1;
            break;
        case 'u':       /* uptime */
            f_uptime = 1;
            break;
        default:
            printf("Usage: %s: [-lu]\n", progname);
            return 1;
        }
    }

	if ((fd = open("/dev/kmem", O_RDONLY)) < 0) {
		printf("ps: no /dev/kmem\n");
		return 1;
	}
	if (ioctl(fd, MEM_GETDS, &ds) < 0 ||
		ioctl(fd, MEM_GETMAXTASKS, &maxtasks) < 0) {
		printf("ps: ioctl mem_getds\n");
		return 1;
	}

#ifdef CONFIG_CPU_USAGE
    if (f_uptime) {
        jiff_t uptime;
        unsigned int upoff;

	    if (ioctl(fd, MEM_GETUPTIME, &upoff) < 0 ||
                !memread(fd, upoff, ds, &uptime, sizeof(uptime))) {
		    printf("ps: ioctl mem_getuptime\n");
		    return 1;
	    }

        unsigned long n = uptime / HZ;
        int days = n / (24 * 3600L);
        n = n % (24 * 3600L);
        int hours = n / 3600L;
        n %= 3600;
        int minutes = n / 60 ;

        printf("up for %d days, %d hour%s, and %d minute%s\n",
            days, hours, hours == 1? "": "s", minutes, minutes == 1? "": "s");
        return 0;
    }
#endif

	if (ioctl(fd, MEM_GETTASK, &off) < 0) {
		printf("ps: ioctl mem_gettask\n");
		return 1;
	}

	printf("  PID   GRP  TTY USER STAT ");
#ifdef CONFIG_CPU_USAGE
    printf("CPU");
#endif
    printf(" ");
	if (f_listall) printf("CSEG DSEG ");
	printf(" HEAP  FREE   SIZE COMMAND\n");
	for (j = 1; j < maxtasks; j++) {
		if (!memread(fd, off + j*sizeof(struct task_struct), ds, &task_table, sizeof(task_table))) {
			printf("ps: memread\n");
			return 1;
		}

		switch (task_table.state) {
		case TASK_UNUSED:			continue;
		case TASK_RUNNING:			c = 'R'; break;
		case TASK_INTERRUPTIBLE:	c = 'S'; break;
		case TASK_UNINTERRUPTIBLE:	c = 's'; break;
		case TASK_STOPPED:			c = 'T'; break;
		case TASK_ZOMBIE:			c = 'Z'; break;
		case TASK_EXITING:			c = 'E'; break;
		default:					c = '?'; break;
		}

		if (task_table.kstack_magic != KSTACK_MAGIC) {
			printf("Recompile ps, mismatched task structure\n");
			return 1;
		}

		pwent = getpwuid(task_table.uid);

		/* pid grp tty user stat*/
		printf("%5d %5d %4s %-8s%c ",
				task_table.pid,
				task_table.pgrp,
				tty_name(fd, (unsigned int)task_table.tty, ds),
				(pwent ? pwent->pw_name : "unknown"),
				c);

#ifdef CONFIG_CPU_USAGE
        {
            /* Round up, then divide by 2 for %. Change if SAMP_FREQ not 2 */
            unsigned long cpu_percent = (task_table.average + FIXED_HALF) >> 1;
            printf("%3d", FIXED_INT(cpu_percent));
        }
#endif
		/* CSEG*/
		cseg = (word_t)task_table.mm[SEG_CODE];
		if (f_listall) printf(" %4x ",
            cseg? getword(fd, (word_t)cseg+offsetof(struct segment, base), ds): 0);

		/* DSEG*/
		dseg = (word_t)task_table.mm[SEG_DATA];
		if (f_listall) printf("%4x",
            dseg? getword(fd, (word_t)dseg+offsetof(struct segment, base), ds): 0);

		if (dseg) {
			/* heap*/
			printf(" %5u ", (word_t)(task_table.t_endbrk - task_table.t_enddata));

			/* free*/
			printf("%5u ", (word_t)(task_table.t_regs.sp - task_table.t_endbrk));

			/* stack*/
			//printf("%5u ", (word_t)(task_table.t_begstack - task_table.t_regs.sp));

			/* size*/
			segext_t size = getword(fd, (word_t)cseg+offsetof(struct segment, size), ds)
							+ getword(fd, (word_t)dseg+offsetof(struct segment, size), ds);
			printf("%6ld ", (long)size << 4);

			process_name(fd, task_table.t_begstack, task_table.t_regs.ss);
		}
		printf("\n");
	}
	return 0;
}
