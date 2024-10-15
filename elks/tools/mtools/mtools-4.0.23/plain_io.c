/*  Copyright 1995-2007,2009,2011 Alain Knaff.
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
 * Io to a plain file or device
 *
 * written by:
 *
 * Alain L. Knaff			
 * alain@knaff.lu
 *
 */

#include "sysincludes.h"
#include "stream.h"
#include "mtools.h"
#include "msdos.h"
#include "plain_io.h"
#include "scsi.h"
#include "partition.h"
#include "llong.h"

#ifdef HAVE_LINUX_FS_H
# include <linux/fs.h>
#endif

typedef struct SimpleFile_t {
    Class_t *Class;
    int refs;
    Stream_t *Next;
    Stream_t *Buffer;
    struct MT_STAT statbuf;
    int fd;
    mt_off_t offset;
    mt_off_t lastwhere;
    int seekable;
    int privileged;
#ifdef OS_hpux
    int size_limited;
#endif
    int scsi_sector_size;
    void *extra_data; /* extra system dependent information for scsi */
    int swap; /* do the word swapping */
} SimpleFile_t;


#include "lockdev.h"

typedef int (*iofn) (int, char *, int);


static void swap_buffer(char *buf, size_t len)
{
	unsigned int i;
	for (i=0; i<len; i+=2) {
		char temp = buf[i];
		buf[i] = buf[i+1];
		buf[i+1] = temp;
	}
}


static int file_io(Stream_t *Stream, char *buf, mt_off_t where, int len,
				   iofn io)
{
	DeclareThis(SimpleFile_t);
	int ret;

	where += This->offset;

	if (This->seekable && where != This->lastwhere ){
		if(mt_lseek( This->fd, where, SEEK_SET) < 0 ){
			perror("seek");
			This->lastwhere = (mt_off_t) -1;
			return -1;
		}
	}

#ifdef OS_hpux
	/*
	 * On HP/UX, we can not write more than MAX_LEN bytes in one go.
	 * If more are written, the write fails with EINVAL
	 */
	#define MAX_SCSI_LEN (127*1024)
	if(This->size_limited && len > MAX_SCSI_LEN)
		len = MAX_SCSI_LEN;
#endif
	ret = io(This->fd, buf, len);

#ifdef OS_hpux
	if (ret == -1 && 
		errno == EINVAL && /* if we got EINVAL */
		len > MAX_SCSI_LEN) {
		This->size_limited = 1;
		len = MAX_SCSI_LEN;
		ret = io(This->fd, buf, len);
	}
#endif

	if ( ret == -1 ){
		perror("plain_io");
		This->lastwhere = (mt_off_t) -1;
		return -1;
	}
	This->lastwhere = where + ret;
	return ret;
}
	


static int file_read(Stream_t *Stream, char *buf, mt_off_t where, size_t len)
{
	DeclareThis(SimpleFile_t);

	int result = file_io(Stream, buf, where, len, (iofn) read);

	if ( This->swap )
		swap_buffer( buf, len );
	return result;
}

static int file_write(Stream_t *Stream, char *buf, mt_off_t where, size_t len)
{
	DeclareThis(SimpleFile_t);

	if ( !This->swap )
		return file_io(Stream, buf, where, len, (iofn) write);
	else {
		int result;
		char *swapping = malloc( len );
		memcpy( swapping, buf, len );
		swap_buffer( swapping, len );

		result = file_io(Stream, swapping, where, len, (iofn) write);

		free(swapping);
		return result;
	}
}

static int file_flush(Stream_t *Stream UNUSEDP)
{
#if 0
	DeclareThis(SimpleFile_t);

	return fsync(This->fd);
#endif
	return 0;
}

static int file_free(Stream_t *Stream)
{
	DeclareThis(SimpleFile_t);

	if (This->fd > 2)
		return close(This->fd);
	else
		return 0;
}

static int file_geom(Stream_t *Stream, struct device *dev, 
		     struct device *orig_dev,
		     int media, union bootsector *boot)
{
	int ret;
	DeclareThis(SimpleFile_t);
	size_t tot_sectors;
	int BootP, Infp0, InfpX, InfTm;
	int sectors, j;
	unsigned char sum;
	int sect_per_track;
	struct label_blk_t *labelBlock;

	dev->ssize = 2; /* allow for init_geom to change it */
	dev->use_2m = 0x80; /* disable 2m mode to begin */

	if(media == 0xf0 || media >= 0x100){		
		dev->heads = WORD(nheads);
		dev->sectors = WORD(nsect);
		tot_sectors = DWORD(bigsect);
		SET_INT(tot_sectors, WORD(psect));
		sect_per_track = dev->heads * dev->sectors;
		if(sect_per_track == 0) {
		    if(mtools_skip_check) {
			/* add some fake values if sect_per_track is
			 * zero. Indeed, some atari disks lack the
			 * geometry values (i.e. have zeroes in their
			 * place). In order to avoid division by zero
			 * errors later on, plug 1 everywhere
			 */
			dev->heads = 1;
			dev->sectors = 1;
			sect_per_track = 1;
		    } else {
			fprintf(stderr, "The devil is in the details: zero number of heads or sectors\n");
			exit(1);
		    }
		}
		tot_sectors += sect_per_track - 1; /* round size up */
		dev->tracks = tot_sectors / sect_per_track;

		BootP = WORD(ext.old.BootP);
		Infp0 = WORD(ext.old.Infp0);
		InfpX = WORD(ext.old.InfpX);
		InfTm = WORD(ext.old.InfTm);
		
		if(WORD(fatlen)) {
			labelBlock = &boot->boot.ext.old.labelBlock;
		} else {
			labelBlock = &boot->boot.ext.fat32.labelBlock;
		}

		if (boot->boot.descr >= 0xf0 &&
		    has_BPB4 &&
		    strncmp( boot->boot.banner,"2M", 2 ) == 0 &&
		    BootP < 512 && Infp0 < 512 && InfpX < 512 && InfTm < 512 &&
		    BootP >= InfTm + 2 && InfTm >= InfpX && InfpX >= Infp0 && 
		    Infp0 >= 76 ){
			for (sum=0, j=63; j < BootP; j++) 
				sum += boot->bytes[j];/* checksum */
			dev->ssize = boot->bytes[InfTm];
			if (!sum && dev->ssize <= 7){
				dev->use_2m = 0xff;
				dev->ssize |= 0x80; /* is set */
			}
		}
	} else
		if(setDeviceFromOldDos(media, dev) < 0)
			exit(1);

	sectors = dev->sectors;
	dev->sectors = dev->sectors * WORD(secsiz) / 512;

#ifdef JPD
	printf("file_geom:media=%0X=>cyl=%d,heads=%d,sects=%d,ssize=%d,use2m=%X\n",
	       media, dev->tracks, dev->heads, dev->sectors, dev->ssize,
	       dev->use_2m);
#endif
	ret = init_geom(This->fd,dev, orig_dev, &This->statbuf);
	dev->sectors = sectors;
#ifdef JPD
	printf("f_geom: after init_geom(), sects=%d\n", dev->sectors);
#endif
	return ret;
}


static int file_data(Stream_t *Stream, time_t *date, mt_size_t *size,
		     int *type, int *address)
{
	DeclareThis(SimpleFile_t);

	if(date)
		*date = This->statbuf.st_mtime;
	if(size)
		*size = This->statbuf.st_size;
	if(type)
		*type = S_ISDIR(This->statbuf.st_mode);
	if(address)
		*address = 0;
	return 0;
}

static int file_discard(Stream_t *Stream)
{
#ifdef BLKFLSBUF
	int ret;
	DeclareThis(SimpleFile_t);
	ret= ioctl(This->fd, BLKFLSBUF);
	if(ret < 0)
		perror("BLKFLSBUF");
	return ret;
#else
	return 0;
#endif
}

/* ZIP or other scsi device on Solaris or SunOS system.
   Since Sun won't accept a non-Sun label on a scsi disk, we must
   bypass Sun's disk interface and use low-level SCSI commands to read
   or write the ZIP drive.  We thus replace the file_read and file_write
   routines with our own scsi_read and scsi_write routines, that use the
   uscsi ioctl interface.  By James Dugal, jpd@usl.edu, 11-96.  Tested
   under Solaris 2.5 and SunOS 4.3.1_u1 using GCC.

   Note: the mtools.conf entry for a ZIP drive would look like this:
(solaris) drive C: file="/dev/rdsk/c0t5d0s2" partition=4  FAT=16 nodelay  exclusive scsi=&
(sunos) drive C: file="/dev/rsd5c" partition=4  FAT=16 nodelay  exclusive scsi=1

   Note 2: Sol 2.5 wants mtools to be suid-root, to use the ioctl.  SunOS is
   happy if we just have access to the device, so making mtools sgid to a
   group called, say, "ziprw" which has rw permission on /dev/rsd5c, is fine.
 */

static void scsi_init(SimpleFile_t *This)
{
   int fd = This->fd;
   unsigned char cdb[10],buf[8];

   memset(cdb, 0, sizeof cdb);
   memset(buf,0, sizeof(buf));
   cdb[0]=SCSI_READ_CAPACITY;
   if (scsi_cmd(fd, (unsigned char *)cdb, 
		sizeof(cdb), SCSI_IO_READ, buf, sizeof(buf), This->extra_data)==0)
   {
       This->scsi_sector_size=
	       ((unsigned)buf[5]<<16)|((unsigned)buf[6]<<8)|(unsigned)buf[7];
       if (This->scsi_sector_size != 512)
	   fprintf(stderr,"  (scsi_sector_size=%d)\n",This->scsi_sector_size);
   }
}

static int scsi_io(Stream_t *Stream, char *buf,
		   mt_off_t where, size_t len, int rwcmd)
{
	unsigned int firstblock, nsect;
	int clen,r;
	size_t max;
	off_t offset;
	unsigned char cdb[10];
	DeclareThis(SimpleFile_t);

	firstblock=truncBytes32((where + This->offset)/This->scsi_sector_size);
	/* 512,1024,2048,... bytes/sector supported */
	offset=truncBytes32(where + This->offset - 
						firstblock*This->scsi_sector_size);
	nsect=(offset+len+This->scsi_sector_size-1)/ This->scsi_sector_size;
#if defined(OS_sun) && defined(OS_i386)
	if (This->scsi_sector_size>512)
		firstblock*=This->scsi_sector_size/512; /* work around a uscsi bug */
#endif /* sun && i386 */

	if (len>512) {
		/* avoid buffer overruns. The transfer MUST be smaller or
		* equal to the requested size! */
		while (nsect*This->scsi_sector_size>len)
			--nsect;
		if(!nsect) {			
			fprintf(stderr,"Scsi buffer too small\n");
			exit(1);
		}
		if(rwcmd == SCSI_IO_WRITE && offset) {
			/* there seems to be no memmove before a write */
			fprintf(stderr,"Unaligned write\n");
			exit(1);
		}
		/* a better implementation should use bounce buffers.
		 * However, in normal operation no buffer overruns or
		 * unaligned writes should happen anyways, as the logical
		 * sector size is (hopefully!) equal to the physical one
		 */
	}


	max = scsi_max_length();
	
	if (nsect > max)
		nsect=max;
	
	/* set up SCSI READ/WRITE command */
	memset(cdb, 0, sizeof cdb);

	switch(rwcmd) {
		case SCSI_IO_READ:
			cdb[0] = SCSI_READ;
			break;
		case SCSI_IO_WRITE:
			cdb[0] = SCSI_WRITE;
			break;
	}

	cdb[1] = 0;

	if (firstblock > 0x1fffff || nsect > 0xff) {
		/* I suspect that the ZIP drive also understands Group 1
		 * commands. If that is indeed true, we may chose Group 1
		 * more aggressively in the future */

		cdb[0] |= SCSI_GROUP1;
		clen=10; /* SCSI Group 1 cmd */

		/* this is one of the rare case where explicit coding is
		 * more portable than macros... The meaning of scsi command
		 * bytes is standardised, whereas the preprocessor macros
		 * handling it might be not... */

		cdb[2] = (unsigned char) (firstblock >> 24) & 0xff;
		cdb[3] = (unsigned char) (firstblock >> 16) & 0xff;
		cdb[4] = (unsigned char) (firstblock >> 8) & 0xff;
		cdb[5] = (unsigned char) firstblock & 0xff;
		cdb[6] = 0;
		cdb[7] = (unsigned char) (nsect >> 8) & 0xff;
		cdb[8] = (unsigned char) nsect & 0xff;
		cdb[9] = 0;
	} else {
		clen = 6; /* SCSI Group 0 cmd */
		cdb[1] |= (unsigned char) ((firstblock >> 16) & 0x1f);
		cdb[2] = (unsigned char) ((firstblock >> 8) & 0xff);
		cdb[3] = (unsigned char) firstblock & 0xff;
		cdb[4] = (unsigned char) nsect;
		cdb[5] = 0;
	}
	
	if(This->privileged)
		reclaim_privs();

	r=scsi_cmd(This->fd, (unsigned char *)cdb, clen, rwcmd, buf,
		   nsect*This->scsi_sector_size, This->extra_data);

	if(This->privileged)
		drop_privs();

	if(r) {
		perror(rwcmd == SCSI_IO_READ ? "SCMD_READ" : "SCMD_WRITE");
		return -1;
	}
#ifdef JPD
	printf("finished %u for %u\n", firstblock, nsect);
#endif

#ifdef JPD
	printf("zip: read or write OK\n");
#endif
	if (offset>0) memmove(buf,buf+offset,nsect*This->scsi_sector_size-offset);
	if (len==256) return 256;
	else if (len==512) return 512;
	else return nsect*This->scsi_sector_size-offset;
}

static int scsi_read(Stream_t *Stream, char *buf, mt_off_t where, size_t len)
{
	
#ifdef JPD
	printf("zip: to read %d bytes at %d\n", len, where);
#endif
	return scsi_io(Stream, buf, where, len, SCSI_IO_READ);
}

static int scsi_write(Stream_t *Stream, char *buf, mt_off_t where, size_t len)
{
#ifdef JPD
	Printf("zip: to write %d bytes at %d\n", len, where);
#endif
	return scsi_io(Stream, buf, where, len, SCSI_IO_WRITE);
}

static Class_t ScsiClass = {
	scsi_read, 
	scsi_write,
	file_flush,
	file_free,
	file_geom,
	file_data,
	0, /* pre-allocate */
	0, /* dos-convert */
	file_discard
};


static Class_t SimpleFileClass = {
	file_read, 
	file_write,
	file_flush,
	file_free,
	file_geom,
	file_data,
	0, /* pre_allocate */
	0, /* dos-convert */
	file_discard
};


Stream_t *SimpleFileOpen(struct device *dev, struct device *orig_dev,
			 const char *name, int mode, char *errmsg, 
			 int mode2, int locked, mt_size_t *maxSize)
{
	SimpleFile_t *This;
#ifdef __EMX__
HFILE FileHandle;
ULONG Action;
APIRET rc;
#endif
	This = New(SimpleFile_t);
	if (!This){
		printOom();
		return 0;
	}
	memset((void*)This, 0, sizeof(SimpleFile_t));
	This->scsi_sector_size = 512;
	This->seekable = 1;
#ifdef OS_hpux
	This->size_limited = 0;
#endif
	This->Class = &SimpleFileClass;
	if (!name || strcmp(name,"-") == 0 ){
		if (mode == O_RDONLY)
			This->fd = 0;
		else
			This->fd = 1;
		This->seekable = 0;
		This->refs = 1;
		This->Next = 0;
		This->Buffer = 0;
		if (MT_FSTAT(This->fd, &This->statbuf) < 0) {
		    Free(This);
		    if(errmsg)
#ifdef HAVE_SNPRINTF
			snprintf(errmsg,199,"Can't stat -: %s", 
				strerror(errno));   
#else
			sprintf(errmsg,"Can't stat -: %s", 
				strerror(errno));
#endif
		    return NULL;
		}

		return (Stream_t *) This;
	}

	
	if(dev) {
		if(!(mode2 & NO_PRIV))
			This->privileged = IS_PRIVILEGED(dev);
		mode |= dev->mode;
	}

	precmd(dev);
	if(IS_PRIVILEGED(dev) && !(mode2 & NO_PRIV))
		reclaim_privs();

#ifdef __EMX__
#define DOSOPEN_FLAGS	(OPEN_FLAGS_DASD | OPEN_FLAGS_WRITE_THROUGH | \
			OPEN_FLAGS_NOINHERIT | OPEN_FLAGS_RANDOM | \
			OPEN_FLAGS_NO_CACHE)
#define DOSOPEN_FD_ACCESS (OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE)
#define DOSOPEN_HD_ACCESS (OPEN_SHARE_DENYNONE | OPEN_ACCESS_READONLY)

	if (isalpha(*name) && (*(name+1) == ':')) {
		rc = DosOpen(
			name, &FileHandle, &Action, 0L, FILE_NORMAL,
			OPEN_ACTION_OPEN_IF_EXISTS, DOSOPEN_FLAGS |
			(IS_NOLOCK(dev)?DOSOPEN_HD_ACCESS:DOSOPEN_FD_ACCESS),
			0L);
#if DEBUG
		if (rc != NO_ERROR) fprintf (stderr, "DosOpen() returned %d\n", rc);
#endif
		if (!IS_NOLOCK(dev)) {
			rc = DosDevIOCtl(
			FileHandle, 0x08L, DSK_LOCKDRIVE, 0, 0, 0, 0, 0, 0);
#if DEBUG
			if (rc != NO_ERROR) fprintf (stderr, "DosDevIOCtl() returned %d\n", rc);
#endif
		}
		if (rc == NO_ERROR)
			This->fd = _imphandle(FileHandle); else This->fd = -1;
	} else
#endif
	    {
		if (IS_SCSI(dev))
		    This->fd = scsi_open(name, mode, IS_NOLOCK(dev)?0444:0666,
					 &This->extra_data);
		else
		    This->fd = open(name, mode | O_LARGEFILE | O_BINARY, 
				    IS_NOLOCK(dev)?0444:0666);
	    }

	if(IS_PRIVILEGED(dev) && !(mode2 & NO_PRIV))
		drop_privs();
		
	if (This->fd < 0) {
		Free(This);
		if(errmsg)
#ifdef HAVE_SNPRINTF
			snprintf(errmsg, 199, "Can't open %s: %s",
				name, strerror(errno));
#else
			sprintf(errmsg, "Can't open %s: %s",
				name, strerror(errno));
#endif
		return NULL;
	}

	if(IS_PRIVILEGED(dev) && !(mode2 & NO_PRIV))
		closeExec(This->fd);

#ifdef __EMX__
	if (*(name+1) != ':')
#endif
	if (MT_FSTAT(This->fd, &This->statbuf) < 0
#ifdef OS_mingw32msvc
	    && strncmp(name, "\\\\.\\", 4) != 0
#endif
	   ) {
		Free(This);
		if(errmsg) {
#ifdef HAVE_SNPRINTF
			snprintf(errmsg,199,"Can't stat %s: %s", 
				name, strerror(errno));
#else
			if(strlen(name) > 50) {
			    sprintf(errmsg,"Can't stat file: %s", 
				    strerror(errno));
			} else {
			    sprintf(errmsg,"Can't stat %s: %s", 
				name, strerror(errno));
			}
#endif
		}
		return NULL;
	}
#ifndef __EMX__
#ifndef __CYGWIN__
#ifndef OS_mingw32msvc
	/* lock the device on writes */
	if (locked && lock_dev(This->fd, mode == O_RDWR, dev)) {
		if(errmsg)
#ifdef HAVE_SNPRINTF
			snprintf(errmsg,199,
				"plain floppy: device \"%s\" busy (%s):",
				dev ? dev->name : "unknown", strerror(errno));
#else
			sprintf(errmsg,
				"plain floppy: device \"%s\" busy (%s):",
				(dev && strlen(dev->name) < 50) ? 
				 dev->name : "unknown", strerror(errno));
#endif

		close(This->fd);
		Free(This);
		return NULL;
	}
#endif
#endif
#endif
	/* set default parameters, if needed */
	if (dev){		
		if ((!IS_MFORMAT_ONLY(dev) && dev->tracks) &&
			init_geom(This->fd, dev, orig_dev, &This->statbuf)){
			close(This->fd);
			Free(This);
			if(errmsg)
				sprintf(errmsg,"init: set default params");
			return NULL;
		}
		This->offset = (mt_off_t) dev->offset;
	} else
		This->offset = 0;

	This->refs = 1;
	This->Next = 0;
	This->Buffer = 0;

	if(maxSize) {
		if (IS_SCSI(dev)) {
			*maxSize = MAX_OFF_T_B(31+log_2(This->scsi_sector_size));
		} else {
			*maxSize = max_off_t_seek;
		}
		if(This->offset > (mt_off_t) *maxSize) {
			close(This->fd);
			Free(This);
			if(errmsg)
				sprintf(errmsg,"init: Big disks not supported");
			return NULL;
		}
		
		*maxSize -= This->offset;
	}
	/* partitioned drive */

	/* jpd@usl.edu: assume a partitioned drive on these 2 systems is a ZIP*/
	/* or similar drive that must be accessed by low-level scsi commands */
	/* AK: introduce new "scsi=1" statement to specifically set
	 * this option. Indeed, there could conceivably be partitioned
	 * devices where low level scsi commands will not be needed */
	if(IS_SCSI(dev)) {
		This->Class = &ScsiClass;
		if(This->privileged)
			reclaim_privs();
		scsi_init(This);
		if(This->privileged)
			drop_privs();
	}

	This->swap = DO_SWAP( dev );

	if(!(mode2 & NO_OFFSET) &&
	   dev && (dev->partition > 4))
	    fprintf(stderr, 
		    "Invalid partition %d (must be between 0 and 4), ignoring it\n", 
		    dev->partition);

	while(!(mode2 & NO_OFFSET) &&
	      dev && dev->partition && dev->partition <= 4) {
		int has_activated;
		unsigned int last_end, j;
		unsigned char buf[2048];
		struct partition *partTable=(struct partition *)(buf+ 0x1ae);
		size_t partOff;
		
		/* read the first sector, or part of it */
		if (force_read((Stream_t *)This, (char*) buf, 0, 512) != 512)
			break;
		if( _WORD(buf+510) != 0xaa55)
			break;

		partOff = BEGIN(partTable[dev->partition]);
		if (maxSize) {
			if (partOff > *maxSize >> 9) {
				close(This->fd);
				Free(This);
				if(errmsg)
					sprintf(errmsg,"init: Big disks not supported");
				return NULL;
			}
			*maxSize -= (mt_off_t) partOff << 9;
		}
			
		This->offset += (mt_off_t) partOff << 9;
		if(!partTable[dev->partition].sys_ind) {
			if(errmsg)
				sprintf(errmsg,
					"init: non-existant partition");
			close(This->fd);
			Free(This);
			return NULL;
		}

		if(!dev->tracks) {
			dev->heads = head(partTable[dev->partition].end)+1;
			dev->sectors = sector(partTable[dev->partition].end);
			dev->tracks = cyl(partTable[dev->partition].end) -
				cyl(partTable[dev->partition].start)+1;
		}
		dev->hidden=
			dev->sectors*head(partTable[dev->partition].start) +
			sector(partTable[dev->partition].start)-1;
		if(!mtools_skip_check &&
		   consistencyCheck((struct partition *)(buf+0x1ae), 0, 0,
				    &has_activated, &last_end, &j, dev, 0)) {
			fprintf(stderr,
				"Warning: inconsistent partition table\n");
			fprintf(stderr,
				"Possibly unpartitioned device\n");
			fprintf(stderr,
				"\n*** Maybe try without partition=%d in "
				"device definition ***\n\n",
				dev->partition);
			fprintf(stderr,
                                "If this is a PCMCIA card, or a disk "
				"partitioned on another computer, this "
				"message may be in error: add "
				"mtools_skip_check=1 to your .mtoolsrc "
				"file to suppress this warning\n");

		}
		break;
		/* NOTREACHED */
	}

	This->lastwhere = -This->offset;
	/* provoke a seek on those devices that don't start on a partition
	 * boundary */

	return (Stream_t *) This;
}

int get_fd(Stream_t *Stream)
{
	Class_t *clazz;
	DeclareThis(SimpleFile_t);
	clazz = This->Class;
	if(clazz != &ScsiClass &&
	   clazz != &SimpleFileClass)
	  return -1;
	else
	  return This->fd;
}

void *get_extra_data(Stream_t *Stream)
{
	DeclareThis(SimpleFile_t);
	
	return This->extra_data;
}
