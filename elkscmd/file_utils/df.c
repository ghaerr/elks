/* df - disk free block printout	Author: Andy Tanenbaum
 *
 * 91/04/30 Kees J. Bot (kjb@cs.vu.nl)
 *	Map filename arguments to the devices they live on.
 *	Changed output to show percentages.
 *
 * 92/12/12 Kees J. Bot
 *	Posixized.  (Almost, the normal output is in kilobytes, it should
 *	be 512-byte units.  'df -P' and 'df -kP' are as it should be.)
 *
 * Ported to ELKS by Greg Haerr May 2020
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <linuxmt/minix_fs.h>

#define BLOCK_SIZE	1024

int df(char *device);

typedef unsigned int bit_t;
bit_t bit_count(unsigned blocks, bit_t bits, int fd);
char *devname(char *dirname);

int iflag= 0;	/* Focus on inodes instead of blocks. */
int Pflag= 0;	/* Posix standard output. */
int kflag= 0;	/* Output in kilobytes instead of 512 byte units for -P. */
int istty;	/* isatty(1) */

void usage(void)
{
	fprintf(stderr, "usage: df [-ikP] [device_or_mount_point]\n");
	exit(1);
}

int main(int argc, char *argv[])
{
  char *device = "/";
  char *blockdev;

  while (argc > 1 && argv[1][0] == '-') {
  	char *opt= argv[1]+1;

  	while (*opt != 0) {
  		switch (*opt++) {
  		case 'i':	iflag= 1;	break;
  		case 'k':	kflag= 1;	break;
  		case 'P':	Pflag= 1;	break;
		default:
			usage();
		}
	}
	argc--;
	argv++;
  }

  istty= isatty(1);

  if (argc > 1)
	device = argv[1];

  if (!(blockdev = devname(device))) {
	fprintf(stderr, "Can't find /dev/ device for %s\n", device);
	exit(1);
  }

  if (Pflag) {
	printf(!iflag ? "\
Filesystem    %4d-blocks    Used  Available  Capacity\n" : "\
Filesystem       Inodes     IUsed    IFree    %%IUsed\n",
		kflag ? 1024 : 512);
  } else {
	printf("%s\n", !iflag ? "\
Filesystem    1k-Blocks     free     used    %  FUsed%" : "\
Filesystem        Files     free     used    %  BUsed%"
	);
  }


  return df(blockdev);
}

/* (num / tot) in percentages rounded up. */
#define percent(num, tot)  ((int) ((100L * (num) + ((tot) - 1)) / (tot)))

/* One must be careful printing all these _t types. */
#define L(n)	((long) (n))

int df(char *device)
{
  int fd;
  bit_t i_count, z_count;
  block_t totblocks, busyblocks;
  int n;
  struct minix_super_block super, *sp;

  if ((fd = open(device, O_RDONLY)) < 0) {
	fprintf(stderr, "df: %s: %s\n", device, strerror(errno));
	return(1);
  }
  lseek(fd, (off_t) BLOCK_SIZE, SEEK_SET);	/* skip boot block */

  if (read(fd, (char *) &super, sizeof(super)) != (int) sizeof(super)) {
	fprintf(stderr, "df: Can't read super block of %s\n", device);
	close(fd);
	return(1);
  }

  lseek(fd, (off_t) BLOCK_SIZE * 2L, SEEK_SET);	/* skip rest of super block */
  sp = &super;
  if (sp->s_magic != MINIX_SUPER_MAGIC && sp->s_magic != MINIX_SUPER_MAGIC2) {
	fprintf(stderr, "df: %s: Not a MINIX file system\n", device);
	close(fd);
	return(1);
  }
  if (sp->s_magic == MINIX_SUPER_MAGIC) sp->s_zones= sp->s_nzones;

  i_count = bit_count(sp->s_imap_blocks, (bit_t) (sp->s_ninodes+1), fd);

  if (i_count == -1) {
	fprintf(stderr, "df: Can't find bit maps of %s\n", device);
	close(fd);
	return(1);
  }
  i_count--;	/* There is no inode 0. */

  /* The first bit in the zone map corresponds with zone s_firstdatazone - 1
   * This means that there are s_zones - (s_firstdatazone - 1) bits in the map
   */
  z_count = bit_count(sp->s_zmap_blocks,
	(bit_t) (sp->s_zones - (sp->s_firstdatazone - 1)), fd);

  if (z_count == -1) {
	fprintf(stderr, "df: Can't find bit maps of %s\n", device);
	close(fd);
	return(1);
  }
  /* Don't forget those zones before sp->s_firstdatazone - 1 */
  z_count += sp->s_firstdatazone - 1;

  totblocks = (block_t) sp->s_zones << sp->s_log_zone_size;
  busyblocks = (block_t) z_count << sp->s_log_zone_size;

  /* Print results. */
  printf("%s", device);
  n= strlen(device);
  if (n > 15 && istty) { printf("\n"); n= 0; }
  while (n < 15) { printf(" "); n++; }

  if (!Pflag && !iflag) {
	printf(" %7ld  %7ld  %7ld %3d%%   %3d%%\n",
		L(totblocks),				/* Blocks */
		L(totblocks - busyblocks),		/* free */
		L(busyblocks),				/* used */
		percent(busyblocks, totblocks),		/* % */
		percent(i_count, sp->s_ninodes)	/* FUsed% */
	);
  }
  if (!Pflag && iflag) {
	printf(" %7ld  %7ld  %7ld %3d%%   %3d%%\n",
		L(sp->s_ninodes),			/* Files */
		L(sp->s_ninodes - i_count),		/* free */
		L(i_count),				/* used */
		percent(i_count, sp->s_ninodes),	/* % */
		percent(busyblocks, totblocks)		/* BUsed% */
	);
  }
  if (Pflag && !iflag) {
  	if (!kflag) {
  		/* 512-byte units please. */
  		totblocks *= 2;
  		busyblocks *= 2;
	}
	printf(" %7ld   %7ld  %7d     %4d%%\n",
		L(totblocks),				/* Blocks */
		L(busyblocks),				/* Used */
		totblocks - busyblocks,			/* Available */
		percent(busyblocks, totblocks)		/* Capacity */
	);
  }
  if (Pflag && iflag) {
	printf(" %7ld   %7ld  %7ld     %4d%%\n",
		L(sp->s_ninodes),			/* Inodes */
		L(i_count),				/* IUsed */
		L(sp->s_ninodes - i_count),		/* IAvail */
		percent(i_count, sp->s_ninodes)	/* Capacity */
	);
  }
  close(fd);
  return(0);
}

bit_t bit_count(unsigned blocks, bit_t bits, int fd)
{
  char *wptr;
  int i, b;
  bit_t busy;
  char *wlim;
  static char buf[BLOCK_SIZE];
  static char bits_in_char[1 << CHAR_BIT];

  /* Precalculate bitcount for each char. */
  if (bits_in_char[1] != 1) {
	for (b = (1 << 0); b < (1 << CHAR_BIT); b <<= 1)
		for (i = 0; i < (1 << CHAR_BIT); i++)
			if (i & b) bits_in_char[i]++;
  }

  /* Loop on blocks, reading one at a time and counting bits. */
  busy = 0;
  for (i = 0; i < blocks && bits != 0; i++) {
	if (read(fd, buf, BLOCK_SIZE) != BLOCK_SIZE) return(-1);

	wptr = &buf[0];
	if (bits >= CHAR_BIT * BLOCK_SIZE) {
		wlim = &buf[BLOCK_SIZE];
		bits -= CHAR_BIT * BLOCK_SIZE;
	} else {
		b = bits / CHAR_BIT;	/* whole chars in map */
		wlim = &buf[b];
		bits -= b * CHAR_BIT;	/* bits in last char, if any */
		b = *wlim & ((1 << bits) - 1);	/* bit pattern from last ch */
		busy += bits_in_char[b];
		bits = 0;
	}

	/* Loop on the chars of a block. */
	while (wptr != wlim)
		busy += bits_in_char[*wptr++ & ((1 << CHAR_BIT) - 1)];
  }
  return(busy);
}

/*
 * $PchId: df.c,v 1.7 1998/07/27 18:42:17 philip Exp $
 */

/* return /dev/ device from dirname*/
char *devname(char *dirname)
{
   static char dev[] = "/dev";
   struct stat st, dst;
   DIR  *fp;
   struct dirent *d;
   static char name[16]; /* should be MAXNAMLEN but that's overkill */

   if (!strncmp(dirname, "/dev/", 5))
	return dirname;

   if (stat(dirname, &st) < 0)
      return 0;

   fp = opendir(dev);
   if (fp == 0)
      return 0;
   strcpy(name, dev);
   strcat(name, "/");

   while ((d = readdir(fp)) != 0) {
      if(d->d_name[0] == '.' || strlen(d->d_name) > sizeof(name) - sizeof(dev) - 1)
         continue;
      strcpy(name + sizeof(dev), d->d_name);
      if (stat(name, &dst) == 0) {
		if (st.st_dev == dst.st_rdev) {
			closedir(fp);
			return name;
        }
      }
   }
   closedir(fp);
   return 0;
}
