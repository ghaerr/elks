/*  Copyright 1996 Grant R. Guenther,  based on work of Itai Nahshon
 *   http://www.torque.net/ziptool.html
 *  Copyright 1997-2002,2007-2009 Alain Knaff.
 *  This file is part of mtools.
 *
 *  Mtools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mtools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mtools.  If not, see <http://www.gnu.org/licenses/>.
 *
 * mzip.c
 * Iomega Zip/Jaz drive tool
 * change protection mode and eject disk
 */

/* mzip.c by Markus Gyger <mgyger@itr.ch> */
/* This code is based on ftp://gear.torque.net/pub/ziptool.c */
/* by Grant R. Guenther with the following copyright notice: */

/*  (c) 1996   Grant R. Guenther,  based on work of Itai Nahshon  */
/*  http://www.torque.net/ziptool.html  */


/* Unprotect-till-eject modes and mount tests added
 * by Ilya Ovchinnikov <ilya@socio.msu.su>
 */

#include "sysincludes.h"
#include "mtools.h"
#include "scsi.h"

#ifndef _PASSWORD_LEN
#define _PASSWORD_LEN 33
#endif

#ifdef OS_linux

#if __GLIBC__ >=2
#include <sys/mount.h>
#else
#define _LINUX_KDEV_T_H 1  /* don't redefine MAJOR/MINOR */
#include <linux/fs.h>
#endif

#include "devices.h"

#endif


static int zip_cmd(int priv, int fd, unsigned char cdb[6], int clen, 
		   scsi_io_mode_t mode, void *data, size_t len, 
		   void *extra_data)
{
	int r;

	if(priv)
		reclaim_privs();
	r = scsi_cmd(fd, cdb, clen,  mode, data, len, extra_data);
	if(priv)
		drop_privs();
	return r;
}

static int test_mounted ( char *dev )
{
#ifdef HAVE_MNTENT_H
	struct mntent	*mnt;
	struct MT_STAT	st_dev, st_mnt;
	FILE		*mtab;
/*
 * Now check if any partition of this device is already mounted (this
 * includes checking if the device is mounted under a different name).
 */
	
	if (MT_STAT (dev, &st_dev)) {
		fprintf (stderr, "%s: stat(%s) failed: %s.\n",
			 progname, dev, strerror (errno));
		exit(1);
	}
	
	if (!S_ISBLK (st_dev.st_mode)) /* not a block device, cannot 
					* be mounted */
		return 0;

#ifndef _PATH_MOUNTED
# define _PATH_MOUNTED "/etc/mtab"
#endif

	if ((mtab = setmntent (_PATH_MOUNTED, "r")) == NULL) {
		fprintf (stderr, "%s: can't open %s.\n",
			 progname, _PATH_MOUNTED);
		exit(1);
	}
	
	while ( ( mnt = getmntent (mtab) ) ) {
		if (!mnt->mnt_fsname

#ifdef MNTTYPE_SWAP
		    || !strcmp (mnt->mnt_type, MNTTYPE_SWAP)
#endif
#ifdef MNTTYPE_NFS
		    || !strcmp (mnt->mnt_type, MNTTYPE_NFS)
#endif
		    ||  !strcmp (mnt->mnt_type, "proc")
		    ||  !strcmp (mnt->mnt_type, "smbfs")
#ifdef MNTTYPE_IGNORE
		    ||  !strcmp (mnt->mnt_type, MNTTYPE_IGNORE)
#endif
			)
			continue;

		if (MT_STAT (mnt->mnt_fsname, &st_mnt)) {
			continue;
		}
		
		if (S_ISBLK (st_mnt.st_mode)) {
#ifdef OS_linux
			/* on Linux, warn also if the device is on the same
			 * partition */
			if (MAJOR(st_mnt.st_rdev) == MAJOR(st_dev.st_rdev) &&
			    MINOR(st_mnt.st_rdev) >= MINOR(st_dev.st_rdev) &&
			    MINOR(st_mnt.st_rdev) <= MINOR(st_dev.st_rdev)+15){
				fprintf (stderr, 
					 "Device %s%d is mounted on %s.\n", 
					 dev, 
					 MINOR(st_mnt.st_rdev) - 
					 MINOR(st_dev.st_rdev),
					 mnt->mnt_dir);
#else
				if(st_mnt.st_rdev != st_dev.st_rdev) {
#endif
					endmntent (mtab);
					return 1;
				}
#if 0
			} /* keep Emacs indentation happy */
#endif
		}
	}
	endmntent (mtab);
#endif
	return 0;
}


static void usage(int ret) NORETURN;
static void usage(int ret)
{
	fprintf(stderr, 
		"Mtools version %s, dated %s\n", 
		mversion, mdate);
	fprintf(stderr, 
		"Usage: %s [-V] [-q] [-e] [-u] [-r|-w|-p|-x] [drive:]\n"
		"\t-q print status\n"
		"\t-e eject disk\n"
		"\t-f eject disk even when mounted\n"
		"\t-r write protected (read-only)\n"
		"\t-w not write-protected (read-write)\n"
		"\t-p password write protected\n"
		"\t-x password protected\n"
		"\t-u unprotect till disk ejecting\n", 
		progname);
	exit(ret);
}


enum mode_t {
	ZIP_RW = 0,
	ZIP_RO = 2,
	ZIP_RO_PW = 3,
	ZIP_PW = 5,
	ZIP_UNLOCK_TIL_EJECT = 8
};

static enum mode_t get_zip_status(int priv, int fd, void *extra_data)
{
	unsigned char status[128];
	unsigned char cdb[6] = { 0x06, 0, 0x02, 0, sizeof status, 0 };
	
	if (zip_cmd(priv, fd, cdb, 6, SCSI_IO_READ, 
		    status, sizeof status, extra_data) == -1) {
		perror("status: ");
		exit(1);
	}
	return status[21] & 0xf;
}


static int short_command(int priv, int fd, int cmd1, int cmd2, 
			 int cmd3, const char *data, void *extra_data)
{
	unsigned char cdb[6] = { 0, 0, 0, 0, 0, 0 };

	cdb[0] = cmd1;
	cdb[1] = cmd2;
	cdb[4] = cmd3;

	return zip_cmd(priv, fd, cdb, 6, SCSI_IO_WRITE, 
		       (char *) data, data ? strlen(data) : 0, extra_data);
}


static int iomega_command(int priv, int fd, int mode, const char *data, 
			  void *extra_data)
{
	return short_command(priv, fd, 
			     SCSI_IOMEGA, mode, data ? strlen(data) : 0,
			     data, extra_data);
}

static int door_command(int priv, int fd, int cmd1, int cmd2,
			void *extra_data)
{
	return short_command(priv, fd, cmd1, 0, cmd2, 0, extra_data);
}

void mzip(int argc, char **argv, int type UNUSEDP) NORETURN;
void mzip(int argc, char **argv, int type UNUSEDP)
{
	void *extra_data = NULL;
	int c;
	char drive;
	device_t *dev;
	int fd = -1;
	char name[EXPAND_BUF];
	enum { ZIP_NIX    =      0,
	       ZIP_STATUS = 1 << 0,
	       ZIP_EJECT  = 1 << 1,
	       ZIP_MODE_CHANGE = 1 << 2,
	       ZIP_FORCE  = 1 << 3
	} request = ZIP_NIX;

	enum mode_t newMode = ZIP_RW;
	enum mode_t oldMode = ZIP_RW;

#define setMode(x) \
	if(request & ZIP_MODE_CHANGE) usage(1); \
	request |= ZIP_MODE_CHANGE; \
	newMode = x; \
	break;
	
	/* get command line options */
	if(helpFlag(argc, argv))
		usage(0);
	while ((c = getopt(argc, argv, "i:efpqrwxuh")) != EOF) {
		switch (c) {
			case 'i':
				set_cmd_line_image(optarg);
				break;
			case 'f':
				if (get_real_uid()) {
					fprintf(stderr, 
						"Only root can use force. Sorry.\n");
					exit(1);
				}
				request |= ZIP_FORCE;
				break;
			case 'e': /* eject */
				request |= ZIP_EJECT;
				break;
			case 'q': /* status query */
				request |= ZIP_STATUS;
				break;

			case 'p': /* password read-only */
				setMode(ZIP_RO_PW);
			case 'r': /* read-only */
				setMode(ZIP_RO);
			case 'w': /* read-write */
				setMode(ZIP_RW);
			case 'x': /* password protected */
				setMode(ZIP_PW);
			case 'u': /* password protected */
				setMode(ZIP_UNLOCK_TIL_EJECT)
			case 'h':
				usage(0);
			default:  /* unrecognized */
				usage(1);
			
		}
	}
	
	if (request == ZIP_NIX) request = ZIP_STATUS;  /* default action */

	if (argc - optind > 1 || 
	    (argc - optind == 1 &&
	     (!argv[optind][0] || argv[optind][1] != ':')))
		usage(1);
	
	drive = ch_toupper(argc - optind == 1 ? argv[argc - 1][0] : ':');
	
	for (dev = devices; dev->name; dev++) {
		unsigned char cdb[6] = { 0, 0, 0, 0, 0, 0 };
		struct {
			char    type,
				type_modifier,
				scsi_version,
				data_format,
				length,
				reserved1[2],
				capabilities,
				vendor[8],
				product[16],
				revision[4],
				vendor_specific[20],
				reserved2[40];
		} inq_data;

		if (dev->drive != drive) 
			continue;
		expand(dev->name, name);
		if ((request & (ZIP_MODE_CHANGE | ZIP_EJECT)) &&
		    !(request & ZIP_FORCE) &&
		    test_mounted(name)) {
			fprintf(stderr, 
				"Can\'t change status of/eject mounted device\n");
			exit(1);
		}
		precmd(dev);

		if(IS_PRIVILEGED(dev))
			reclaim_privs();
		fd = scsi_open(name, O_RDONLY
#ifdef O_NDELAY
			       | O_NDELAY
#endif
			       , 0644,
			       &extra_data);
		if(IS_PRIVILEGED(dev))
			drop_privs();

				/* need readonly, else we can't
				 * open the drive on Solaris if
				 * write-protected */		
		if (fd == -1) 
			continue;
		closeExec(fd);

		if (!(request & (ZIP_MODE_CHANGE | ZIP_STATUS)))
			/* if no mode change or ZIP specific status is
			 * involved, the command (eject) is applicable
			 * on all drives */
			break;

		cdb[0] = SCSI_INQUIRY;
		cdb[4] = sizeof inq_data;
		if (zip_cmd(IS_PRIVILEGED(dev), fd, cdb, 6, SCSI_IO_READ, 
			    &inq_data, sizeof inq_data, extra_data) != 0) {
			close(fd);
			continue;
		}
		
#ifdef DEBUG
		fprintf(stderr, "device: %s\n\tvendor: %.8s\n\tproduct: %.16s\n"
			"\trevision: %.4s\n", name, inq_data.vendor,
			inq_data.product, inq_data.revision);
#endif /* DEBUG */

		if (strncasecmp("IOMEGA  ", inq_data.vendor,
				sizeof inq_data.vendor) ||
		    (strncasecmp("ZIP 100         ",
				 inq_data.product, sizeof inq_data.product) &&
		     strncasecmp("ZIP 100 PLUS    ",
				 inq_data.product, sizeof inq_data.product) &&
		     strncasecmp("ZIP 250         ",
				 inq_data.product, sizeof inq_data.product) &&
		     strncasecmp("ZIP 750         ",
				 inq_data.product, sizeof inq_data.product) &&
		     strncasecmp("JAZ 1GB         ",
				 inq_data.product, sizeof inq_data.product) &&
		     strncasecmp("JAZ 2GB         ",
				 inq_data.product, sizeof inq_data.product))) {

			/* debugging */
			fprintf(stderr,"Skipping drive with vendor='");
			fwrite(inq_data.vendor,1, sizeof(inq_data.vendor), 
			       stderr);
			fprintf(stderr,"' product='");
			fwrite(inq_data.product,1, sizeof(inq_data.product), 
			       stderr);
			fprintf(stderr,"'\n");
			/* end debugging */
			close(fd);
			continue;
		}
		break;  /* found Zip/Jaz drive */
	}

	if (dev->drive == 0) {
		fprintf(stderr, "%s: drive '%c:' is not a Zip or Jaz drive\n",
			argv[0], drive);
		exit(1);
	}

	if (request & (ZIP_MODE_CHANGE | ZIP_STATUS))
		oldMode = get_zip_status(IS_PRIVILEGED(dev), fd, extra_data);

	if (request & ZIP_MODE_CHANGE) {
				/* request temp unlock, and disk is already unlocked */
		if(newMode == ZIP_UNLOCK_TIL_EJECT &&
		   (oldMode & ZIP_UNLOCK_TIL_EJECT))
			request &= ~ZIP_MODE_CHANGE;

				/* no password change requested, and disk is already
				 * in the requested state */
		if(!(newMode & 0x01) && newMode == oldMode)
			request &= ~ZIP_MODE_CHANGE;
	}

	if (request & ZIP_MODE_CHANGE) {
		int ret;
		enum mode_t unlockMode, unlockMask;
		const char *passwd;
		char dummy[1];

		if(newMode == ZIP_UNLOCK_TIL_EJECT) {
			unlockMode = newMode | oldMode;
			unlockMask = 9;
		} else {
			unlockMode = newMode & ~0x5;
			unlockMask = 1;
		}

		if ((oldMode & unlockMask) == 1) {  /* unlock first */
			char *s;
			passwd = "APlaceForYourStuff";
			if ((s = strchr(passwd, '\n'))) *s = '\0';  /* chomp */
			iomega_command(IS_PRIVILEGED(dev), fd, unlockMode, 
				       passwd, extra_data);
		}
		
		if ((get_zip_status(IS_PRIVILEGED(dev), fd, extra_data) & 
		     unlockMask) == 1) {
			/* unlock first */
			char *s;
			passwd = getpass("Password: ");
			if ((s = strchr(passwd, '\n'))) *s = '\0';  /* chomp */
			if((ret=iomega_command(IS_PRIVILEGED(dev), fd, 
					       unlockMode, passwd, 
					       extra_data))){
				if (ret == -1) perror("passwd: ");
				else fprintf(stderr, "wrong password\n");
				exit(1);
			}
			if((get_zip_status(IS_PRIVILEGED(dev), 
					   fd, extra_data) & 
			    unlockMask) == 1) {
				fprintf(stderr, "wrong password\n");
				exit(1);
			}
		}
		
		if (newMode & 0x1) {
			char first_try[_PASSWORD_LEN+1];
			
			passwd = getpass("Enter new password:");
			strncpy(first_try, passwd,_PASSWORD_LEN);
			passwd = getpass("Re-type new password:");
			if(strncmp(first_try, passwd, _PASSWORD_LEN)) {
				fprintf(stderr,
					"You misspelled it. Password not set.\n");
				exit(1);
			}
		} else {
			passwd = dummy;
			dummy[0] = '\0';
		}

		if(newMode == ZIP_UNLOCK_TIL_EJECT)
			newMode |= oldMode;

		if((ret=iomega_command(IS_PRIVILEGED(dev), fd, 
				       newMode, passwd, extra_data))){
			if (ret == -1) perror("set passwd: ");
			else fprintf(stderr, "password not changed\n");
			exit(1);
		}
#ifdef OS_linux
		ioctl(fd, BLKRRPART); /* revalidate the disk, so that the
					 kernel notices that its writable
					 status has changed */
#endif
	}
	
	if (request & ZIP_STATUS) {
		const char *unlocked;

		if(oldMode & 8)
			unlocked = " and unlocked until eject";
		else
			unlocked = "";		
		switch (oldMode & ~8) {
			case ZIP_RW:  
				printf("Drive '%c:' is not write-protected\n",
				       drive);
				break;
			case ZIP_RO:
				printf("Drive '%c:' is write-protected%s\n",
				       drive, unlocked);
				break;
			case ZIP_RO_PW: 
				printf("Drive '%c:' is password write-protected%s\n", 
				       drive, unlocked);
				break;
			case ZIP_PW:  
				printf("Drive '%c:' is password protected%s\n", 
				       drive, unlocked);
				break;
			default: 
				printf("Unknown protection mode %d of drive '%c:'\n",
				       oldMode, drive);
				break;				
		}		
	}
	
	if (request & ZIP_EJECT) {
		if(request & ZIP_FORCE)
			if(door_command(IS_PRIVILEGED(dev), fd, 
					SCSI_ALLOW_MEDIUM_REMOVAL, 0,
					extra_data) < 0) {
				perror("door unlock: ");
				exit(1);
			}

		if(door_command(IS_PRIVILEGED(dev), fd, 
				SCSI_START_STOP, 1,
				extra_data) < 0) {
			perror("stop motor: ");
			exit(1);
		}

		if(door_command(IS_PRIVILEGED(dev), fd, 
				SCSI_START_STOP, 2, extra_data) < 0) {
			perror("eject: ");
			exit(1);
		}
		if(door_command(IS_PRIVILEGED(dev), fd, 
				SCSI_START_STOP, 2, extra_data) < 0) {
			perror("second eject: ");
			exit(1);
		}
	}
	
	close(fd);
	exit(0);
}
