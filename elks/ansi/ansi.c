/*
 * ANSI driver for ELKS kernel, v0.0.1
 * Copyright (C) 2001, Memory Alpha Systems,
 * Licensed under the GNU General Public Licence, version 2 only.
 */

#include <stdio.h>

typedef unsigned char BYTE;
typedef unsigned short int WORD;

#define FALSE	0
#define TRUE	(!FALSE)

#define BufLen	1023

/* Prototypes */

void addcmd(char ch);
void chout(char ch);
void lineout(char *S, WORD N);

/* Variables and Buffers */

char Buffer[BufLen+1], Command[BufLen+1], *Next = Buffer, *NextCmd = Command;

char XmitOK = TRUE;

/* Functions */

void addcmd(char ch)
{
    if (NextCmd - Command < BufLen)
	*NextCmd++ = ch;
    else if (ch == ';') {
	while (* --NextCmd != ';')
	    /* Do nothing */;
	*NextCmd++ = ch;
    }
}

void ANSI(char cmd)
{
    char *Ptr = Buffer + 2;
    WORD A, B;

    *Next = '\0';
    switch (cmd) {

	case '/':	/* RM */

	case 'A':	/* CUU */
	case 'B':	/* CUD */
	case 'C':	/* CUF */
	case 'D':	/* CUB */
	    while ((A = getparm(&Ptr))) {
		if (!A)
		    A = 1;
		while (A--) {
		    chout(*Buffer);
		    chout(cmd);
		}
	    }
	    break;

	case 'H':	/* CUP */
	case 'f':	/* HVP */
	    A = getparm(&Ptr) + 31;
	    B = getparm(&Ptr) + 31;
	    chout(*Buffer);
	    chout('Y');
	    chout(A);
	    chout(B);
	    break;

	case 'J':	/* ED */
	case 'K':	/* EL */
	case 'R':	/* CPR */

	case 'c':	/* DA */
	case 'g':	/* TBC */
	case 'h':	/* SM */
	case 'm':	/* SGR */
	case 'n':	/* DSR */
	case 'q':	/* DECLL */
	case 'r':	/* DECSTBM */
	case 'x':	/* DECREQTPARM */
	case 'y':	/* DECTST */

	default:	/* Anything else */
	    break;
    }
    NextCmd = Command;
}

void DEC(char ch)
{
    switch (ch) {
	case '3':	/* DECDHL Top */
	case '4':	/* DECDHL Bottom */
	case '5':	/* DECSWL */
	case '6':	/* DECDWL */
	case '8':	/* DECALN */
    }
}

void chout(char ch)
{
    if (XmitOK) {
	lineout(Buffer,Next-Buffer);
	Next = Buffer;
    } else {
	*Next++ = ch;
	if (Next - Buffer == BufLen)
	    Next = Buffer;
    }
}

void lineout(char *S, WORD N)
{
    while (N) {
	chout( *S++ );
	N--;
    }
}

/* Main program */

int main(int argc, char **argv)
{
    char ch;

    while (TRUE) {
	ch = getchar();
	if (ch < ' ') {
	    switch (ch) {
		case 5:		/* ENQ */
		    lineout("ELKS-ANSI\r\n",11);
		    break;

		case 7:		/* BEL */
		case 8:		/* BS */
		    chout(ch);
		    break;

		case 9:		/* HT / TAB */

		case 10:	/* LF */
		case 11:	/* VT */
		case 12:	/* FF */
		    chout('\n');
		    break;

		case 13:	/* CR */
		    chout('\r');
		    break;

		case 14:	/* SO */

		case 15:	/* SI */

		case 17:	/* XON */
		    XmitOK = TRUE;
		    chout(0);
		    break;

		case 19:	/* XOFF */
		    XmitOK = FALSE;
		    break;

		case 24:	/* CAN */
		case 26:	/* SUB */
		    NextCmd = Command;
		    break;

		case 27:	/* ESC */
		    NextCmd = Command;
		    *NextCmd++ = ch;

		default:
		    break;
	    }
	} else {
	    switch (NextCmd-Command) {
		case 0:
		    chout(ch);
		    break;
		case 1:
		    *NextCmd++ = ch;
		    break;
		default:
		    if (Command[1] == '[') {
			if (index("0123456789;",ch))
			    *NextCmd++ = ch;
			else
			    ANSI(Command,ch);
		    } else {

		    }
		    break;
	}
    }
}
