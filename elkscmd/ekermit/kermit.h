#ifndef __KERMIT_H__
#define __KERMIT_H__

#define VERSION "1.8"			/* Kermit module version number */

/*
  kermit.h -- Symbol and struct definitions for embedded Kermit.

  As of version 1.6 of 30 March 2011, E-Kermit is Open Source software under
  the Revised 3-Clause BSD license which follows.  E-Kermit 1.6 is identical
  to version 1.51 except for the new license.

  Author: Frank da Cruz.

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

#ifdef COMMENT                          /* COMMENT must not be defined */
#undef COMMENT				/* (e.g. in a platform header file) */
#endif /* COMMENT */

/*
  Never use NULL as a pointer.  Always use 0 cast to the appropriate type,
  for example: (UCHAR *)0.  Reason: compiler might define NULL to be an
  integer 0, and when passed to a function that wants a pointer, might wind
  up with junk in the left half (if pointers wider than ints).
*/
#ifdef NULL
#undef NULL
#endif /* NULL */

/* Feature Selection */

/* XAC compiler for Philips XAG30 microprocessor */
/* See http://www.columbia.edu/kermit/em-apex.html */

#ifdef XAC				/* XAC has tiny command line */
#define NO_LP				/* Long packets too big for APF9 */
#define NO_SSW
#define NO_SCAN				/* No file system */
#define FN_MAX  16
#define IBUFLEN 128
#define OBUFLEN 512

#else  /* XAC */

#ifdef MINSIZE
#define NO_LP
#define NO_AT
#define NO_CTRLC
#define NO_SSW
#define NO_CRC
#define NO_SCAN
#endif	/* MINSIZE */

#endif	/* XAC */

#ifndef NO_LP
#define F_LP                            /* Long packets */
#endif	/* NO_LP */

#ifndef NO_AT
#define F_AT                            /* Attribute packets */
#endif	/* NO_AT */

#ifndef NO_CTRLC
#define F_CTRLC                         /* 3 consecutive Ctrl-C's to quit */
#endif	/* NO_CTRLC */

#ifndef NO_SSW
#define F_SSW				/* Simulated sliding windows */
#endif	/* NO_SSW */

#ifndef NO_SCAN
#define F_SCAN				/* Scan files for text/binary */
#endif	/* NO_SCAN */

#ifndef NO_CRC				/* Type 2 and 3 block checks */
#define F_CRC
#endif /* NO_CRC */

/*
  F_SSW means we say (in negotiations) that we support sliding windows, but we
  really don't.  This allows the sender to send to us in a steady stream, and
  works just fine except that error recovery is via go-back-to-n rather than
  selective repeat.
*/

#ifdef COMMENT                          /* None of the following ... */
/*
  + = It works if selected
  - = Partially implemented but doesn't work
  0 = Not implemented
*/
  #define F_TSW                         /* - True sliding windows */
  #define F_LS                          /* 0 Locking shifts */
  #define F_RS                          /* 0 Recovery */

#endif /* COMMENT */

#ifdef F_TSW				/* F_SW is defined if either */
#ifndef F_SW				/* F_SSW or F_TSW is defined... */
#define F_SW
#endif /* F_SW */
#endif /* F_TSW */

#ifdef F_SSW
#ifndef F_SW
#define F_SW
#endif /* F_SW */
#endif /* F_SSW */

/* Control character symbols */

#define NUL  '\0'                       /* Null */
#define SOH  001                        /* Start of header */
#define LF   012                        /* Linefeed */
#define CR   015                        /* Carriage Return */
#define SO   016                        /* Shift Out */
#define SI   017                        /* Shift In */
#define DLE  020                        /* Datalink Escape */
#define ESC  033                        /* Escape */
#define XON  021                        /* XON */
#define XOFF 023                        /* XOFF */
#define SP   040                        /* Space */
#define DEL  0177                       /* Delete (Rubout) */

#ifndef HAVE_VERSION			/* k_data struct has version member */
#define HAVE_VERSION			/* as of version 1.1 */
#endif /* HAVE_VERSION */

/* Main program return codes */

#define SUCCESS     0
#define FAILURE     1

/* Buffer lengths (can be overridden in platform.h) */

#ifndef RECVONLY
#ifndef IBUFLEN
#define IBUFLEN  1024			/* File input buffer size */
#endif /* IBUFLEN */
#endif	/* RECVONLY */

#ifndef OBUFLEN
#define OBUFLEN  1024                   /* File output buffer size */
#endif /* OBUFLEN */

#ifndef IDATALEN			/* S/I packet data max length */
#define IDATALEN 32
#endif /* IDATALEN */

#ifndef FN_MAX
#define FN_MAX   1024                   /* Maximum filename length */
#endif /* FN_MAX */

#define DATE_MAX   20                   /* Max length for file date */

/* Protocol parameters */

#ifndef P_WSLOTS
#ifdef F_SW                             /* Window slots */
#ifdef F_TSW				/* True window slots */
#define P_WSLOTS    4			/* Max is 4 */
#else
#define P_WSLOTS   31			/* Simulated max is 31 */
#endif /* F_TSW */
#else
#define P_WSLOTS    1
#endif /* F_SW */
#endif /* P_WSLOTS */

#ifndef P_PKTLEN			/* Kermit max packet length */
#ifdef F_LP
#define P_PKTLEN 4096
#else
#define P_PKTLEN   94
#endif /* F_LP */
#endif /* P_PKTLEN */

/* Generic On/Off values */

#define OFF         0
#define ON          1

/* File Transfer Modes */

#define BINARY      1                   /* Corrected in E-Kermit 1.8 */
#define TEXT        0                   /* Corrected in E-Kermit 1.8 */

/* Parity values */

#define PAR_NONE    0
#define PAR_SPACE   1
#define PAR_EVEN    2
#define PAR_ODD     3
#define PAR_MARK    4

/* Protocol parameters */

#define P_S_TIMO   40                   /* Timeout to tell other Kermit  */
#define P_R_TIMO    5                   /* Default timeout for me to use */
#define P_RETRY    10                   /* Per-packet retramsit limit    */
#define P_PARITY  PAR_NONE              /* Default parity        */
#define P_R_SOH   SOH                   /* Incoming packet start */
#define P_S_SOH   SOH                   /* Outbound packet start */
#define P_R_EOM    CR                   /* Incoming packet end   */
#define P_S_EOM    CR                   /* Outbound packet end   */

/* Capability bits */

#define CAP_LP      2                   /* Long packet capability */
#define CAP_SW      4                   /* Sliding windows capability */
#define CAP_AT      8                   /* Attribute packet capability */
#define CAP_RS     16                   /* Resend capability */
#define CAP_LS     32                   /* Locking shift capability */

/* Actions */

#define A_SEND      1			/* Send file(s) */
#define A_RECV      2			/* Receive file(s) */

/* Receive protocol states */

#define R_ERROR    -1                   /* Fatal protocol error */
#define R_NONE      0                   /* Protocol not running */
#define R_WAIT      1                   /* Waiting for S packet */
#define R_FILE      2                   /* Waiting for F or B packet */
#define R_ATTR      3                   /* Waiting for A or D packet */
#define R_DATA      4                   /* Waiting for D or Z packet */

/* Send protocol states */

#define S_ERROR    -1                   /* Fatal protocol error */
#define S_NONE     10                   /* Protocol not running */
#define S_INIT     11                   /* Sent S packet */
#define S_FILE     12                   /* Sent F packet */
#define S_ATTR     13                   /* Sent A packet */
#define S_DATA     14                   /* Sent D packet */
#define S_EOF      15                   /* Sent Z packet */
#define S_EOT      16                   /* Sent B packet */

/* What I'm Doing */

#define W_NOTHING   0
#define W_SEND      1
#define W_RECV      2

/* Kermit module function codes */

#define K_INIT      0                   /* Initialize */
#define K_RUN       1                   /* Run */
#define K_STATUS    2                   /* Request status */
#define K_CONTINUE  3                   /* Keep going */
#define K_QUIT      4                   /* Quit immediately */
#define K_ERROR     5                   /* Quit with error packet, msg given */
#define K_SEND      6			/* Begin Send sequence */

/* Kermit module return codes */

#define X_ERROR    -1                   /* Fatal error */
#define X_OK        0                   /* OK, no action needed */
#define X_FILE      1                   /* Filename received */
#define X_DATA      2                   /* File data received */
#define X_DONE      3                   /* Done */
#define X_STATUS    4                   /* Status report */

/* Interruption codes */

#define I_FILE      1			/* Cancel file */
#define I_GROUP     2			/* Cancel group */

struct packet {
    int len;                            /* Length */
    short seq;                          /* Sequence number */
    char typ;                           /* Type */
    short rtr;                          /* Retry count */
    UCHAR * dat;                        /* Pointer to data */
    short flg;				/* Flags */
};

struct k_data {                         /* The Kermit data structure */
    UCHAR * version;			/* Version number of Kermit module */
    short remote;			/* 0 = local, 1 = remote */
    short xfermode;			/* 0 = automatic, 1 = manual */
    short binary;                       /* 0 = text, 1 = binary */
    short state;                        /* Kermit protocol state */
    short what;				/* Action (send or receive) */
    short s_first;			/* Enocode at beginning of file */
    short s_next;			/* Encode lookahead byte */
    short s_seq;                        /* Sequence number sent */
    short r_seq;                        /* Sequence number received */
    short s_type;                       /* Packet type sent */
    short r_type;                       /* Packet type received */
    short s_soh;                        /* Packet start sent */
    short r_soh;                        /* Packet start received */
    short s_eom;                        /* Packet end sent */
    short r_eom;                        /* Packet end received */
    int size;				/* Current size of output pkt data */
    int osize;				/* Previous output packet data size */
    int r_timo;                         /* Receive and send timers */
    int s_timo;                         /* ... */
    int r_maxlen;                       /* maximum packet length to receive */
    int s_maxlen;                       /* maximum packet length to send */
    short window;                       /* maximum window slots */
    short wslots;                       /* current window slots */
    short parity;                       /* 0 = none, nonzero = some */
    short retry;                        /* retry limit */
    short cancel;			/* Cancellation */
    short ikeep;			/* Keep incompletely received files */
    char s_ctlq;                        /* control-prefix out */
    char r_ctlq;                        /* control-prefix in */
    char ebq;				/* 8-bit prefix */
    char ebqflg;			/* 8-bit prefixing negotiated */
    char rptq;				/* Repeat-count prefix */
    int s_rpt;				/* Current repeat count */
    short rptflg;                       /* flag for repeat counts negotiated */
    short bct;                          /* Block-check type 1..3 */
    unsigned short capas;               /* Capability bits */
#ifdef F_CRC
    USHORT crcta[16];			/* CRC generation table A */
    USHORT crctb[16];			/* CRC generation table B */
#endif /* F_CRC */
    UCHAR s_remain[6];			 /* Send data leftovers */
    UCHAR ipktbuf[P_PKTLEN+8][P_WSLOTS]; /* Buffers for incoming packets */
    struct packet ipktinfo[P_WSLOTS];    /* Incoming packet info */
#ifdef COMMENT
    UCHAR opktbuf[P_PKTLEN+8][P_WSLOTS]; /* Buffers for outbound packets */
#else
    UCHAR opktbuf[P_PKTLEN+8];		/* Outbound packet buffer */
    int opktlen;			/* Outbound packet length */
    UCHAR xdatabuf[P_PKTLEN+2];		/* Buffer for building data field */
#endif /* COMMENT */
    struct packet opktinfo[P_WSLOTS];	/* Outbound packet info */
    UCHAR * xdata;			/* Pointer to data field of outpkt */
#ifdef F_TSW
    short r_pw[64];			/* Packet Seq.No. to window-slot map */
    short s_pw[64];			/* Packet Seq.No. to window-slot map */
#endif /* F_TSW */
    UCHAR ack_s[IDATALEN];		/* Our own init parameter string */
    UCHAR * obuf;
    int rx_avail;			/* Comms bytes available for reading */
    int obuflen;                        /* Length of output file buffer */
    int obufpos;                        /* Output file buffer position */
    UCHAR ** filelist;			/* List of files to send */
    UCHAR * dir;			/* Directory */
    UCHAR * filename;			/* Name of current file */
    UCHAR * istring;			/* Pointer to string to encode from */
    UCHAR * ostring;			/* Pointer to string to decode to */
    int (*rxd)(struct k_data *, UCHAR *, int);   /* Comms read function */
    int (*txd)(struct k_data *, UCHAR *, int);   /* and comms write function */
    int (*ixd)(struct k_data *);	         /* and comms info function */
    int (*openf)(struct k_data *,UCHAR *,int);   /* open-file function  */
    ULONG (*finfo)(struct k_data *,UCHAR *,UCHAR *,int,short *,short);
    int (*readf)(struct k_data *);	         /* read-file function  */
    int (*writef)(struct k_data *,UCHAR *, int); /* write-file function */
    int (*closef)(struct k_data *,UCHAR,int);    /* close-file function */
    int (*dbf)(int,UCHAR *,UCHAR *,long);  /* debug function */
    UCHAR * zinbuf;			/* Input file buffer itself */
    int zincnt;				/* Input buffer position */
    int zinlen;				/* Length of input file buffer */
    UCHAR * zinptr;			/* Pointer to input file buffer */
    int bctf;				/* Flag to force type 3 block check */
    int dummy;
};

struct k_response {			/* Report from Kermit */
    short status;                       /* Current status */
    UCHAR filename[FN_MAX];             /* Name of current file */
    UCHAR filedate[DATE_MAX];           /* Date of file */
    long filesize;                      /* Size of file */
    long sofar;				/* Bytes transferred so far */
};

/* Macro definitions */

#define tochar(ch)  (UCHAR)((UCHAR)((UCHAR)(ch) + SP ))
#define xunchar(ch) (UCHAR)((UCHAR)((UCHAR)(ch) - SP ))
#define ctl(ch)     (UCHAR)((UCHAR)((UCHAR)(ch) ^ 64 ))

#ifdef COMMENT
#define tochar(ch)  (((ch) + SP ) & 0xFF ) /* Digit to character */
#define xunchar(ch) (((ch) - SP ) & 0xFF ) /* Character to number */
#define ctl(ch)     (((ch) ^ 64 ) & 0xFF ) /* Controllify/uncontrollify */
#endif /* COMMENT */

/* Prototypes for kermit() functions */

int kermit(short, struct k_data *, short, int, char *, struct k_response *);
UCHAR * getrslot(struct k_data *, short *);
UCHAR * getsslot(struct k_data *, short *);
void freerslot(struct k_data *, short);
void freesslot(struct k_data *, short);

#endif /* __KERMIT_H__ */
