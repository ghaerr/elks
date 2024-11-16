#ifndef __LINUXMT_FD_H
#define __LINUXMT_FD_H

/*
 * Direct floppy (DF) driver header file
 */

/* place most of this driver in the far text section if possible */
#if defined(CONFIG_FARTEXT_KERNEL) && !defined(__STRICT_ANSI__)
#define DFPROC __far __attribute__ ((far_section, noinline, section (".fartext.df")))
#else
#define DFPROC
#endif

#if UNUSED
#define FDCLRPRM 0              /* clear user-defined parameters */
#define FDSETPRM 1              /* set user-defined parameters for current media */
#define FDDEFPRM 2              /* set user-defined parameters until explicitly cleared */
#define FDGETPRM 3              /* get disk parameters */
#define FDMSGON  4              /* issue kernel messages on media type change */
#define FDMSGOFF 5              /* don't issue kernel messages on media type change */
#define FDFMTBEG 6              /* begin formatting a disk */
#define FDFMTTRK 7              /* format the specified track */
#define FDFMTEND 8              /* end formatting a disk */
#define FDSETEMSGTRESH  10      /* set fdc error reporting treshold */
#define FDFLUSH  11             /* flush buffers for media; either for verifying media,
                                 * or for handling a media change without closing the
                                 * file descriptor */
#endif

struct floppy_struct {
    unsigned int size,          /* nr of 512-byte sectors total */
        sect,                   /* sectors per track */
        head,                   /* nr of heads */
        track,                  /* nr of tracks */
        stretch;                /* !=0 means double track steps */
    unsigned char gap,          /* gap1 size */
        rate,                   /* data rate. |= 0x40 for perpendicular */
        spec1,                  /* stepping rate, head unload time */
        fmt_gap;                /* gap2 size */
    const char *name;           /* used only for predefined formats */
};

#endif
