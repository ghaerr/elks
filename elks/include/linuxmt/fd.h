#ifndef _LINUX_FD_H
#define _LINUX_FD_H

#define FDCLRPRM 0 /* clear user-defined parameters */
#define FDSETPRM 1 /* set user-defined parameters for current media */
#define FDDEFPRM 2 /* set user-defined parameters until explicitly cleared */
#define FDGETPRM 3 /* get disk parameters */
#define	FDMSGON  4 /* issue kernel messages on media type change */
#define	FDMSGOFF 5 /* don't issue kernel messages on media type change */
#define FDFMTBEG 6 /* begin formatting a disk */
#define	FDFMTTRK 7 /* format the specified track */
#define FDFMTEND 8 /* end formatting a disk */
#define FDSETEMSGTRESH	10	/* set fdc error reporting treshold */
#define FDFLUSH  11 /* flush buffers for media; either for verifying media, or for
                       handling a media change without closing the file
		       descriptor */

#define FD_FILL_BYTE 0xF6 /* format fill byte */

#define FORMAT_NONE	0	/* no format request */
#define FORMAT_WAIT	1	/* format request is waiting */
#define FORMAT_BUSY	2	/* formatting in progress */
#define FORMAT_OKAY	3	/* successful completion */
#define FORMAT_ERROR	4	/* formatting error */

struct floppy_struct {
	unsigned int	size,		/* nr of 512-byte sectors total */
			sect,		/* sectors per track */
			head,		/* nr of heads */
			track,		/* nr of tracks */
			stretch;	/* !=0 means double track steps */
	unsigned char	gap,		/* gap1 size */
			rate,		/* data rate. |= 0x40 for perpendicular */
			spec1,		/* stepping rate, head unload time */
			fmt_gap;	/* gap2 size */
	char 	      * name; /* used only for predefined formats */
};

struct format_descr {
	unsigned int device,head,track;
};

#endif
