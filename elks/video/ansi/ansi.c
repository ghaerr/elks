/************************************************************************
 *
 * ANSI driver for ELKS kernel, v0.0.1
 * Copyright (C) 2001, Memory Alpha Systems,
 * Licensed under the GNU General Public Licence, version 2 only.
 *
 ************************************************************************
 *
 * Notes regarding development:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Tue Aug 21 10:51:15 BST 2001: Riley Williams <rhw@MemAlpha.cx>
 *
 *	Discovered that it needs two streams of communication:
 *	stdin/stdout to talk to the user or application, and
 *	vidin/vidout to talk to the VT52 driver in the kernel.
 *
 ************************************************************************/

#include <stdio.h>

typedef unsigned char BYTE;
typedef unsigned short int WORD;

#define FALSE	0
#define TRUE	(!FALSE)

#define BufLen	1023

/************************************************************************
 * Variables and Buffers
 ************************************************************************/

WORD X = 1, Y = 1;

char Buffer[BufLen+1],  *Next    = Buffer;
char Command[BufLen+1], *NextCmd = Command;
char XmitOK = TRUE;

/************************************************************************
 * Function prototypes
 ************************************************************************/

extern char *index(char *,int);

void addcmd(char);
void ANSI(char);
void chout(char);
void DEC(char);
char getparm(char **);
void lineout(char *,WORD);

/************************************************************************
 * Functions
 ************************************************************************/

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
    char *Ptr = Command + 2;
    char A, B;

    *NextCmd = '\0';
    NextCmd = Command;
    switch (cmd) {
	case '/':					/* RM */
	    break;
	case 'A':					/* CUU */
	case 'B':					/* CUD */
	case 'C':					/* CUF */
	case 'D':					/* CUB */
	    while ((A = getparm(&Ptr))) {
		if (!A)
		    A = 1;
		while (A--) {
		    chout(*Buffer);
		    chout(cmd);
		}
	    }
	    break;
	case 'H':					/* CUP */
	case 'f':					/* HVP */
	    A = getparm(&Ptr) + 31;
	    B = getparm(&Ptr) + 31;
	    chout(*Buffer);
	    chout('Y');
	    chout(A);
	    chout(B);
	    break;
	case 'J':					/* ED */
	    break;
	case 'K':					/* EL */
	    break;
	case 'R':					/* CPR */
	    break;
	case 'c':					/* DA */
	    break;
	case 'g':					/* TBC */
	    break;
	case 'h':					/* SM */
	    break;
	case 'm':					/* SGR */
	    break;
	case 'n':					/* DSR */
	    break;
	case 'q':					/* DECLL */
	    break;
	case 'r':					/* DECSTBM */
	    break;
	case 'x':					/* DECREQTPARM */
	    break;
	case 'y':					/* DECTST */
	    break;
	default:					/* Anything else */
	    break;
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

void DEC(char ch)
{
    switch (ch) {
	case '3':	/* DECDHL Top */
	case '4':	/* DECDHL Bottom */
	case '5':	/* DECSWL */
	case '6':	/* DECDWL */
	case '8':	/* DECALN */
	    break;
    }
}

char getparm(char **S)
{
    char N = 0;

    while (index("0123456789",**S)) {
	N %= 1000;
	N *= 10;
	N += *(*S)++ - '0';
    }
    if (**S == ';')
	(*S)++;
    return N;
}

void lineout(char *S, WORD N)
{
    while (N) {
	chout( *S++ );
	N--;
    }
}

/************************************************************************
 * Main program
 ************************************************************************/

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
			    ANSI( ch );
		    } else {
			*NextCmd++ = ch;
		    }
		    break;
	    }
	}
    }
}

/*******/
/* EOF */
/*******/

