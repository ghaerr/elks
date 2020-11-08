/*
 * makeboot - make a bootable image
 * Part of /bin/sys script for creating ELKS images from ELKS
 *
 * Usage: makeboot /dev/{fd0,fd1,hda1,hda2,etc}
 *
 * Copies boot sector from root device to target device
 * Sets EPB and BPB parameters in boot sector
 * Copies /linux and /bootopts for MINIX
 * Copies /linux and creates /dev for FAT
 *
 * Nov 2020 Greg Haerr
 */

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <utime.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <linuxmt/hdreg.h>
#include <linuxmt/minix_fs.h>

#define BUF_SIZE	1024 

#define MOUNTDIR	"/tmp/mnt"

#define SYSFILE1	"/linux"		/* copied for MINIX and FAT*/
#define SYSFILE2	"/bootopts"		/* copied for MINIX only */
#define DEVDIR		"/dev"			/* created for FAT only */

/* See bootblocks/minix.map for the offsets, these used for MINIX and FAT */
#define ELKS_BPB_NumTracks	0x1F7		/* offset of number of tracks (word)*/
#define ELKS_BPB_SecPerTrk	0x1F9		/* offset of sectors per track (byte)*/
#define ELKS_BPB_NumHeads	0x1FA		/* offset of number of heads (byte)*/

/* MINIX-only offsets, FIXME - SectOffset not yet standard*/
#define MINIX_SectOffset	0x1B5		/* offset of partition start sector FIXME */

/* FAT BPB start and end offsets*/
#define FATBPB_START	11				/* start at bytes per sector*/
#define FATBPB_END		61				/* through end of file system type*/

/* FAT-only offsets */
#define FAT_BPB_SecPerTrk	0x18		/* offset of sectors per track (word) */
#define FAT_BPB_NumHeads	0x1A		/* offset of number of heads (word) */
#define FAT_BPB_SectOffset	0x1C		/* offset of partition start sector (long) */

unsigned int SecPerTrk, NumHeads, NumTracks;
unsigned long Start_sector;

int bootsecsize;
char bootblock[1024];				/* 1024 for MINIX, 512 for FAT */

/* return name of root device*/
char *devname(dev_t dev)
{
	DIR *dp;
	struct dirent *d;
	struct stat st;
	static char devdir[] = "/dev";
	static char name[16];

	dp = opendir(devdir);
	if (dp == 0) {
		perror("/dev");
		return NULL;
	}
	strcpy(name, devdir);
	strcat(name, "/");

	while ((d = readdir(dp)) != NULL) {
		if (d->d_name[0] == '.')
 			continue;
		strcpy(name + sizeof(devdir), d->d_name);
		if (stat(name, &st) == 0) {
			if (S_ISBLK(st.st_mode) && st.st_rdev == dev) {
				closedir(dp);
				return name;
			}
		}
	}
	closedir(dp);
	fprintf(stderr, "Can't find root device\n");
	return NULL;
}

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
	printf("magic %x\n", sb->s_magic);
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
	printf("Boot drive geometry CHS %d/%d/%d offset %ld\n",
		NumTracks, NumHeads, SecPerTrk, Start_sector);

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
	static char buf[BUF_SIZE];

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
		times.actime = statbuf1.st_atime;
		times.modtime = statbuf1.st_mtime;
		utime(destname, &times);
	}

	return 1;	// success

error_exit:
	close(rfd);
	close(wfd);

	return 0;	// fail
}

int main(int ac, char **av)
{
	char *rootdevice, *targetdevice;
	int rootfstype, fstype, fd, n;
	struct stat sbuf;

	if (ac != 2) {
		fprintf(stderr, "Usage: makeboot /dev/{fd0,fd1,hda1,hda2,etc}\n");
		return -1;
	}
	targetdevice = av[1];

	if (stat("/", &sbuf) < 0) {
		perror("/");
		return -1;
	}
	rootdevice = devname(sbuf.st_dev);
	printf("root device %s\n", rootdevice);

	fd = open(rootdevice, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Can't open root device %s\n", rootdevice);
		return -1;
	}
	if (!get_geometry(fd)) {
		fprintf(stderr, "Can't get geometry for root device %s\n", rootdevice);
		close(fd);
		return -1;
	}
	rootfstype = get_fstype(fd);
	if (rootfstype == 0) {
		fprintf(stderr, "Unknown root device filesystem format\n");
		close(fd);
		return -1;
	}
	printf("root type %d\n", rootfstype);

	bootsecsize = (rootfstype == FST_MINIX)? 1024: 512;
	lseek(fd, 0L, SEEK_SET);
	n = read(fd, bootblock, bootsecsize);
	if (n != bootsecsize) {
		fprintf(stderr, "Can't read root boot sector\n");
		close(fd);
		return -1;
	}
	close(fd);

	fd = open(targetdevice, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Can't open target device %s\n", targetdevice);
		return -1;
	}
	if (!get_geometry(fd)) {	/* gets ELKS CHS parameters for bootblock write*/
		fprintf(stderr, "Can't get geometry for target device %s\n", targetdevice);
		close(fd);
		return -1;
	}
	setEPBparms(bootblock);		/* sets ELKS CHS parameters in bootblock*/
	fstype = get_fstype(fd);
	if (fstype == 0) {
		fprintf(stderr, "Unknown target device filesystem format\n");
		close(fd);
		return -1;
	}
	if (fstype == FST_MINIX) setMINIXparms(bootblock);
	if (fstype == FST_MSDOS) {
		if (!setFATparms(fd, bootblock)) {
			close(fd);
			return -1;
		}
	}
	printf("target type %d\n", fstype);

	if (rootfstype != fstype) {
		fprintf(stderr, "Root and new system device must be same filesystem format\n");
		close(fd);
		return -1;
	}

	lseek(fd, 0L, SEEK_SET);
	n = write(fd, bootblock, bootsecsize);
	if (n != bootsecsize) {
		fprintf(stderr, "Can't write target boot sector\n");
		close(fd);
		return -1;
	}
	close(fd);
//sync();
//return 0;

	if (mkdir(MOUNTDIR, 0777) < 0) {
		fprintf(stderr, "Can't create temp mount point %s\n", MOUNTDIR);
		return -1;
	}
	if (mount(targetdevice, MOUNTDIR, fstype, 0) < 0) {
		fprintf(stderr, "Can't mount %s on %s\n", targetdevice, MOUNTDIR);
		rmdir(MOUNTDIR);
		return -1;
	}
	if (!copyfile(SYSFILE1, MOUNTDIR SYSFILE1, 1)) {
		fprintf(stderr, "Error copying %s\n", SYSFILE1);
		goto errout;
	}

	if (fstype == FST_MSDOS) {
		if (mkdir(MOUNTDIR DEVDIR, 0777) < 0)
			fprintf(stderr, "/dev directory may already exist on target\n");
	} else {
		if (!copyfile(SYSFILE2, MOUNTDIR SYSFILE2, 1))
			fprintf(stderr, "Not copying %s file\n", SYSFILE2);
	}

	if (umount(targetdevice) < 0)
		fprintf(stderr, "Unmount error\n");
	rmdir(MOUNTDIR);

	return fstype;		/* return filesystem type (1=MINIX, 2=FAT */

errout:
	umount(targetdevice);
	rmdir(MOUNTDIR);
	return 0;
}
