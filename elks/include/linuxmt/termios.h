#ifndef __LINUXMT_TERMIOS_H
#define __LINUXMT_TERMIOS_H

#include <linuxmt/types.h>

/* This is just a magic number to make these relatively unique ('T') */
#define __TERMIOS_MAJ 	('T'<<8)

#define TCGETS		(__TERMIOS_MAJ+0x01)
#define TCSETS		(__TERMIOS_MAJ+0x02)
#define TCSETSW		(__TERMIOS_MAJ+0x03)
#define TCSETSF		(__TERMIOS_MAJ+0x04)
#define TCGETA		(__TERMIOS_MAJ+0x05)
#define TCSETA		(__TERMIOS_MAJ+0x06)
#define TCSETAW		(__TERMIOS_MAJ+0x07)
#define TCSETAF		(__TERMIOS_MAJ+0x08)
#define TCSBRK		(__TERMIOS_MAJ+0x09)
#define TCXONC		(__TERMIOS_MAJ+0x0A)
#define TCFLSH		(__TERMIOS_MAJ+0x0B)
#define TIOCEXCL	(__TERMIOS_MAJ+0x0C)
#define TIOCNXCL	(__TERMIOS_MAJ+0x0D)
#define TIOCSCTTY	(__TERMIOS_MAJ+0x0E)
#define TIOCGPGRP	(__TERMIOS_MAJ+0x0F)
#define TIOCSPGRP	(__TERMIOS_MAJ+0x10)
#define TIOCOUTQ	(__TERMIOS_MAJ+0x11)
#define TIOCSTI		(__TERMIOS_MAJ+0x12)
#define TIOCGWINSZ	(__TERMIOS_MAJ+0x13)
#define TIOCSWINSZ	(__TERMIOS_MAJ+0x14)
#define TIOCMGET	(__TERMIOS_MAJ+0x15)
#define TIOCMBIS	(__TERMIOS_MAJ+0x16)
#define TIOCMBIC	(__TERMIOS_MAJ+0x17)
#define TIOCMSET	(__TERMIOS_MAJ+0x18)
#define TIOCGSOFTCAR	(__TERMIOS_MAJ+0x19)
#define TIOCSSOFTCAR	(__TERMIOS_MAJ+0x1A)
#define FIONREAD	(__TERMIOS_MAJ+0x1B)
#define TIOCINQ		FIONREAD
#define TIOCLINUX	(__TERMIOS_MAJ+0x1C)
#define TIOCCONS	(__TERMIOS_MAJ+0x1D)
#define TIOCGSERIAL	(__TERMIOS_MAJ+0x1E)
#define TIOCSSERIAL	(__TERMIOS_MAJ+0x1F)
#define TIOCPKT		(__TERMIOS_MAJ+0x20)
#define FIONBIO		(__TERMIOS_MAJ+0x21)
#define TIOCNOTTY	(__TERMIOS_MAJ+0x22)
#define TIOCSETD	(__TERMIOS_MAJ+0x23)
#define TIOCGETD	(__TERMIOS_MAJ+0x24)
#define TCSBRKP		(__TERMIOS_MAJ+0x25)	/* Needed for POSIX tcsendbreak() */
#define TIOCTTYGSTRUCT	(__TERMIOS_MAJ+0x26)	/* For debugging only */
#define FIONCLEX	(__TERMIOS_MAJ+0x50)	/* these numbers need to be adjusted. */
#define FIOCLEX		(__TERMIOS_MAJ+0x51)
#define FIOASYNC	(__TERMIOS_MAJ+0x52)
#define TIOCSERCONFIG	(__TERMIOS_MAJ+0x53)
#define TIOCSERGWILD	(__TERMIOS_MAJ+0x54)
#define TIOCSERSWILD	(__TERMIOS_MAJ+0x55)
#define TIOCGLCKTRMIOS	(__TERMIOS_MAJ+0x56)
#define TIOCSLCKTRMIOS	(__TERMIOS_MAJ+0x57)
#define TIOCSERGSTRUCT	(__TERMIOS_MAJ+0x58)	/* For debugging only */
#define TIOCSERGETLSR   (__TERMIOS_MAJ+0x59)	/* Get line status register */
#define TIOCSERGETMULTI (__TERMIOS_MAJ+0x5A)	/* Get multiport config  */
#define TIOCSERSETMULTI (__TERMIOS_MAJ+0x5B)	/* Set multiport config */

#define TIOCMIWAIT	(__TERMIOS_MAJ+0x5C)	/* wait for a change on serial input line(s) */
#define TIOCGICOUNT	(__TERMIOS_MAJ+0x5D)	/* read serial port inline interrupt counts */
#define TIOSETCONSOLE	(__TERMIOS_MAJ+0x5E)	/* set console dev_t*/

/* Used for packet mode */
#define TIOCPKT_DATA		 0
#define TIOCPKT_FLUSHREAD	 1
#define TIOCPKT_FLUSHWRITE	 2
#define TIOCPKT_STOP		 4
#define TIOCPKT_START		 8
#define TIOCPKT_NOSTOP		16
#define TIOCPKT_DOSTOP		32

struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

#if NOTSUPPORTED
#define NCC 8
struct termio {
    unsigned short c_iflag;	/* input mode flags */
    unsigned short c_oflag;	/* output mode flags */
    unsigned short c_cflag;	/* control mode flags */
    unsigned short c_lflag;	/* local mode flags */
    unsigned char c_line;	/* line discipline */
    unsigned char c_cc[NCC];	/* control characters */
};
#endif

#define NCCS 17
struct termios {
    tcflag_t c_iflag;		/* input mode flags */
    tcflag_t c_oflag;		/* output mode flags */
    tcflag_t c_cflag;		/* control mode flags */
    tcflag_t c_lflag;		/* local mode flags */
    cc_t c_line;		/* line discipline */
    cc_t c_cc[NCCS];		/* control characters */
};

/* c_cc characters */
#define VINTR 0
#define VQUIT 1
#define VERASE 2
#define VKILL 3
#define VEOF 4
#define VTIME 5
#define VMIN 6
#define VSWTCH 7
#define VSTART 8
#define VSTOP 9
#define VSUSP 10
#define VEOL 11
#define VREPRINT 12
#define VDISCARD 13
#define VWERASE 14
#define VLNEXT 15
#define VERASE2 16

#ifdef __KERNEL__
/*	intr=^C		quit=^|		erase=^H	kill=^U
	eof=^D		vtime=\0	vmin=\1		sxtc=\0
	start=^Q	stop=^S		susp=^Z		eol=\0
	reprint=^R	discard=^U	werase=^W	lnext=^V
	erase2=\177
*/
#define INIT_C_CC "\003\034\007\025\004\0\1\0\021\023\032\0\022\017\027\026\177"
#endif

/* c_iflag bits */
#define IGNBRK	0000001
#define BRKINT	0000002
#define IGNPAR	0000004
#define PARMRK	0000010
#define INPCK	0000020
#define ISTRIP	0000040
#define INLCR	0000100
#define IGNCR	0000200
#define ICRNL	0000400
#define IUCLC	0001000
#define IXON	0002000
#define IXANY	0004000
#define IXOFF	0010000
#define IMAXBEL	0020000
#define IUTF8	0040000

/* c_oflag bits */
#define OPOST	0000001
#define OLCUC	0000002
#define ONLCR	0000004
#define OCRNL	0000010
#define ONOCR	0000020
#define ONLRET	0000040
#define OFILL	0000100
#define OFDEL	0000200
#define NLDLY	0000400
#define   NL0	0000000
#define   NL1	0000400
#define CRDLY	0003000
#define   CR0	0000000
#define   CR1	0001000
#define   CR2	0002000
#define   CR3	0003000
#define TABDLY	0014000
#define   TAB0	0000000
#define   TAB1	0004000
#define   TAB2	0010000
#define   TAB3	0014000
#define   XTABS	0014000
#define BSDLY	0020000
#define   BS0	0000000
#define   BS1	0020000
#define VTDLY	0040000
#define   VT0	0000000
#define   VT1	0040000
#define FFDLY	0100000
#define   FF0	0000000
#define   FF1	0100000

/* c_cflag bit meaning */
#define CBAUD	0010017
#define  B0	0000000		/* hang up */
#define  B50	0000001
#define  B75	0000002
#define  B110	0000003
#define  B134	0000004
#define  B150	0000005
#define  B200	0000006
#define  B300	0000007
#define  B600	0000010
#define  B1200	0000011
#define  B1800	0000012
#define  B2400	0000013
#define  B4800	0000014
#define  B9600	0000015
#define  B19200	0000016
#define  B38400	0000017

#define EXTA B19200
#define EXTB B38400

#define CSIZE	0000060
#define   CS5	0000000
#define   CS6	0000020
#define   CS7	0000040
#define   CS8	0000060
#define CSTOPB	0000100
#define CREAD	0000200
#define PARENB	0000400
#define PARODD	0001000
#define HUPCL	0002000
#define CLOCAL	0004000
#define CBAUDEX 0010000
#define  B57600  0010001
#define  B115200 0010002
#define  B230400 0010003
#define CIBAUD	  002003600000	/* input baud rate (not used) */
#define CRTSCTS	  020000000000	/* flow control */

/* c_lflag bits */
#define ISIG	0000001
#define ICANON	0000002
#define XCASE	0000004
#define ECHO	0000010
#define ECHOE	0000020
#define ECHOK	0000040
#define ECHONL	0000100
#define NOFLSH	0000200
#define TOSTOP	0000400
#define ECHOCTL	0001000
#define ECHOPRT	0002000
#define ECHOKE	0004000
#define FLUSHO	0010000
#define PENDIN	0040000
#define IEXTEN	0100000

/* modem lines */
#define TIOCM_LE	0x001
#define TIOCM_DTR	0x002
#define TIOCM_RTS	0x004
#define TIOCM_ST	0x008
#define TIOCM_SR	0x010
#define TIOCM_CTS	0x020
#define TIOCM_CAR	0x040
#define TIOCM_RNG	0x080
#define TIOCM_DSR	0x100
#define TIOCM_CD	TIOCM_CAR
#define TIOCM_RI	TIOCM_RNG

/* ioctl (fd, TIOCSERGETLSR, &result) where result may be as below */
#define TIOCSER_TEMT    0x01	/* Transmitter physically empty */

/* tcflow() and TCXONC use these */
#define	TCOOFF		0
#define	TCOON		1
#define	TCIOFF		2
#define	TCION		3

/* tcflush() and TCFLSH use these */
#define	TCIFLUSH	0
#define	TCOFLUSH	1
#define	TCIOFLUSH	2

/* tcsetattr uses these */
#define	TCSANOW		0
#define	TCSADRAIN	1
#define	TCSAFLUSH	2

/* line disciplines */
#define N_TTY		0
#define N_SLIP		1
#define N_MOUSE		2
#define N_PPP		3

#define _POSIX_VDISABLE		'\0'

#endif
