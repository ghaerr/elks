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

#include <linuxmt/mem.h>
#include <linuxmt/sched.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <pwd.h>

int read_task(fd, off, ds, task_table)
int fd;
unsigned int off;
unsigned int ds;
struct task_struct * task_table;
{
	unsigned int i;
	off_t addr;

	addr = (off_t) (((off_t)ds << 4) + off);
	if (!(addr == lseek(fd, addr, SEEK_SET))) {
		return -1;
	}
	if (!read(fd, task_table, sizeof (struct task_struct))) {
		return -1;
	}
}

int get_name(fd, seg, off)
int fd;
unsigned int seg;
unsigned int off;
{
	int i, j;
	unsigned int strptr;
	off_t addr;
	int * d;
	char dbuf[64];

	addr = (off_t) (((off_t)seg << 4) + (off_t)off);
	if (!(addr = lseek(fd, addr, SEEK_SET))) {
		return -1;
	}
	if ((i = read(fd, &strptr, 2 )) != 2) {
		return -1;
	}
	addr = (off_t) (((off_t)seg << 4) + (off_t)strptr);
	if (!lseek(fd, addr, SEEK_SET)) {
		return -1;
	}
	if ((i = read(fd, dbuf, 64 )) != 64) {
		return -1;
	}
	printf("%s \n",dbuf);
}


int main(argc, argv)
int argc;
char ** argv;
{
	int i, fd;
	unsigned int j, ds, off;
	unsigned long addr;
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

	for (j = off, i = 0; i < 20; j += sizeof(struct task_struct), i++) {
		if (read_task(fd, j, ds, &task_table) == -1) {
			perror("ps");
			exit(1);
		}
		if (task_table.t_kstackm != KSTACK_MAGIC) {
			break;
		}
		if (task_table.t_regs.ss == 0) {
			continue;
		}
		if (task_table.state != TASK_UNUSED) {
			pwent = (getpwuid(task_table.uid));
			printf("%5d %5d %-8s %c %5u ", 
				task_table.pid,
				task_table.pgrp,
				(pwent ? pwent->pw_name : "unknown"),
				((task_table.state == TASK_RUNNING) ? 'R' : 'S'),
				task_table.t_inode);
			if(task_table.mm.flags & DS_SWAP)
				printf("[data swapped] \n");
			else
				get_name(fd, task_table.t_regs.ss, task_table.t_begstack + 2);
			fflush(stdout);
		}
	}
	exit(0);
}

