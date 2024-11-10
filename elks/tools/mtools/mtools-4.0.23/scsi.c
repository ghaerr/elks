/*  Copyright 1996   Grant R. Guenther,  based on work of Itai Nahshon
 *   http://www.torque.net/ziptool.html
 *  Copyright 1997-1999,2001,2002,2005,2007,2009 Alain Knaff.
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
 * scsi.c
 * Iomega Zip/Jaz drive tool
 * change protection mode and eject disk
 */

/* scis.c by Markus Gyger <mgyger@itr.ch> */
/* This code is based on ftp://gear.torque.net/pub/ziptool.c */
/* by Grant R. Guenther with the following copyright notice: */

/*  (c) 1996   Grant R. Guenther,  based on work of Itai Nahshon  */
/*  http://www.torque.net/ziptool.html  */


/* A.K. Moved this from mzip.c to a separate file in order to share with
 * plain_io.c */

#include "sysincludes.h"
#include "mtools.h"
#include "scsi.h"

#if defined OS_hpux
#include <sys/scsi.h>
#endif

#ifdef OS_solaris
#include <sys/scsi/scsi.h>
#endif /* solaris */

#ifdef OS_sunos
#include <scsi/generic/commands.h>
#include <scsi/impl/uscsi.h>
#endif /* sunos */

#ifdef sgi
#include <sys/dsreq.h>
#endif

#ifdef OS_linux
#include <scsi/scsi.h>
#include <scsi/sg.h>
#endif

#ifdef _SCO_DS
#include <sys/scsicmd.h>
#endif

#if (defined(OS_freebsd)) && (__FreeBSD__ >= 2)
#include <camlib.h>
#endif

#if defined(OS_netbsd) || defined(OS_netbsdelf)
#include <sys/scsiio.h>
#endif

int scsi_max_length(void)
{
#ifdef OS_linux
	return 8;
#else
	return 255;
#endif
}

int scsi_open(const char *name, int flag UNUSEDP, int mode UNUSEDP,
	      void **extra_data UNUSEDP)
{
#if (defined(OS_freebsd)) && (__FreeBSD__ >= 2)
    struct cam_device *cam_dev;
    cam_dev = cam_open_device(name, O_RDWR);
    *extra_data = (void *) cam_dev;
    if (cam_dev)
        return cam_dev->fd;
    else
        return -1;
#else
    return open(name, O_RDONLY | O_LARGEFILE | O_BINARY
#ifdef O_NDELAY
		| O_NDELAY
#endif
	/* O_RDONLY  | dev->mode*/);
#endif
}

int scsi_cmd(int fd, unsigned char *cdb, int cmdlen, scsi_io_mode_t mode,
	     void *data, size_t len, void *extra_data UNUSEDP)
{
#if defined OS_hpux
	struct sctl_io sctl_io;
	
	memset(&sctl_io, 0, sizeof sctl_io);   /* clear reserved fields */
	memcpy(sctl_io.cdb, cdb, cmdlen);      /* copy command */
	sctl_io.cdb_length = cmdlen;           /* command length */
	sctl_io.max_msecs = 2000;              /* allow 2 seconds for cmd */

	switch (mode) {
		case SCSI_IO_READ:
			sctl_io.flags = SCTL_READ;
			sctl_io.data_length = len;
			sctl_io.data = data;
			break;
		case SCSI_IO_WRITE: 
			sctl_io.flags = 0;
			sctl_io.data_length = data ? len : 0;
			sctl_io.data = len ? data : 0;
			break;
	}

	if (ioctl(fd, SIOC_IO, &sctl_io) == -1) {
		perror("scsi_io");
		return -1;
	}

	return sctl_io.cdb_status;
	
#elif defined OS_sunos || defined OS_solaris
	struct uscsi_cmd uscsi_cmd;
	memset(&uscsi_cmd, 0, sizeof uscsi_cmd);
	uscsi_cmd.uscsi_cdb = (char *)cdb;
	uscsi_cmd.uscsi_cdblen = cmdlen;
#ifdef OS_solaris
	uscsi_cmd.uscsi_timeout = 20;  /* msec? */
#endif /* solaris */
	
	uscsi_cmd.uscsi_buflen = (u_int)len;
	uscsi_cmd.uscsi_bufaddr = data;

	switch (mode) {
		case SCSI_IO_READ:
			uscsi_cmd.uscsi_flags = USCSI_READ;
			break;
		case SCSI_IO_WRITE:
			uscsi_cmd.uscsi_flags = USCSI_WRITE;
			break;
	}

	if (ioctl(fd, USCSICMD, &uscsi_cmd) == -1) {
		perror("scsi_io");
		return -1;
	}

	if(uscsi_cmd.uscsi_status) {
		errno = 0;
		fprintf(stderr,"scsi status=%x\n",  
			(unsigned short)uscsi_cmd.uscsi_status);
		return -1;
	}
	
	return 0;
	
#elif defined OS_linux
	struct sg_io_hdr my_scsi_cmd;

	/*
	** Init the command
	*/
	memset(&my_scsi_cmd,0,sizeof(my_scsi_cmd));
	my_scsi_cmd.interface_id    = 'S';
	my_scsi_cmd.dxfer_direction = (mode == SCSI_IO_READ)?(SG_DXFER_FROM_DEV):(SG_DXFER_TO_DEV);
	my_scsi_cmd.cmd_len         = cmdlen;
	my_scsi_cmd.mx_sb_len       = 0;
	my_scsi_cmd.dxfer_len       = len;
	my_scsi_cmd.dxferp          = data;
	my_scsi_cmd.cmdp            = cdb;
	my_scsi_cmd.timeout         = ~0; /* where is MAX_UINT defined??? */

#ifdef DEBUG
	printf("CMD(%d): %02x%02x%02x%02x%02x%02x %sdevice\n",cmdlen,cdb[0],cdb[1],cdb[2],cdb[3],cdb[4],cdb[5],
		(mode==SCSI_IO_READ)?("<-"):("->"));
	printf("DATA   : len = %d\n",len);
#endif

	if (ioctl(fd, SG_IO,&my_scsi_cmd) < 0) {
		perror("scsi_io");
		return -1;
	}
	
	return my_scsi_cmd.status & STATUS_MASK;

#elif (defined _SCO_DS) && (defined SCSIUSERCMD)
	struct scsicmd my_scsi_cmd;

	memset(my_scsi_cmd.cdb, 0, SCSICMDLEN);	/* ensure zero pad */
	memcpy(my_scsi_cmd.cdb, cdb, cmdlen);
	my_scsi_cmd.cdb_len = cmdlen;
	my_scsi_cmd.data_len = len;
	my_scsi_cmd.data_ptr = data;
	my_scsi_cmd.is_write = mode == SCSI_IO_WRITE;
	if (ioctl(fd,SCSIUSERCMD,&my_scsi_cmd) == -1) {
		perror("scsi_io: SCSIUSERCMD");
		return -1;
	}
	if (my_scsi_cmd.host_sts != 0 || my_scsi_cmd.target_sts != 0) {
		fprintf(stderr, "scsi_io: scsi status: host=%x; target=%x\n",
		(unsigned)my_scsi_cmd.host_sts,(unsigned)my_scsi_cmd.target_sts);
		return -1;
	}
	return 0;
#elif defined sgi
 	struct dsreq my_scsi_cmd;

	my_scsi_cmd.ds_cmdbuf = (char *)cdb;
	my_scsi_cmd.ds_cmdlen = cmdlen;
	my_scsi_cmd.ds_databuf = data;
	my_scsi_cmd.ds_datalen = len;
       	switch (mode) {
	case SCSI_IO_READ:
	  my_scsi_cmd.ds_flags = DSRQ_READ|DSRQ_SENSE;
	  break;
	case SCSI_IO_WRITE:
	  my_scsi_cmd.ds_flags = DSRQ_WRITE|DSRQ_SENSE;
	  break;
        } 
	my_scsi_cmd.ds_time = 10000;
	my_scsi_cmd.ds_link = 0;
	my_scsi_cmd.ds_synch =0;
	my_scsi_cmd.ds_ret =0;
	if (ioctl(fd, DS_ENTER, &my_scsi_cmd) == -1) {
                perror("scsi_io");
                return -1;
        }

        if(my_scsi_cmd.ds_status) {
                errno = 0;
                fprintf(stderr,"scsi status=%x\n",  
                        (unsigned short)my_scsi_cmd.ds_status);
                return -1;
        }
        
        return 0;
#elif (defined OS_freebsd) && (__FreeBSD__ >= 2)
#define MSG_SIMPLE_Q_TAG 0x20 /* O/O */
      union ccb *ccb;
      int flags;
      int r;
      struct cam_device *cam_dev = (struct cam_device *) extra_data;


      if (cam_dev==NULL || cam_dev->fd!=fd)
      {
                fprintf(stderr,"invalid file descriptor\n");
              return -1;
      }
      ccb = cam_getccb(cam_dev);

      bcopy(cdb, ccb->csio.cdb_io.cdb_bytes, cmdlen);

      if (mode == SCSI_IO_READ)
              flags = CAM_DIR_IN;
      else if (data && len)
              flags = CAM_DIR_OUT;
      else
              flags = CAM_DIR_NONE;
      cam_fill_csio(&ccb->csio,
                    /* retry */ 1,
                    /* cbfcnp */ NULL,
                    flags,
                    /* tag_action */ MSG_SIMPLE_Q_TAG,
                    /*data_ptr*/ len ? data : 0,
                    /*data_len */ data ? len : 0,
                    96,
                    cmdlen,
                    5000);
                    
      if (cam_send_ccb(cam_dev, ccb) < 0 ||
	  (ccb->ccb_h.status & CAM_STATUS_MASK) != CAM_REQ_CMP) {
	  return -1;
      }
      return 0;
#elif defined(OS_netbsd) || defined(OS_netbsdelf)
 	struct scsireq sc;

	memset(&sc, 0, sizeof(sc));
	memcpy(sc.cmd, cdb, cmdlen);
	sc.cmdlen = cmdlen;
	sc.databuf = data;
	sc.datalen = len;
	sc.senselen = 0;
	sc.timeout = 10000;
	switch (mode) {
	case SCSI_IO_READ:
	  sc.flags = SCCMD_READ;
	  break;
	case SCSI_IO_WRITE:
	  sc.flags = SCCMD_WRITE;
	  break;
	}

	if (ioctl(fd, SCIOCCOMMAND, &sc) == -1) {
                perror("SCIOCCOMMAND ioctl");
                return -1;
	}

	if (sc.retsts) {
                errno = EIO;
                fprintf(stderr, "SCSI command failed, retsts %d\n", 
sc.retsts);
                return -1;
	}

        return 0;
#else
      fprintf(stderr, "scsi_io not implemented\n");
      return -1;
#endif
}
