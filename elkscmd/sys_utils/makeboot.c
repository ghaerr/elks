/*
 * makeboot - make a bootable image
 * Part of /bin/sys script for creating ELKS images from ELKS
 *
 * Usage: makeboot [-M][-F] /dev/{fd0,fd1,hda1,hda2,etc}
 *
 * Copies boot sector from root device to target device
 * Sets EPB and BPB parameters in boot sector
 * Copies /linux and /bootopts for MINIX
 * Copies /linux and creates /dev for FAT
 * If -M: write MBR using compiled-in mbr.bin
 * If -F: allow writing to flat (non-MBR) hard drive
 * If -f file: take bootblock from file
 * If -s: Skip file copying, useful for bootblock copy only
 * If -m: MBR only, skip file copy and bootblock
 *
 * Nov 2020 Greg Haerr
 */

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <utime.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <arch/hdreg.h>
#include <linuxmt/minix_fs.h>
#include <linuxmt/kdev_t.h>
#include "../../bootblocks/mbr_autogen.c"
#include "../../elks/arch/i86/drivers/block/bioshd.h"

#define BUF_SIZE	1024 

#define MOUNTDIR	"/tmp/mnt"

#define SYSFILE1	"/linux"		/* copied for MINIX and FAT*/
#define SYSFILE2	"/bootopts"		/* copied for MINIX and FAT */
#define DEVDIR		"/dev"			/* created for FAT only */

/* BIOS driver numbers must match elks/arch/i86/drivers/block/bioshd.h */
#define BIOS_NUM_MINOR	NUM_MINOR       /* max minor devices per drive */
#define BIOS_MINOR_MASK	(BIOS_NUM_MINOR - 1)
#define BIOS_FD0_MINOR	(DRIVE_FD0 * (1<<MINOR_SHIFT)) /* minor # of first floppy */

/* See bootblocks/minix.map for the offsets, these used for MINIX and FAT */
#define ELKS_BPB_NumTracks	0x1F7		/* offset of number of tracks (word)*/
#define ELKS_BPB_SecPerTrk	0x1F9		/* offset of sectors per track (byte)*/
#define ELKS_BPB_NumHeads	0x1FA		/* offset of number of heads (byte)*/

/* MINIX-only offsets*/
#define MINIX_SectOffset	0x1F3		/* offset of partition start sector (long)*/

/* FAT BPB start and end offsets*/
#define FATBPB_START	11				/* start at bytes per sector*/
#define FATBPB_END		61				/* through end of file system type*/

/* FAT-only offsets */
#define FAT_BPB_RootEntCnt	0x11		/* offset of FAT12/16 root directory entry count (word) */
#define FAT_BPB_FATSz16		0x16		/* offset of FAT12/16 allocation table sector count (word) */
#define FAT_BPB_SecPerTrk	0x18		/* offset of sectors per track (word) */
#define FAT_BPB_NumHeads	0x1A		/* offset of number of heads (word) */
#define FAT_BPB_SectOffset	0x1C		/* offset of partition start sector (long) */
#define FAT_BPB_RootClus	0x2C		/* offset of FAT32 root directory start cluster (long) */

#define PARTITION_START		0x01be		/* offset of partition table in MBR*/
#define PARTITION_END		0x01fd		/* end of partition 4 in MBR*/

unsigned int SecPerTrk, NumHeads, NumTracks;
unsigned long Start_sector;

int bootsecsize;
int rootfstype;
char bootblock[1024];				/* 1024 for MINIX, 512 for FAT */
char *rootdevice;
char *fsname[3] = { "Unknown", "Minix", "FAT" };

/* determine and return filesystem type*/
int get_fstype(int fd)
{
	struct minix_super_block *sb;
	char superblock[512];

	lseek(fd, 1024L, SEEK_SET);
	if (read(fd, superblock, 512) != 512) {
		fprintf(stderr, "Can't read superblock\n");
		return 0;
	}
	sb = (struct minix_super_block *)superblock;
	if (sb->s_magic == MINIX_SUPER_MAGIC)
		return FST_MINIX;
	return FST_MSDOS;		/* guess FAT if not MINIX*/
}

/* get device geometry*/
int get_geometry(int fd)
{
	struct hd_geometry geom;

	if (ioctl(fd, HDIO_GETGEO, &geom) < 0) {
		fprintf(stderr, "Can't get boot drive geometry\n");
		return 0;
	}

	NumTracks = geom.cylinders;
	NumHeads = geom.heads;
	SecPerTrk = geom.sectors;
	Start_sector = geom.start;

	return 1;	// success
}

/* set ELKS EPB track, head, sector and start_offset values in buffer*/
void setEPBparms(char *buf)
{
	buf[ELKS_BPB_SecPerTrk] = (unsigned char)SecPerTrk;
	buf[ELKS_BPB_NumHeads] = (unsigned char)NumHeads;
	buf[ELKS_BPB_NumTracks] = (unsigned char)NumTracks;
	buf[ELKS_BPB_NumTracks+1] = (unsigned char)(NumTracks >> 8);
}

/* set MINIX SectorOffset in buffer*/
void setMINIXparms(char *buf)
{
	*(unsigned long *)&buf[MINIX_SectOffset] = Start_sector;
}

/* set FAT BPB values from target in buffer*/
int setFATparms(int fd, char *buf)
{
	int n;
	char BPB[512];

	lseek(fd, 0L, SEEK_SET);
	n = read(fd, BPB, 512);
	if (n != 512) {
		fprintf(stderr, "Can't read target boot sector\n");
		return 0;
	}

	/* copy FAT BPB*/
	for (n = FATBPB_START; n <= FATBPB_END; n++)
		buf[n] = BPB[n];

	printf("BPB sect_offset %ld setting to %ld\n",
		*(unsigned long *)&buf[FAT_BPB_SectOffset], Start_sector);
#if 1
	*(unsigned short *)&buf[FAT_BPB_SecPerTrk] = SecPerTrk;
	*(unsigned short *)&buf[FAT_BPB_NumHeads] = NumHeads;
	*(unsigned long *)&buf[FAT_BPB_SectOffset] = Start_sector;
#endif
	return 1;
}

/* write MBR code outside of partition table*/
int setMBRparms(int fd)
{
	int n;
	char MBR[512];
	unsigned short *bsect = (unsigned short *)MBR;

	lseek(fd, 0L, SEEK_SET);
	n = read(fd, MBR, 512);
	if (n != 512) {
		fprintf(stderr, "Can't read target MBR\n");
		return 0;
	}

	/* copy MBR code*/
	for (n = 0; n < 512; n++)
		if (n < PARTITION_START || n > PARTITION_END)
			MBR[n] = mbr_bin[n];
	if (bsect[255] != 0xaa55)
		fprintf(stderr, "Warning: No boot signature in MBR\n");
	lseek(fd, 0L, SEEK_SET);
	n = write(fd, MBR, 512);
	if (n != 512) {
		fprintf(stderr, "Can't write target MBR\n");
		return 0;
	}
	return 1;
}

int copyfile(char *srcname, char *destname, int setmodes)
{
	int		rfd;
	int		wfd;
	int		rcc;
	int		wcc;
	char		*bp;
	struct	stat	statbuf1;
	struct	stat	statbuf2;
	struct	utimbuf	times;
	static	char	buf[BUF_SIZE];

	printf("Copying %s to %s\n", srcname, destname);
	if (stat(srcname, &statbuf1) < 0) {
		perror(srcname);
		return 0;
	}

	if (stat(destname, &statbuf2) < 0) {
		statbuf2.st_ino = -1;
		statbuf2.st_dev = -1;
	}

	rfd = open(srcname, 0);
	if (rfd < 0) {
		perror(srcname);
		return 0;
	}

	wfd = creat(destname, statbuf1.st_mode);
	if (wfd < 0) {
		perror(destname);
		close(rfd);
		return 0;
	}

	while ((rcc = read(rfd, buf, BUF_SIZE)) > 0) {
		bp = buf;
		while (rcc > 0) {
			wcc = write(wfd, bp, rcc);
			if (wcc < 0) {
				perror(destname);
				goto error_exit;
			}
			bp += wcc;
			rcc -= wcc;
		}
	}

	if (rcc < 0) {
		perror(srcname);
		goto error_exit;
	}

	close(rfd);
	if (close(wfd) < 0) {
		perror(destname);
		return 0;
	}

	if (setmodes) {
		chmod(destname, statbuf1.st_mode);
		chown(destname, statbuf1.st_uid, statbuf1.st_gid);
		times.actime =	statbuf1.st_atime;
		times.modtime = statbuf1.st_mtime;
		utime(destname, &times);
	}

	return 1;	// success

error_exit:
	close(rfd);
	close(wfd);

	return 0;	// fail
}

void fatalmsg(const char *s, ...)
{
	va_list p;

	va_start(p, s);
	fprintf(stderr, "Error: ");
	vfprintf(stderr, s, p);
	va_end(p);
	exit(-1);
}

/*
 * Read bootblock from current partition or from file.
 */
void get_bootblock(char *bootf) 
{
	int fd, n;
	unsigned short *bsect = (unsigned short *)bootblock;
	
	if (bootf == NULL) {	/* get bootblock from current root */
		fd = open(rootdevice, O_RDONLY);
		if (fd < 0)
			fatalmsg("Can't open boot device %s\n", rootdevice);

		if (!get_geometry(fd))
			fatalmsg("Can't get geometry for boot device %s\n", rootdevice);
		rootfstype = get_fstype(fd);
		if (rootfstype == 0)
			fatalmsg("Unknown boot device filesystem\n");
		printf("System on %s: %s (CHS %d/%d/%d at offset %ld)\n",
			rootdevice, fsname[rootfstype], NumTracks, NumHeads, SecPerTrk, Start_sector);
		bootsecsize = (rootfstype == FST_MINIX)? 1024: 512;

	} else {	/* get bootblock from file */

		fd = open(bootf, O_RDONLY);
		if (fd < 0)
			fatalmsg("Can't open boot file %s\n", bootf);
		bootsecsize = 1024;
	}
	lseek(fd, 0L, SEEK_SET);
	n = read(fd, bootblock, bootsecsize);
	if (n == 512) { rootfstype = FST_MSDOS; }
	else if (n == 1024) { rootfstype = FST_MINIX; }
	else { fatalmsg("Bootblock size error %d\n", n); }

	if (bsect[255] != 0xaa55)
		fatalmsg("Bad signature in bootblock\n");
	close(fd);
}

int main(int ac, char **av)
{
	char *targetdevice;
	char *bootfile = 0;
	int fstype = -1, fd, n;
	int opt_writembr = 0;
	int opt_writeflat = 0;
	int opt_syscopy = 0;
	int opt_writebb = 0;
	int fat32 = 0;
	dev_t rootdev, targetdev;
	struct stat sbuf;

	if (ac < 2 || ac > 5) {
usage:
		fatalmsg("Usage: makeboot [-M][-F] [-f file] [-s] /dev/{fd0,fd1,hda1,hda2,etc}\n");
	}
	if (ac == 2) opt_writebb++;
	while (av[1] && av[1][0] == '-') {
		if (av[1][1] == 'M') {
			opt_writembr = 1;
			av++;
			ac--;
		}
		else if (av[1][1] == 'F') {
			opt_writeflat = 1;
			av++;
			ac--;
		}
		else if (av[1][1] == 's') {
			opt_syscopy = 1;
			opt_writebb = 1;
			av++;
			ac--;
		}
		else if (av[1][1] == 'f') {
			av++;
			ac--;
			opt_writebb = 1;
			bootfile = av[1];
			av++;
			ac--;
		} else goto usage;
	}
	if (ac != 2)
		goto usage;
	targetdevice = av[1];

	if (stat("/", &sbuf) < 0) {
		perror("/");
		return -1;
	}

	rootdev = sbuf.st_dev;
	rootdevice = devname(rootdev, S_IFBLK);
    if (!rootdevice)
	    fatalmsg("Can't find device: 0x%x\n", rootdev);

	if (opt_writebb == 1) {
		get_bootblock(bootfile);
		fstype = rootfstype;
	}

	fd = open(targetdevice, O_RDWR);
	if (fd < 0)
		fatalmsg("Can't open target device %s\n", targetdevice);

	/* check target is not boot device, raw device, or -M used with floppy*/
	if (fstat(fd, &sbuf) < 0)
		fatalmsg("Can't stat %s\n", targetdevice);
	targetdev = sbuf.st_rdev;

	if (rootdev == targetdev)	/* FIXME: Should be able to MBR rootdev */
		fatalmsg("Can't specify current boot device as target\n");

	if (MINOR(targetdev) < BIOS_FD0_MINOR) {	/* hard drive*/
		if (!opt_writeflat && !opt_writembr) {
			if ((targetdev & BIOS_MINOR_MASK) == 0)	/* non-partitioned device*/
				fatalmsg("Must specify partitioned device (example /dev/hda1)\n");
		}
	} else {					/* floppy*/
		if (opt_writembr)
			fatalmsg("Can't use -M on floppy devices\n");
	}

	/* Write MBR */
	if (opt_writembr) {
		int ffd;

		char *rawtargetdevice = devname(targetdev & ~BIOS_MINOR_MASK, S_IFBLK);
		if (!rawtargetdevice)
			fatalmsg("Can't find raw target device\n");
		ffd = open(rawtargetdevice, O_RDWR);
		if (ffd < 0)
			fatalmsg("Can't open raw target device %s\n", targetdevice);
		if (setMBRparms(ffd))
			printf("Writing MBR on %s\n", rawtargetdevice);
		close(ffd);
	}

	if (opt_writeflat || opt_writebb) {
		if (!get_geometry(fd))		/* gets ELKS CHS parameters for bootblock write*/
			fatalmsg("Can't get geometry for target device %s\n", targetdevice);
		setEPBparms(bootblock);		/* sets ELKS CHS parameters in bootblock*/
		fstype = get_fstype(fd);
		printf("Target on %s: %s (CHS %d/%d/%d at offset %ld)\n",
			targetdevice, fsname[fstype], NumTracks, NumHeads, SecPerTrk, Start_sector);

		if (rootfstype != fstype)
			fatalmsg("Target and System filesystem must be same type\n");

		if (fstype == FST_MINIX) setMINIXparms(bootblock);
		if (fstype == FST_MSDOS) {
			if (!setFATparms(fd, bootblock)) {
				close(fd);
				return -1;
			}
			fat32 = (*(unsigned short *)&bootblock[FAT_BPB_FATSz16] == 0);
			if (fat32) {	/* if FAT32, check if root directory starts on cluster 2 */
				unsigned long rootclus;
				rootclus = *(unsigned long *)&bootblock[FAT_BPB_RootClus];
				if (rootclus != 2)
				    fatalmsg("Unsupported: weird root directory start cluster (%lu)\n",
						rootclus);
			}
		}

		lseek(fd, 0L, SEEK_SET);
		n = write(fd, bootblock, bootsecsize);
		if (n != bootsecsize)
			fatalmsg("Can't write target boot sector\n");
		close(fd);
		printf("Bootblock written\n");
	}

	if (opt_syscopy == 1) {
		if (mkdir(MOUNTDIR, 0777) < 0)
			fprintf(stderr, "Temp mount point %s not created (may already exist)\n", MOUNTDIR);

		if (mount(targetdevice, MOUNTDIR, fstype, 0) < 0) {
			fprintf(stderr, "Error: Can't mount %s on %s\n", targetdevice, MOUNTDIR);
			goto errout2;
		}
		if (!copyfile(SYSFILE1, MOUNTDIR SYSFILE1, 1)) {
			fprintf(stderr, "Error copying %s\n", SYSFILE1);
			goto errout;
		}
		if (!copyfile(SYSFILE2, MOUNTDIR SYSFILE2, 1))
			fprintf(stderr, "Not copying %s file\n", SYSFILE2);

		if (fstype == FST_MSDOS) {
			if (mkdir(MOUNTDIR DEVDIR, 0777) < 0)
				fprintf(stderr, "/dev directory may already exist on target\n");
		}
	
		if (umount(targetdevice) < 0)
			fprintf(stderr, "Unmount error\n");
		rmdir(MOUNTDIR);
		printf("System copied\n");
	}

	sync();

	return fstype;		/* return filesystem type (1=MINIX, 2=FAT) */

errout:
	umount(targetdevice);
errout2:
	rmdir(MOUNTDIR);
	sync();
	return -1;
}
