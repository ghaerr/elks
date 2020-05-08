/* From linux libc-4.6.27 again */
#ifdef L_cfmakeraw
#include <termios.h>
/* Copyright (C) 1992 Free Software Foundation, Inc.
This file is part of the GNU C Library.*/

void
cfmakeraw(struct termios *t)
{
/* I changed it to the current form according to the suggestions
 * from Bruce Evans. Thanks Bruce. Please report the problems to
 * H.J. Lu (hlu@eecs.wsu.edu).
 */

/*
 * I took out the bits commented out by #if 1...#else    - RHP
 */

	/*  VMIN = 0 means non-blocking for Linux */
	t->c_cc[VMIN] = 1;
	t->c_cc[VTIME] = 1;
	/* clear some bits with &= ~(bits), set others with |= */
	t->c_cflag &= ~(CSIZE | PARENB | CSTOPB);
	t->c_cflag |= (CS8 | CREAD);
	t->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | INPCK | ISTRIP);
	t->c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
	t->c_iflag |= (BRKINT | IGNPAR);
	t->c_oflag &= ~(OPOST | OLCUC | OCRNL | ONOCR | ONLRET | OFILL | OFDEL);
	t->c_oflag &= ~(NLDLY | CRDLY | TABDLY | BSDLY | VTDLY | FFDLY);
	t->c_oflag |= (ONLCR | NL0 | CR0 | TAB3 | BS0 | VT0 | FF0);
	t->c_lflag &= ~(ISIG | ICANON | IEXTEN | ECHO | ECHOE | ECHOK | ECHONL);
	t->c_lflag &= ~(NOFLSH | XCASE);
	t->c_lflag &= ~(ECHOPRT | ECHOCTL | ECHOKE);
}
#endif
