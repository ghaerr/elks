/*  Embedded Kermit demo, main program.  */

/*
  Author: Frank da Cruz, the Kermit Project, Columbia University, New York.
  Copyright (C) 1995, 2021,
  Trustees of Columbia University in the City of New York.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

  * Neither the name of Columbia University nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

/*
  This is a demo/test framework, to be replaced by a real control program.
  It includes a simple Unix-style command-line parser to allow setup and
  testing of the Kermit module.  Skip past all this to where it says REAL
  STUFF to see the real stuff.  ANSI C required.  Note: order of the
  following includes is important.
*/
#include "cdefs.h"      /* Data types for all modules */
#include "debug.h"	/* Debugging */
#include "platform.h"	/* Platform-specific includes and definitions */
#include "kermit.h"	/* Kermit symbols and data structures */
#ifdef __linux
#include <errno.h>
#include <stdlib.h>
#endif /* __linux */

/*
  Sample prototypes for i/o functions.
  The functions are defined in a platform-specific i/o module.
  The function names are unknown to the Kermit module.
  The names can be changed but not their calling conventions.
  The following prototypes are keyed to unixio.c.
*/
int devopen(char *);			/* Communications device/path */
int devsettings(char *);
int devrestore(void);
int devclose(void);
int pktmode(short);

int readpkt(struct k_data *, UCHAR *, int); /* Communications i/o functions */
int tx_data(struct k_data *, UCHAR *, int);
int inchk(struct k_data *);

int openfile(struct k_data *, UCHAR *, int); /* File i/o functions */
int writefile(struct k_data *, UCHAR *, int);
int readfile(struct k_data *);
int closefile(struct k_data *, UCHAR, int);
ULONG fileinfo(struct k_data *, UCHAR *, UCHAR *, int, short *, short);

/* External data */

extern UCHAR o_buf[];                   /* Must be defined in io.c */
extern UCHAR i_buf[];                   /* Must be defined in io.c */
extern int errno;

/* Data global to this module */

struct k_data k;                        /* Kermit data structure */
struct k_response r;                    /* Kermit response structure */

char **xargv;				/* Global pointer to arg vector */
UCHAR **cmlist = (UCHAR **)0;		/* Pointer to file list */
char * xname = "ek";			/* Default program name */

int xargc;				/* Global argument count */
int nfils = 0;				/* Number of files in file list */
int action = 0;				/* Send or Receive */
int xmode = 0;				/* File-transfer mode */
int ftype = 1;				/* Global file type 0=text 1=binary*/
int keep = 0;				/* Keep incompletely received files */
int db = 0;				/* Debugging */
short fmode = -1;			/* Transfer mode for this file */
int parity = 0;				/* Parity */
#ifdef F_CRC
int check = 3;				/* Block check */
#else
int check = 1;
#endif /* F_CRC */
int remote = 1;				/* 1 = Remote, 0 = Local */
#ifdef DEBUG
int errorrate = 0;			/* Simulated error rate */
int seed = 1234;			/* Random number generator seed */
#endif /* DEBUG */

void
doexit(int status) {
    devrestore();                       /* Restore device */
    devclose();                         /* Close device */
    exit(status);                       /* Exit with indicated status */
}

void
usage() {
    fprintf(stderr,"E-Kermit %s\n",VERSION);
    fprintf(stderr,"Usage: %s <options>\n",xname);
    fprintf(stderr,"Options:\n");
    fprintf(stderr," -r           Receive files\n");
#ifndef RECVONLY
    fprintf(stderr," -s <files>   Send files\n");
#endif /* RECVONLY */
    fprintf(stderr," -p [neoms]   Parity: none, even, odd, mark, space\n");
#ifdef F_CRC
    fprintf(stderr," -b [1235]    Block check type: 1, 2, 3, or 5\n");
#endif /* F_CRC */
    fprintf(stderr," -k           Keep incompletely received files\n");
    fprintf(stderr," -B           Force binary mode\n");
    fprintf(stderr," -T           Force text mode\n");
    fprintf(stderr," -R           Remote mode (vs local)\n");
    fprintf(stderr," -L           Local mode (vs remote)\n");
#ifdef DEBUG
    fprintf(stderr," -E <number>  Simulated error rate (0-100)\n");
    fprintf(stderr," -d           Create debug.log\n");
#endif /* DEBUG */
    fprintf(stderr," -h           Help (this message)\n");
    doexit(FAILURE);
}

void
fatal(char *msg1, char *msg2, char *msg3) { /* Not to be called except */
    if (msg1) {				    /* from this module */
	fprintf(stderr,"%s: %s",xname,msg1);
	if (msg2) fprintf(stderr,"%s",msg2);
	if (msg3) fprintf(stderr,"%s",msg3);
	fprintf(stderr,"\n");
    }
    doexit(FAILURE);
}

/* Simple user interface for testing */

int
doarg(char c) {				/* Command-line option parser */
    int x;				/* Parses one option with its arg(s) */
    char *xp, *s;
    struct stat statbuf;

    xp = *xargv+1;			/* Pointer for bundled args */
    while (c) {
#ifdef DEBUG
	if (errorrate) seed += (int)c;
#endif /* DEBUG) */
	switch (c) {
	  case 'r':			/* Receive */
	    if (action) fatal("Conflicting actions",(char *)0,(char *)0);
	    action = A_RECV;
	    break;

#ifndef RECVONLY
	  case 's':			/* Send */
	    if (action)
	      fatal("Conflicting actions",(char *)0,(char *)0);
	    if (*(xp+1))
	      fatal("Invalid argument bundling after -s",(char *)0,(char *)0);
	    nfils = 0;			/* Initialize file counter, flag */
	    cmlist = (UCHAR **)(xargv+1); /* Remember this pointer */
	    while (--xargc > 0) {	/* Traverse the list */
		xargv++;
		s = *xargv;
#ifdef DEBUG
		if (errorrate) seed += (int)*s;
#endif /* DEBUG) */
		if (**xargv == '-')
		  break;
		errno = 0;
		x = stat(s,&statbuf);
		if (x < 0)
		  fatal("File '",s,"' not found");
		if (access(s,4) < 0)
		  fatal("File '",s,"' not accessible");
		nfils++;
	    }
	    xargc++, *xargv--;		/* Adjust argv/argc */
	    if (nfils < 1)
	      fatal("Missing filename for -s",(char *)0,(char *)0);
	    action = A_SEND;
	    break;
#endif /* RECVONLY */

#ifdef F_CRC
	  case 'b':			/* Block-check type */
#endif /* F_CRC */
#ifdef DEBUG
	  case 'E':			/* Simulated error rate */
#endif /* DEBUG */
	    if (*(xp+1))
	      fatal("Invalid argument bundling",(char *)0,(char *)0);
	    *xargv++, xargc--;
	    if ((xargc < 1) || (**xargv == '-'))
	      fatal("Missing option argument",(char *)0,(char *)0);
	    s = *xargv;
	    while (*s) {
		if (!isdigit(*s))
		  fatal("Numeric argument required",(char *)0,(char *)0);
		s++;
	    }
	    if (c == 'b') {
		check = atoi(*xargv);
		if (check < 1 || check > 5 || check == 4)
		  fatal("Invalid block check",(char *)0,(char *)0);
#ifdef DEBUG
	    } else if (c == 'E') {
		errorrate = atoi(*xargv);
		if (errorrate > 100)
		  fatal("Invalid error rate",(char *)0,(char *)0);
#endif /* DEBUG */
	    }
	    break;

	  case 'h':			/* Help */
	  case '?':
	    usage();

	  case 'B':			/* Force binary file transfer */
	    xmode = 1;			/* So no automatic switching */
	    ftype = BINARY;
	    break;

	  case 'T':			/* Force text file transfer */
	    xmode = 1;			/* So no automatic switching */
	    ftype = TEXT;
	    break;

	  case 'R':			/* Tell Kermit it's in remote mode */
	    remote = 1;
	    break;

	  case 'L':			/* Tell Kermit it's in local mode */
	    remote = 0;
	    break;

	  case 'k':			/* Keep incompletely received files */
	    keep = 1;
	    break;

	  case 'p':			/* Parity */
	    if (*(xp+1))
	      fatal("Invalid argument bundling",(char *)0,(char *)0);
	    *xargv++, xargc--;
	    if ((xargc < 1) || (**xargv == '-'))
	      fatal("Missing parity",(char *)0,(char *)0);
	    switch(x = **xargv) {
	      case 'e':			/* Even */
	      case 'o':			/* Odd */
	      case 'm':			/* Mark */
	      case 's': parity = x; break; /* Space */
	      case 'n': parity = 0; break; /* None */
	      default:  fatal("Invalid parity '", *xargv, "'");
	    }
	    break;

#ifdef DEBUG
	  case 'd':
	    db++;
	    break;
#endif /* DEBUG */

	  default:			/* Anything else */
	    fatal("Unknown command-line option ",
		  *xargv,
		  " type 'ek -h' for help."
		  );
        }
	c = *++xp;			/* See if options are bundled */
    }
    return(action);
}

void
main(int argc, char ** argv) {
    int status, rx_len, i, x;
    char c;
    UCHAR *inbuf;
    short r_slot;

    parity = P_PARITY;                  /* Set this to desired parity */
    status = X_OK;                      /* Initial kermit status */

    xargc = argc;
    xargv = argv;
    xname = argv[0];

    while (--xargc > 0) {		/* Loop through command-line words */
	xargv++;
	if (**xargv == '-') {		/* Have dash */
	    c = *(*xargv+1);		/* Get the option letter */
	    x = doarg(c);		/* Go handle the option */
	    if (x < 0) doexit(FAILURE);
	} else {			/* No dash where expected */
	    fatal("Malformed command-line option: '",*xargv,"'");
	}
    }
    if (!action)			/* Nothing to do, give usage message */
      usage();

/* THE REAL STUFF IS FROM HERE DOWN */

    if (!devopen("dummy"))		/* Open the communication device */
      doexit(FAILURE);
    if (!devsettings("dummy"))		/* Perform any needed settings */
      doexit(FAILURE);
    if (db)				/* Open debug log if requested */
      debug(DB_OPN,"debug.log",0,0);

    debug(DB_MSG,"Initializing...",0,0);
    debug(DB_LOG,"SIMULATED ERROR RATE:",0,errorrate);
    if (errorrate) srand(seed);		/* Init random error generator */

/*  Fill in parameters for this run */

    k.xfermode = xmode;			/* Text/binary automatic/manual  */
    k.remote = remote;			/* Remote vs local */
    k.binary = ftype;			/* 0 = text, 1 = binary */
    k.parity = parity;                  /* Communications parity */
    k.bct = (check == 5) ? 3 : check;	/* Block check type */
    k.ikeep = keep;			/* Keep incompletely received files */
    k.filelist = cmlist;		/* List of files to send (if any) */
    k.cancel = 0;			/* Not canceled yet */

/*  Fill in the i/o pointers  */

    k.zinbuf = i_buf;			/* File input buffer */
    k.zinlen = IBUFLEN;			/* File input buffer length */
    k.zincnt = 0;			/* File input buffer position */
    k.obuf = o_buf;			/* File output buffer */
    k.obuflen = OBUFLEN;		/* File output buffer length */
    k.obufpos = 0;			/* File output buffer position */

/* Fill in function pointers */

    k.rxd    = readpkt;			/* for reading packets */
    k.txd    = tx_data;			/* for sending packets */
    k.ixd    = inchk;			/* for checking connection */
    k.openf  = openfile;                /* for opening files */
    k.finfo  = fileinfo;                /* for getting file info */
    k.readf  = readfile;		/* for reading files */
    k.writef = writefile;               /* for writing to output file */
    k.closef = closefile;               /* for closing files */
#ifdef DEBUG
    k.dbf    = db ? dodebug : 0;	/* for debugging */
#else
    k.dbf    = 0;
#endif /* DEBUG */
    /* Force Type 3 Block Check (16-bit CRC) on all packets, or not */
    k.bctf   = (check == 5) ? 1 : 0;

/* Initialize Kermit protocol */

    status = kermit(K_INIT, &k, 0, 0, "", &r);
#ifdef DEBUG
    debug(DB_LOG,"init status:",0,status);
    debug(DB_LOG,"version:",k.version,0);
#endif /* DEBUG */
    if (status == X_ERROR)
      doexit(FAILURE);
    if (action == A_SEND)
      status = kermit(K_SEND, &k, 0, 0, "", &r);
/*
  Now we read a packet ourselves and call Kermit with it.  Normally, Kermit
  would read its own packets, but in the embedded context, the device must be
  free to do other things while waiting for a packet to arrive.  So the real
  control program might dispatch to other types of tasks, of which Kermit is
  only one.  But in order to read a packet into Kermit's internal buffer, we
  have to ask for a buffer address and slot number.

  To interrupt a transfer in progress, set k.cancel to I_FILE to interrupt
  only the current file, or to I_GROUP to cancel the current file and all
  remaining files.  To cancel the whole operation in such a way that the
  both Kermits return an error status, call Kermit with K_ERROR.
*/
    while (status != X_DONE) {
/*
  Here we block waiting for a packet to come in (unless readpkt times out).
  Another possibility would be to call inchk() to see if any bytes are waiting
  to be read, and if not, go do something else for a while, then come back
  here and check again.
*/
        inbuf = getrslot(&k,&r_slot);	/* Allocate a window slot */
        rx_len = k.rxd(&k,inbuf,P_PKTLEN); /* Try to read a packet */
        debug(DB_PKT,"main packet",&(k.ipktbuf[0][r_slot]),rx_len);
/*
  For simplicity, kermit() ACKs the packet immediately after verifying it was
  received correctly.  If, afterwards, the control program fails to handle the
  data correctly (e.g. can't open file, can't write data, can't close file),
  then it tells Kermit to send an Error packet next time through the loop.
*/
        if (rx_len < 1) {               /* No data was read */
            freerslot(&k,r_slot);	/* So free the window slot */
            if (rx_len < 0)             /* If there was a fatal error */
              doexit(FAILURE);          /* give up */

	    /* This would be another place to dispatch to another task */
	    /* while waiting for a Kermit packet to show up. */

        }
        /* Handle the input */

        switch (status = kermit(K_RUN, &k, r_slot, rx_len, "", &r)) {
	  case X_OK:
#ifdef DEBUG
/*
  This shows how, after each packet, you get the protocol state, file name,
  date, size, and bytes transferred so far.  These can be used in a
  file-transfer progress display, log, etc.
*/
	    debug(DB_LOG,"NAME",r.filename ? (char *)r.filename : "(NULL)",0);
	    debug(DB_LOG,"DATE",r.filedate ? (char *)r.filedate : "(NULL)",0);
	    debug(DB_LOG,"SIZE",0,r.filesize);
	    debug(DB_LOG,"STATE",0,r.status);
	    debug(DB_LOG,"SOFAR",0,r.sofar);
#endif /* DEBUG */
	    /* Maybe do other brief tasks here... */
	    continue;			/* Keep looping */
	  case X_DONE:
	    break;			/* Finished */
	  case X_ERROR:
	    doexit(FAILURE);		/* Failed */
	}
    }
    doexit(SUCCESS);
}
