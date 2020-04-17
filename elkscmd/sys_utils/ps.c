/*
 * ps.c
 *
 * Copyright 1998 Alistair Riddoch
 * ajr@ecs.soton.ac.uk
 *
 * This file may be distributed under the terms of the GNU General Public
 * License v2, or at your option any later version.
 */

/*
 * This is a small version of ps for use in the ELKS project.
 * It is not fully functional, and it is not portable.
 */

#define __KERNEL__
#include <linuxmt/ntty.h>
#undef __KERNEL__
#include <linuxmt/mem.h>
#include <linuxmt/sched.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <string.h>

int read_task(int fd, unsigned int off, unsigned int ds,
		struct task_struct *task_table)
{
	off_t addr;

	addr = (off_t) (((off_t)ds << 4) + off);
	if (!(addr == lseek(fd, addr, SEEK_SET))) {
		return -1;
	}
	if (!read(fd, task_table, sizeof (struct task_struct))) {
		return -1;
	}
	return 0;
}

int process_name(int fd, unsigned int off, unsigned int seg)
{
	int i;
	unsigned int strptr;
	off_t addr;
	char dbuf[64];

	addr = (off_t) (((off_t)seg << 4) + (off_t)off);
	if (!(addr = lseek(fd, addr, SEEK_SET))) return -1;
	if ((i = read(fd, &strptr, 2 )) != 2) return -1;
	addr = (off_t) (((off_t)seg << 4) + (off_t)strptr);
	if (!lseek(fd, addr, SEEK_SET)) return -1;
	if ((i = read(fd, dbuf, 64 )) != 64) return -1;
	printf("%s \n",dbuf);
	return 0;
}


char *devname(unsigned int minor)
{
	struct dirent *d;
	dev_t ttydev = MKDEV(4, minor);
	struct stat st;
	static char dev[] = "/dev";
	static char name[16]; /* should be MAXNAMLEN but that's overkill */

	DIR *fp = opendir(dev);
	if (fp == 0)
		return "??";
	strcpy(name, dev);
	strcat(name, "/");

	while ((d = readdir(fp)) != 0) {
		if( strlen(d->d_name) > sizeof(name) - sizeof(dev) - 1)
			continue;
		strcpy(name + sizeof(dev), d->d_name);
		if (!stat(name, &st) && st.st_rdev == ttydev) {
			closedir(fp);
			return name+8;
		}
	}
	closedir(fp);
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

	return devname(tty.minor);
}

int main(int argc, char **argv)
{
	int i, c, fd;
	unsigned int j, ds, off;
	struct task_struct task_table;
	struct passwd * pwent;

	if ((fd = open("/dev/kmem", O_RDONLY)) < 0) {
		perror("ps");
		exit(1);
	}
	if ((i = ioctl(fd, MEM_GETDS, &ds))) {
		perror("ps");
		exit(1);
	}
	if ((i = ioctl(fd, MEM_GETTASK, &off))) {
		perror("ps");
		exit(1);
	}

	printf("  PID   GRP  TTY USER  STAT INODE COMMAND\n");
	for (j = 1; j < MAX_TASKS; j++) {
		if (read_task(fd, off + j*sizeof(struct task_struct), ds, &task_table) == -1) {
			perror("ps");
			exit(1);
		}
		if (task_table.t_kstackm != KSTACK_MAGIC) break;
		if (task_table.t_regs.ss == 0) continue;
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
		pwent = (getpwuid(task_table.uid));
		printf("%5d %5d %4s %-8s %c %5u ",
				task_table.pid, task_table.pgrp, tty_name(fd, (unsigned int)task_table.tty, ds),
				(pwent ? pwent->pw_name : "unknown"),
				c, (unsigned int)task_table.t_inode);
		process_name(fd, task_table.t_begstack + 2, task_table.t_regs.ss);
		fflush(stdout);
	}
	exit(0);
}

