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
#include <sys/mount.h>
#include <linuxmt/minix_fs.h>
#include <linuxmt/fs.h>

#define BLOCK_SIZE	1024

int df(char *device, char *mpnt);

typedef unsigned int bit_t;
bit_t bit_count(unsigned blocks, bit_t bits, int fd);
struct dnames {
	char *name;
	char *mpoint;
};
struct dnames *devname(char *);
static char *dev_name(dev_t);

int iflag= 0;	/* Focus on inodes instead of blocks. */
int Pflag= 0;	/* Posix standard output. */
int kflag= 0;	/* Output in kilobytes instead of 512 byte units for -P. */
int istty;	/* isatty(1) */

/* (num / tot) in percentages rounded up. */
#define percent(num, tot)  ((int) ((100L * (num) + ((tot) - 1)) / (tot)))

/* One must be careful printing all these _t types. */
#define L(n)	((long) (n))

void usage(void)
{
	fprintf(stderr, "usage: df [-ikP] [device_or_mount_point]\n");
	exit(1);
}

int main(int argc, char *argv[])
{
  char *device = "/";
  char *blockdev;
  struct dnames *dname;
  struct statfs statfs;

  while (argc > 1 && argv[1][0] == '-') {
  	char *opt= argv[1]+1;

  	while (*opt != 0) {
  		switch (*opt++) {
  		case 'i':	iflag= 1;	break;
  		case 'k':	kflag= 1;	break;
  		case 'P':	Pflag= 1;	break;
		default:	usage();
		}
	}
	argc--;
	argv++;
  }

  istty= isatty(1);

  if (argc > 1)
	device = argv[1];

  dname = devname(device);
  if (!(blockdev = dname->name)) {
	fprintf(stderr, "Can't find /dev/ device for %s\n", device);
	exit(1);
  }

  if (iflag) {
	printf("Filesystem       Inodes     IUsed    IFree    %%IUsed   Mounted on\n");
  } else {
	if (!Pflag) 
		printf("Filesystem    1k-Blocks     free     used    %%  FUsed%%  Mounted on\n");
	else 
		printf("Filesystem    %4d-blocks    Used  Available  Capacity   Mounted on\n",
			kflag ? 1024 : 512);
  }

  if (argc == 1) {	/* loop through all mounted devices */
	int i;
  	for (i = 0; i < NR_SUPER; i++) {
		if (ustatfs(i, &statfs, 0) >= 0) {
			char *nm = dev_name(statfs.f_dev);
			if (statfs.f_type > FST_MSDOS || (statfs.f_type == FST_MSDOS && (Pflag||iflag)))
				printf("%s     -- Not a MINIX filesystem\n", nm);
			else if (statfs.f_type == FST_MSDOS && !Pflag)
				printf("%-15s %7ld  %7ld  %7ld %3d%%          %s (FAT)\n", nm,\
				    statfs.f_blocks, statfs.f_bfree,\
				    statfs.f_blocks-statfs.f_bfree,\
				    percent(statfs.f_blocks-statfs.f_bfree, statfs.f_blocks),\
				    statfs.f_mntonname);
			else
				df(nm, statfs.f_mntonname);
		}
	}
	return 0;
  } else
  	return df(blockdev, dname->mpoint);
}

int df(char *device, char *mpnt)
{
  int fd;
  bit_t i_count, z_count;
  long totblocks, busyblocks;
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
  n = strlen(device);
  if (n > 15 && istty) { printf("\n"); n= 0; }
  while (n < 15) { printf(" "); n++; }

  if (iflag) {
	printf(" %7ld   %7ld  %7ld     %4d%%   %s\n",
		L(sp->s_ninodes),			/* Inodes */
		L(i_count),				/* IUsed */
		L(sp->s_ninodes - i_count),		/* IAvail */
		percent(i_count, sp->s_ninodes),	/* Capacity */
		mpnt					/* Mount pnt */

	);
  }
  if (!Pflag && !iflag) {
	printf(" %7ld  %7ld  %7ld %3d%%   %3d%%   %s\n",
		L(totblocks),				/* Blocks */
		L(totblocks - busyblocks),		/* free */
		L(busyblocks),				/* used */
		percent(busyblocks, totblocks),		/* % */
		percent(i_count, sp->s_ninodes),	/* FUsed% */
		mpnt					/* Mount point */
	);
  }
  if (Pflag && !iflag) {
  	if (!kflag) {
  		/* 512-byte units please. */
  		totblocks *= 2;
  		busyblocks *= 2;
	}
	printf(" %7ld   %7ld  %7ld     %4d%%     %s\n",
		L(totblocks),				/* Blocks */
		L(busyblocks),				/* Used */
		totblocks - busyblocks,			/* Available */
		percent(busyblocks, totblocks),		/* Capacity */
		mpnt					/* Mount point */
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

/* return /dev/ device from dirname */
struct dnames *devname(char *dirname)
{
   static char dev[] = "/dev";
   struct stat st, dst;
   DIR  *fp;
   struct dirent *d;
   static struct statfs statfs;
   static struct dnames dn;
   static char name[16]; /* should be MAXNAMLEN but that's overkill */

   if (!strncmp(dirname, "/dev/", 5)) {
	dn.name = dirname;
	dn.mpoint = dirname;
	return &dn;
   }

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
	     if (ustatfs(st.st_dev, &statfs, 0) < 0) {
		dn.mpoint = NULL;
	     } else {
		dn.mpoint = statfs.f_mntonname;
	     }
	     closedir(fp);
	     dn.name = name;
	     return &dn;
        }
      }
   }
   closedir(fp);
   return NULL;
}

/*
 * Convert a block device number to name.
 * From mount.c
 */
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

static char *dev_name(dev_t dev)
{
	int i;
#define NAMEOFF 5
	static char name[10] = "/dev/";

	for (i=0; i<sizeof(devices)/sizeof(devices[0])-1; i++) {
		if (devices[i].num == (dev & 0xfff0)) {
			strcpy(&name[NAMEOFF], devices[i].name);
			if (i < 4) {
				if (dev & 0x07) {
					name[NAMEOFF+3] = '0' + (dev & 7);
					name[NAMEOFF+4] = 0;
				}
			}
			return name;
		}
	}
	return NULL;
}

