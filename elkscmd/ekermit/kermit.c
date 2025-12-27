#define KERMIT_C
/*
  Embedded Kermit protocol module
  Version: 1.8
  Most Recent Update: Tue May 25 19:20:27 2021

  No stdio or other runtime library calls, no system calls, no system
  includes, no static data, and no global variables in this module.

  Warning: you can't use debug() in any routine whose argument list
  does not include "struct k_data * k".  Thus most routines in this
  module include this arg, even if they don't use it.

  Author: Frank da Cruz.
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
#include "cdefs.h"			/* C language defs for all modules */
#include "debug.h"			/* Debugging */
#include "kermit.h"			/* Kermit protocol definitions */

#define zgetc() \
((--(k->zincnt))>=0)?((int)(*(k->zinptr)++)&0xff):(*(k->readf))(k)

/* See cdefs.h for meaning of STATIC, ULONG, and UCHAR */

STATIC ULONG stringnum(UCHAR *, struct k_data *);
STATIC UCHAR * numstring(ULONG, UCHAR *, int, struct k_data *);
int STATIC spkt(char, short, int, UCHAR *, struct k_data *);
int STATIC ack(struct k_data *, short, UCHAR * text);
int STATIC nak(struct k_data *, short, short);
int STATIC chk1(UCHAR *, struct k_data *);
STATIC USHORT chk2(UCHAR *, struct k_data *);
#ifdef F_CRC
STATIC USHORT chk3(UCHAR *, struct k_data *);
#endif /* F_CRC */
void STATIC spar(struct k_data *, UCHAR *, int);
int STATIC rpar(struct k_data *, char);
int STATIC decode(struct k_data *, struct k_response *, short, UCHAR *);
#ifdef F_AT
int STATIC gattr(struct k_data *, UCHAR *, struct k_response *);
int STATIC sattr(struct k_data *, struct k_response *);
#endif /* F_AT */
#ifndef RECVONLY
int STATIC sdata(struct k_data *, struct k_response *);
#endif /* RECVONLY */
void STATIC epkt(char *, struct k_data *);
int STATIC getpkt(struct k_data *, struct k_response *);
int STATIC encstr(UCHAR *, struct k_data *, struct k_response *);
void STATIC decstr(UCHAR *, struct k_data *, struct k_response *);
void STATIC encode(int, int, struct k_data *);
int STATIC nxtpkt(struct k_data *);
int STATIC resend(struct k_data *);
#ifdef DEBUG
int xerror(void);
#endif /* DEBUG */

int					/* The kermit() function */
kermit(short f,				/* Function code */
       struct k_data *k,		/* The control struct */
       short r_slot,			/* Received packet slot number */
       int len,				/* Length of packet in slot */
       char *msg,			/* Message for error packet */
       struct k_response *r) {		/* Response struct */

    int i, j, rc;			/* Workers */
    int datalen;                        /* Length of packet data field */
    UCHAR *p;                           /* Pointer to packet data field */
    UCHAR *q;                           /* Pointer to data to be checked */
    UCHAR *s;				/* Worker string pointer */
    UCHAR c, t;                         /* Worker chars */
    UCHAR pbc[4];                       /* Copy of packet block check */
    short seq, prev;			/* Copies of sequence numbers */
    short chklen;                       /* Length of packet block check */
#ifdef F_CRC
    unsigned int crc;                   /* 16-bit CRC */
#endif /* F_CRC */
    int ok;

    debug(DB_MSG,"----------",0,0);	/* Marks each entry */
    debug(DB_LOG,"f",0,f);
    debug(DB_LOG,"state",0,k->state);
    debug(DB_LOG,"zincnt",0,(k->zincnt));

    if (f == K_INIT) {			/* Initialize packet buffers etc */

	k->version = (UCHAR *)VERSION;	/* Version of this module */
        r->filename[0] = '\0';		/* No filename yet. */
        r->filedate[0] = '\0';		/* No filedate yet. */
        r->filesize = 0L;               /* No filesize yet. */
	r->sofar = 0L;			/* No bytes transferred yet */

        for (i = 0; i < P_WSLOTS; i++) { /* Packet info for each window slot */
	    freerslot(k,i);
	    freesslot(k,i);
	}
#ifdef F_TSW
        for (i = 0; i < 64; i++) {	/* Packet finder array */
	    k->r_pw[i] = -1;		/* initialized to "no packets yet" */
	    k->s_pw[i] = -1;		/* initialized to "no packets yet" */
	}
#endif /* F_TSW */

/* Initialize the k_data structure */

	for (i = 0; i < 6; i++)
	  k->s_remain[i] = '\0';

        k->state    = R_WAIT;		/* Beginning protocol state */
	r->status   = R_WAIT;
	k->what     = W_RECV;		/* Default action */
	k->s_first  = 1;		/* Beginning of file */
        k->r_soh    = k->s_soh = SOH;	/* Packet start */
        k->r_eom    = k->s_eom = CR;	/* Packet end */
        k->s_seq    = k->r_seq =  0;	/* Packet sequence number */
        k->s_type   = k->r_type = 0;	/* Packet type */
        k->r_timo   = P_R_TIMO;		/* Timeout interval for me to use */
        k->s_timo   = P_S_TIMO;		/* Timeout for other Kermit to use */
        k->r_maxlen = P_PKTLEN;         /* Maximum packet length */
        k->s_maxlen = P_PKTLEN;         /* Maximum packet length */
        k->window   = P_WSLOTS;		/* Maximum window slots */
        k->wslots   = 1;		/* Current window slots */
	k->zincnt   = 0;
	k->dummy    = 0;
	k->filename = (UCHAR *)0;

        /* Parity must be filled in by the caller */

        k->retry  = P_RETRY;            /* Retransmission limit */
        k->s_ctlq = k->r_ctlq = '#';    /* Control prefix */
        k->ebq    = 'Y';		/* 8th-bit prefix negotiation */
	k->ebqflg = 0;			/* 8th-bit prefixing flag */
        k->rptq   = '~';		/* Send repeat prefix */
        k->rptflg = 0;                  /* Repeat counts negotiated */
	k->s_rpt  = 0;			/* Current repeat count */
        k->capas  = 0                   /* Capabilities */
#ifdef F_LP
          | CAP_LP                      /* Long packets */
#endif /* F_LP */
#ifdef F_SW
            | CAP_SW                    /* Sliding windows */
#endif /* F_SW */
#ifdef F_AT
              | CAP_AT                  /* Attribute packets */
#endif /* F_AT */
                ;

	k->opktbuf[0] = '\0';		/* No packets sent yet. */
	k->opktlen = 0;

#ifdef F_CRC
/* This is the only way to initialize these tables -- no static data. */

        k->crcta[ 0] =       0;		/* CRC generation table A */
        k->crcta[ 1] =  010201;
        k->crcta[ 2] =  020402;
        k->crcta[ 3] =  030603;
        k->crcta[ 4] =  041004;
        k->crcta[ 5] =  051205;
        k->crcta[ 6] =  061406;
        k->crcta[ 7] =  071607;
        k->crcta[ 8] = 0102010;
        k->crcta[ 9] = 0112211;
        k->crcta[10] = 0122412;
        k->crcta[11] = 0132613;
        k->crcta[12] = 0143014,
        k->crcta[13] = 0153215;
        k->crcta[14] = 0163416;
        k->crcta[15] = 0173617;

        k->crctb[ 0] =       0;        /* CRC table B */
        k->crctb[ 1] =  010611;
        k->crctb[ 2] =  021422;
        k->crctb[ 3] =  031233;
        k->crctb[ 4] =  043044;
        k->crctb[ 5] =  053655;
        k->crctb[ 6] =  062466;
        k->crctb[ 7] =  072277;
        k->crctb[ 8] = 0106110;
        k->crctb[ 9] = 0116701;
        k->crctb[10] = 0127532;
        k->crctb[11] = 0137323;
        k->crctb[12] = 0145154;
        k->crctb[13] = 0155745;
        k->crctb[14] = 0164576;
        k->crctb[15] = 0174367;
#endif /* F_CRC */

	return(X_OK);

#ifndef RECVONLY
    } else if (f == K_SEND) {
	if (rpar(k,'S') != X_OK)	/* Send S packet with my parameters */
	  return(X_ERROR);		/* I/O error, quit. */
	k->state = S_INIT;		/* All OK, switch states */
	r->status = S_INIT;
	k->what = W_SEND;		/* Act like a sender */
        return(X_OK);
#endif /* RECVONLY */

    } else if (f == K_STATUS) {         /* Status report requested. */
        return(X_STATUS);               /* File name, date, size, if any. */

    } else if (f == K_QUIT) {           /* You told me to quit */
        return(X_DONE);                 /* so I quit. */

    } else if (f == K_ERROR) {          /* Send an error packet... */
        epkt(msg,k);
	k->closef(k,0,(k->state == S_DATA) ? 1 : 2); /* Close file */
        return(X_DONE);                 /* and quit. */

    } else if (f != K_RUN) {            /* Anything else is an error. */
        return(X_ERROR);
    }
    if (k->state == R_NONE)             /* (probably unnecessary) */
      return(X_OK);

/* If we're in the protocol, check to make sure we got a new packet */

    debug(DB_LOG,"r_slot",0,r_slot);
    debug(DB_LOG,"len",0,len);

    if (r_slot < 0)			/* We should have a slot here */
      return(K_ERROR);
    else
      k->ipktinfo[r_slot].len = len;	/* Copy packet length to ipktinfo. */

    if (len < 4) {			/* Packet obviously no good? */
#ifdef RECVONLY
	return(nak(k,k->r_seq,r_slot)); /* Send NAK for the packet we want */
#else
	if (k->what == W_RECV)		/* If receiving */
	  return(nak(k,k->r_seq,r_slot)); /* Send NAK for the packet we want */
	else				/* If sending */
	  return(resend(k));		/* retransmit last packet. */
#endif /* RECVONLY */
    }

/* Parse the packet */

    if (k->what == W_RECV) {		/* If we're sending ACKs */
	switch(k->cancel) {		/* Get cancellation code if any */
	  case 0: s = (UCHAR *)0;   break;
	  case 1: s = (UCHAR *)"X"; break;
	  case 2: s = (UCHAR *)"Z"; break;
	}
    }
    p = &(k->ipktbuf[0][r_slot]);	/* Point to it */

    q = p;                              /* Pointer to data to be checked */
    k->ipktinfo[r_slot].len = xunchar(*p++); /* Length field */
    seq = k->ipktinfo[r_slot].seq = xunchar(*p++); /* Sequence number */
    t = k->ipktinfo[r_slot].typ = *p++;	/* Type */

    if (
#ifndef RECVONLY
	(k->what == W_RECV) &&		/* Echo (it happens), ignore */
#endif /* RECVONLY */
	(t == 'N' || t  == 'Y')) {
        freerslot(k,r_slot);
        return(X_OK);
    }
    k->ipktinfo[r_slot].dat = p;	/* Data field, maybe */
#ifdef F_LP
    if (k->ipktinfo[r_slot].len == 0) {	/* Length 0 means long packet */
        c = p[2];                       /* Get header checksum */
        p[2] = '\0';
        if (xunchar(c) != chk1(p-3,k)) {  /* Check it */
            freerslot(k,r_slot);	/* Bad */
	    debug(DB_MSG,"HDR CHKSUM BAD",0,0);
#ifdef RECVONLY
	    return(nak(k,k->r_seq,r_slot)); /* Send NAK */
#else
	    if (k->what == W_RECV)
	      return(nak(k,k->r_seq,r_slot)); /* Send NAK */
	    else
	      return(resend(k));
#endif /* RECVONLY */
        }
	debug(DB_MSG,"HDR CHKSUM OK",0,0);
        p[2] = c;                       /* Put checksum back */
	/* Data length */
        datalen = xunchar(p[0])*95 + xunchar(p[1]) - ((k->bctf) ? 3 : k->bct);
        p += 3;                         /* Fix data pointer */
        k->ipktinfo[r_slot].dat = p;	/* Permanent record of data pointer */
    } else {                            /* Regular packet */
#endif /* F_LP */
        datalen = k->ipktinfo[r_slot].len - k->bct - 2; /* Data length */
#ifdef F_LP
    }
#endif /* F_LP */
#ifdef F_CRC
    if (k->bctf) {			/* FORCE 3 */
	chklen = 3;
    } else {
	if (t == 'S' || k->state == S_INIT) { /* S-packet was retransmitted? */
	    if (q[10] == '5') {		/* Block check type requested is 5 */
		k->bctf = 1;		/* FORCE 3 */
		chklen = 3;
	    }
	    chklen = 1;			/* Block check is always type 1 */
	    datalen = k->ipktinfo[r_slot].len - 3; /* Data length */
	} else {
	    chklen = k->bct;
	}
    }
#else
    chklen = 1;				/* Block check is always type 1 */
    datalen = k->ipktinfo[r_slot].len - 3; /* Data length */
#endif /* F_CRC */
    debug(DB_LOG,"bct",0,(k->bct));
    debug(DB_LOG,"datalen",0,datalen);
    debug(DB_LOG,"chkalen",0,chklen);

#ifdef F_CRC
    for (i = 0; i < chklen; i++)        /* Copy the block check */
      pbc[i] = p[datalen+i];
    pbc[i] = '\0';			/* Null-terminate block check string */
#else
    pbc[0] = p[datalen];
    pbc[1] = '\0';
#endif /* F_CRC */
    p[datalen] = '\0';			/* and the packet DATA field. */
#ifdef F_CRC
    switch (chklen) {                   /* Check the block check  */
      case 1:				/* Type 1, 6-bit checksum */
#endif /* F_CRC */
	ok = (xunchar(*pbc) == chk1(q,k));
#ifdef DEBUG
	if (ok && xerror()) ok = 0;
#endif /* DEBUG */
	if (!ok) {
	    freerslot(k,r_slot);
#ifdef RECVONLY
	    nak(k,k->r_seq,r_slot);
#else
	    if (k->what == W_RECV)
	      nak(k,k->r_seq,r_slot);
	    else
	      resend(k);
#endif /* RECVONLY */
	    return(X_OK);
	}
#ifdef F_CRC
	break;

      case 2:                         /* Type 2, 12-bit checksum */
	i = xunchar(*pbc) << 6 | xunchar(pbc[1]);
	ok = (i == chk2(q,k));
#ifdef DEBUG
	if (ok && xerror()) ok = 0;
#endif /* DEBUG */
	if (!ok) {			/* No match */
	    if (t == 'E') {		/* Allow E packets to have type 1 */
		int j;
		j = datalen;
		p[j++] = pbc[0];
		p[j] = '\0';
		if (xunchar(pbc[1]) == chk1(q,k))
		  break;
		else
		  p[--j] = '\0';
	    }
	    freerslot(k,r_slot);
#ifdef RECVONLY
	    nak(k,k->r_seq,r_slot);
#else
	    if (k->what == W_RECV)
	      nak(k,k->r_seq,r_slot);
	    else
	      resend(k);
#endif /* RECVONLY */
	    return(X_OK);
	}
	break;

      case 3:				/* Type 3, 16-bit CRC */
	crc = (xunchar(pbc[0]) << 12)
	  | (xunchar(pbc[1]) << 6)
	    | (xunchar(pbc[2]));
	ok = (crc == chk3(q,k));
#ifdef DEBUG
	if (ok && xerror()) {
	    ok = 0;
	    debug(DB_MSG,"CRC ERROR INJECTED",0,0);
	}
#endif /* DEBUG */
	if (!ok) {
	    debug(DB_LOG,"CRC ERROR t",0,t);
	    if (t == 'E') {		/* Allow E packets to have type 1 */
		int j;
		j = datalen;
		p[j++] = pbc[0];
		p[j++] = pbc[1];
		p[j] = '\0';
		if (xunchar(pbc[2]) == chk1(q,k))
		  break;
		else { j -=2; p[j] = '\0'; }
	    }
	    freerslot(k,r_slot);
#ifdef RECVONLY
	    nak(k,k->r_seq,r_slot);
#else
	    if (k->what == W_RECV)
	      nak(k,k->r_seq,r_slot);
	    else
	      resend(k);
#endif /* RECVONLY */
	    return(X_OK);
	}
    }
#endif /* F_CRC */
    if (t == 'E')			/* (AND CLOSE FILES?) */
      return(X_ERROR);

    prev = k->r_seq - 1;		/* Get sequence of previous packet */
    if (prev < 0)
      prev = 63;

    debug(DB_LOG,"Seq",0,seq);
    debug(DB_LOG,"Prev",0,prev);

    if (seq == k->r_seq) {		/* Is this the packet we want? */
	k->ipktinfo[r_slot].rtr = 0;	/* Yes */
    } else {
        freerslot(k,r_slot);		/* No, discard it. */

        if (seq == prev) {              /* If it's the previous packet again */
	    debug(DB_LOG,"PREVIOUS PKT RETRIES",0,
		  (long)(k->ipktinfo[r_slot].rtr));
            if (k->ipktinfo[r_slot].rtr++ > k->retry) { /* Count retries */
                epkt("Too many retries", k); /* Too may */
                return(X_ERROR);	/* Give up */
            } else {			/* Otherwise */
		return(resend(k));	/* Send old outbound packet buffer */
            }
#ifdef RECVONLY
	} else {
	    return(nak(k,k->r_seq,r_slot));
#else
        } else if (k->what == W_RECV) {	/* Otherwise NAK the one we want */
	    return(nak(k,k->r_seq,r_slot));
	} else {			/* or whatever... */
	    return(resend(k));
#endif /* RECVONLY */
	}
    }
#ifndef RECVONLY
    if (k->what == W_SEND) {		/* Sending, check for ACK */
	if (t != 'Y') {			/* Not an ACK */
	    debug(DB_LOG,"t!=Y t",0,t);
	    freerslot(k,r_slot);	/* added 2004-06-30 -- JHD */
	    return(resend(k));
	}
	if (k->state == S_DATA) {	/* ACK to Data packet?*/
	    if (k->cancel ||		/* Cancellation requested by caller? */
		*p == 'X' || *p == 'Z') { /* Or by receiver? */
		k->closef(k,*p,1);	  /* Close input file*/
		nxtpkt(k);		  /* Next packet sequence number */
		if ((rc = spkt('Z',k->s_seq,0,(UCHAR *)0,k)) != X_OK)
		  return(rc);
		if (*p == 'Z' || k->cancel == I_GROUP) { /* Cancel Group? */
		    debug(DB_MSG,"Group Cancel (Send)",0,0);
		    while (*(k->filelist)) { /* Go to end of file list */
			debug(DB_LOG,"Skip",*(k->filelist),0);
			(k->filelist)++;
		    }
		}
		k->state = S_EOF;	/* Wait for ACK to EOF */
		r->status = S_EOF;
		k->r_seq = k->s_seq;	/* Sequence number of packet we want */
		return(X_OK);
	    }
	}
	freerslot(k,r_slot);		/* It is, free the ACK. */
    }
#endif /* RECVONLY */

/* Now we have an incoming packet with the expected sequence number. */

    debug(DB_CHR,"Packet OK",0,t);
    debug(DB_LOG,"State",0,k->state);

    switch (k->state) {                 /* Kermit protocol state switcher */

#ifndef RECVONLY
      case S_INIT:			/* Got other Kermit's parameters */
      case S_EOF:			/* Got ACK to EOF packet */
	nxtpkt(k);			/* Get next packet number etc */
	if (k->state == S_INIT) {	/* Got ACK to S packet? */
	    spar(k,p,datalen);		/* Set negotiated parameters */
	    debug(DB_CHR,"Parity",0,k->parity);
	    debug(DB_LOG,"Ebqflg",0,(k->ebqflg));
	    debug(DB_CHR,"Ebq",0,(k->ebq));
	}
	k->filename = *(k->filelist);	/* Get next filename */
	if (k->filename) {		/* If there is one */
	    int i;
	    for (i = 0; i < FN_MAX; i++) { /* Copy name to result struct */
		r->filename[i] = k->filename[i];
		if (!(r->filename[i]))
		    break;
	    }
	    (k->filelist)++;
	    debug(DB_LOG,"Filename",k->filename,0);
	    if ((rc = (k->openf)(k,k->filename,1)) != X_OK) /* Try to open */
	      return(rc);
	    encstr(k->filename,k,r);	/* Encode the name for transmission */
	    if ((rc = spkt('F',k->s_seq,-1,k->xdata,k)) != X_OK)
	      return(rc);		/* Send F packet */
	    r->sofar = 0L;
	    k->state = S_FILE;		/* Wait for ACK */
	    r->status = S_FILE;
	} else {			/* No more files - we're done */
	    if ((rc = spkt('B',k->s_seq,0,(UCHAR *)0,k)) != X_OK)
	      return(rc);		/* Send EOT packet */
	    k->state = S_EOT;		/* Wait for ACK */
	    r->status = S_EOT;
	}
	k->r_seq = k->s_seq;		/* Sequence number of packet we want */
	return(X_OK);			/* Return to control program */

      case S_FILE:			/* Got ACK to F packet */
	nxtpkt(k);			/* Get next packet number etc */
#ifdef F_AT
	if (k->capas & CAP_AT) {	/* A-packets negotiated? */
	    if ((rc = sattr(k,r)) != X_OK) /* Yes, send Attribute packet */
	      return(rc);
	    k->state = S_ATTR;		/* And wait for its ACK */
	    r->status = S_ATTR;
	} else
#endif /* F_AT */
	  if (sdata(k,r) == 0) {	/* No A packets - send first data */
	    /* File is empty so send EOF packet */
	    if ((rc = spkt('Z',k->s_seq,0,(UCHAR *)0,k)) != X_OK)
	      return(rc);
	    k->closef(k,*p,1);		/* Close input file*/
	    k->state = S_EOF;		/* Wait for ACK to EOF */
	    r->status = S_EOF;
	} else {			/* Sent some data */
	    k->state = S_DATA;		/* Wait for ACK to first data */
	    r->status = S_DATA;
	}
	k->r_seq = k->s_seq;		/* Sequence number to wait for */
	return(X_OK);

      case S_ATTR:			/* Got ACK to A packet */
      case S_DATA:			/* Got ACK to D packet */
	nxtpkt(k);			/* Get next packet number */
	if (k->state == S_ATTR) {
	    /* CHECK ATTRIBUTE RESPONSE */
	    /* IF REJECTED do the right thing... */
	    k->state = S_DATA;
	    r->status = S_DATA;
	}
	rc = sdata(k,r);		/* Send first or next data packet */

	debug(DB_LOG,"Seq",0,(k->s_seq));
	debug(DB_LOG,"sdata()",0,rc);

	if (rc == 0) {			/* If there was no data to send */
	    if ((rc = spkt('Z',k->s_seq,0,(UCHAR *)0,k)) != X_OK)
	      return(rc);		/* Send EOF */
	    k->closef(k,*p,1);		/* Close input file*/
	    k->state = S_EOF;		/* And wait for ACK */
	    r->status = S_EOF;
	}				/* Otherwise stay in data state */
	k->r_seq = k->s_seq;		/* Sequence number to wait for */
	return(X_OK);

      case S_EOT:			/* Get ACK to EOT packet */
        return(X_DONE);			/* (or X_ERROR) */
#endif /* RECVONLY */

      case R_WAIT:                      /* Waiting for the S packet */
        if (t == 'S') {                 /* Got it */
            spar(k,p,datalen);          /* Set parameters from it */
            rc = rpar(k,'Y');		/* ACK with my parameters */
	    debug(DB_LOG,"rpar rc",0,rc);
            if (rc != X_OK)
              return(X_ERROR);          /* I/O error, quit. */
            k->state = R_FILE;          /* All OK, switch states */
            r->status = R_FILE;
        } else {                        /* Wrong kind of packet, send NAK */
            epkt("Unexpected packet type",k);
            rc = X_ERROR;
        }
        freerslot(k,r_slot);		/* Free packet slot */
        return(rc);

      case R_FILE:                      /* Want an F or B packet */
        if (t == 'F') {                 /* File name */
            if ((rc = decode(k, r, 0, p)) == X_OK) /* Decode and save */
              k->state = R_ATTR;        /* Switch to next state */
	    r->status = k->state;
	    debug(DB_LOG,"R_FILE decode rc",0,rc);
	    debug(DB_LOG,"R_FILE FILENAME",r->filename,0);
            if (rc == X_OK) {		/* All OK so far */
		r->filedate[0] = '\0';	/* No file date yet */
		r->filesize = 0L;	/* Or file size */
		r->sofar = 0L;		/* Or bytes transferred yet */
		rc = ack(k, k->r_seq, r->filename); /* so ACK the F packet */
	    } else {
		epkt("Filename error",k); /* Error decoding filename */
	    }
        } else if (t == 'B') {          /* Break, end of transaction */
            freerslot(k,r_slot);
            rc = (ack(k, k->r_seq, (UCHAR *)0) == X_OK) ? X_DONE : X_ERROR;

        } else {
            epkt("Unexpected packet type",k);
            rc = X_ERROR;
        }
        freerslot(k,r_slot);
        return(rc);

      case R_ATTR:                      /* Want A, D, or Z packet */
#ifdef F_AT
        if (t == 'A') {                 /* Attribute packet */
	    int x;
            x = gattr(k, p, r);		/* Read the attributes */
	    if (x > -1)
	      k->binary = x;
            freerslot(k,r_slot);
            ack(k, k->r_seq, (UCHAR *) "Y"); /* Always accept the file */
            return(X_OK);
        } else
#endif /* F_AT */
	  if (t == 'D') {		/* First data packet */
            k->obufpos = 0;             /* Initialize output buffer */
	    k->filename = r->filename;
	    r->sofar = 0L;
            if ((rc = (*(k->openf))(k,r->filename, 2)) == X_OK) {
                k->state = R_DATA;      /* Switch to Data state */
		r->status = k->state;
                rc = decode(k, r, 1, p); /* Write out first data packet */
                freerslot(k,r_slot);
            } else {
                epkt("File refused or can't be opened", k);
                freerslot(k,r_slot);
                return(rc);
            }
            if (rc == X_OK)
              rc = ack(k, k->r_seq, s);
            else
              epkt("Error writing data", k);
            return(rc);
	} else if (t == 'Z') {		/* Empty file */
	    debug(DB_LOG,"R_ATTR empty file",r->filename,0);
            k->obufpos = 0;             /* Initialize output buffer */
	    k->filename = r->filename;
	    r->sofar = 0L;		/* Open and close the file */
            if ((rc = (*(k->openf))(k,r->filename, 2)) == X_OK) {
		if (((rc = (*(k->closef))(k,*p,2)) == X_OK)) {
		    k->state = R_FILE;
		    rc = ack(k, k->r_seq, s);
		} else {
		    epkt("Error closing empty file", k);
		    freerslot(k,r_slot);
		    return(rc);
		}
            } else {
                epkt("File refused or can't be opened", k);
                freerslot(k,r_slot);
                return(rc);
            }
	    r->status = k->state;
            freerslot(k,r_slot);

        } else {
            epkt("Unexpected packet type",k);
            return(X_ERROR);
        }
        break;

      case R_DATA:                      /* Want a D or Z packet */
	debug(DB_CHR,"R_DATA t",0,t);
        if (t == 'D') {                 /* Data */
            rc = decode(k, r, 1, p);	/* Decode it */
            freerslot(k,r_slot);
        } else if (t == 'Z') {          /* End of file */
	    debug(DB_CHR,"R_DATA",0,t);
            if (k->obufpos > 0) {       /* Flush output buffer */
                rc = (*(k->writef))(k,k->obuf,k->obufpos);
		debug(DB_LOG,"R_DATA writef rc",0,rc);
		r->sofar += k->obufpos;
                k->obufpos = 0;
            }
            if (((rc = (*(k->closef))(k,*p,2)) == X_OK) && (rc == X_OK))
              k->state = R_FILE;
	    debug(DB_LOG,"R_DATA closef rc",0,rc);
	    r->status = k->state;
            freerslot(k,r_slot);
        } else {
            epkt("Unexpected packet type",k);
            return(X_ERROR);
        }
        if (rc == X_OK)
          rc = ack(k, k->r_seq, s);
        else
          epkt(t == 'Z' ? "Can't close file" : "Error writing data",k);
        return(rc);

      case R_ERROR:                     /* Canceled from above */
      default:
        epkt(msg,k);
        return(X_ERROR);
    }
    return(X_ERROR);
}

/* Utility routines */

UCHAR *
getrslot(struct k_data *k, short *n) {   /* Find a free packet buffer */
    register int i;
/*
  Note: We don't clear the retry count here.
  It is cleared only after the NEXT packet arrives, which
  indicates that the other Kermit got our ACK for THIS packet.
*/
    for (i = 0; i < P_WSLOTS; i++) {    /* Search */
        if (k->ipktinfo[i].len < 1) {
            *n = i;                     /* Slot number */
            k->ipktinfo[i].len = -1;	/* Mark it as allocated but not used */
            k->ipktinfo[i].seq = -1;
            k->ipktinfo[i].typ = SP;
            /* k->ipktinfo[i].rtr =  0; */  /* (see comment above) */
            k->ipktinfo[i].dat = (UCHAR *)0;
            return(&(k->ipktbuf[0][i]));
        }
    }
    *n = -1;
    return((UCHAR *)0);
}

void					/* Initialize a window slot */
freerslot(struct k_data *k, short n) {
    k->ipktinfo[n].len = 0;		/* Packet length */
#ifdef COMMENT
    k->ipktinfo[n].seq = 0;		/* Sequence number */
    k->ipktinfo[n].typ = (char)0;	/* Type */
    k->ipktinfo[n].rtr = 0;		/* Retry count */
    k->ipktinfo[n].flg = 0;		/* Flags */
#endif /* COMMENT */
}

UCHAR *
getsslot(struct k_data *k, short *n) {   /* Find a free packet buffer */
#ifdef COMMENT
    register int i;
    for (i = 0; i < P_WSLOTS; i++) {    /* Search */
        if (k->opktinfo[i].len < 1) {
            *n = i;                     /* Slot number */
            k->opktinfo[i].len = -1;	/* Mark it as allocated but not used */
            k->opktinfo[i].seq = -1;
            k->opktinfo[i].typ = SP;
            k->opktinfo[i].rtr =  0;
            k->opktinfo[i].dat = (UCHAR *)0;
            return(&(k->opktbuf[0][i]));
        }
    }
    *n = -1;
    return((UCHAR *)0);
#else
    *n = 0;
    return(k->opktbuf);
#endif /* COMMENT */
}

void                                    /* Initialize a window slot */
freesslot(struct k_data * k, short n) {
    k->opktinfo[n].len = 0;		/* Packet length */
    k->opktinfo[n].seq = 0;		/* Sequence number */
    k->opktinfo[n].typ = (char)0;	/* Type */
    k->opktinfo[n].rtr = 0;		/* Retry count */
    k->opktinfo[n].flg = 0;		/* Flags */
}

/*  C H K 1  --  Compute a type-1 Kermit 6-bit checksum.  */

STATIC int
chk1(UCHAR *pkt, struct k_data * k) {
    register unsigned int chk;
    chk = chk2(pkt,k);
    chk = (((chk & 0300) >> 6) + chk) & 077;
    return((int) chk);
}

/*  C H K 2  --  Numeric sum of all the bytes in the packet, 12 bits.  */

STATIC USHORT
chk2(UCHAR *pkt,struct k_data * k) {
    register USHORT chk;
    for (chk = 0; *pkt != '\0'; pkt++)
      chk += *pkt;
    return(chk);
}

#ifdef F_CRC

/*  C H K 3  --  Compute a type-3 Kermit block check.  */
/*
 Calculate the 16-bit CRC-CCITT of a null-terminated string using a lookup
 table.  Assumes the argument string contains no embedded nulls.
*/
STATIC USHORT
chk3(UCHAR *pkt, struct k_data * k) {
    register USHORT c, crc;
    for (crc = 0; *pkt != '\0'; pkt++) {
#ifdef COMMENT
        c = crc ^ (long)(*pkt);
        crc = (crc >> 8) ^ (k->crcta[(c & 0xF0) >> 4] ^ k->crctb[c & 0x0F]);
#else
	c = crc ^ (*pkt);
       crc = (crc >> 8) ^ ((k->crcta[(c & 0xF0) >> 4]) ^ (k->crctb[c & 0x0F]));
#endif	/*  COMMENT */
    }
    return(crc);
}
#endif /* F_CRC */

/*   S P K T  --  Send a packet.  */
/*
  Call with packet type, sequence number, data length, data, Kermit struct.
  Returns:
    X_OK on success
    X_ERROR on i/o error
*/
STATIC int
spkt(char typ, short seq, int len, UCHAR * data, struct k_data * k) {

    unsigned int crc;                   /* For building CRC */
    int i, j, lenpos, m, n, x;		/* Workers */
    UCHAR * s, * buf;

    debug(DB_LOG,"spkt len 1",0,len);
    if (len < 0) {			/* Calculate data length ourselves? */
	len = 0;
	s = data;
	while (*s++) len++;
    }
    debug(DB_LOG,"spkt len 2",0,len);
    buf = k->opktbuf;			/* Where to put packet (FOR NOW) */

    i = 0;                              /* Packet buffer position */
    buf[i++] = k->s_soh;		/* SOH */
    lenpos = i++;			/* Remember this place */
    buf[i++] = tochar(seq);		/* Sequence number */
    buf[i++] = typ;			/* Packet type */
    j = len + k->bct;
#ifdef F_LP
    if ((len + k->bct + 2) > 94) {	/* If long packet */
	buf[lenpos] = tochar(0);	/* Put blank in LEN field */
	buf[i++] = tochar(j / 95);	/* Make extended header: Big part */
	buf[i++] = tochar(j % 95);	/* and small part of length. */
        buf[i] = NUL;			/* Terminate for header checksum */
        buf[i++] = tochar(chk1(&buf[lenpos],k)); /* Insert header checksum */
    } else {				/* Short packet */
#endif /* F_LP */
	buf[lenpos] = tochar(j+2);	/* Single-byte length in LEN field */
#ifdef F_LP
    }
#endif /* F_LP */
    if (data)                           /* Copy data, if any */
      for ( ; len--; i++)
        buf[i] = *data++;
    buf[i] = '\0';

#ifdef F_CRC
    switch (k->bct) {                   /* Add block check */
      case 1:                           /* 1 = 6-bit chksum */
	buf[i++] = tochar(chk1(&buf[lenpos],k));
        break;
      case 2:                           /* 2 = 12-bit chksum */
        j = chk2(&buf[lenpos],k);
#ifdef XAC
	/* HiTech's XAC compiler silently ruins the regular code. */
	/* An intermediate variable provides a work-around. */
	/* 2004-06-29 -- JHD */
	{
	    USHORT jj;
	    jj = (j >> 6) & 077; buf[i++] = tochar(jj);
	    jj = j & 077;        buf[i++] = tochar(jj);
	}
#else
        buf[i++] = (unsigned)tochar((j >> 6) & 077);
        buf[i++] = (unsigned)tochar(j & 077);
#endif /* XAC */
        break;
      case 3:                           /* 3 = 16-bit CRC */
        crc = chk3(&buf[lenpos],k);
#ifdef XAC
	/* HiTech's XAC compiler silently ruins the above code. */
	/* An intermediate variable provides a work-around. */
	/* 2004-06-29 -- JHD */
	{
	    USHORT jj;
	    jj = (crc >> 12) & 0x0f; buf[i++] = tochar(jj);
	    jj = (crc >>  6) & 0x3f; buf[i++] = tochar(jj);
	    jj =  crc        & 0x3f; buf[i++] = tochar(jj);
	}
#else
        buf[i++] = (unsigned)tochar(((crc & 0170000)) >> 12);
        buf[i++] = (unsigned)tochar((crc >> 6) & 077);
        buf[i++] = (unsigned)tochar(crc & 077);
#endif /* XAC */
        break;
    }
#else
    buf[i++] = tochar(chk1(&buf[lenpos],k));
#endif /* F_CRC */

    buf[i++] = k->s_eom;		/* Packet terminator */
    buf[i] = '\0';			/* String terminator */
    k->s_seq = seq;                     /* Remember sequence number */

    k->opktlen = i;			/* Remember length for retransmit */

#ifdef DEBUG
/* CORRUPT THE PACKET SENT BUT NOT THE ONE WE SAVE */
    if (xerror()) {
	UCHAR p[P_PKTLEN+8];
	int i;
	for (i = 0; i < P_PKTLEN; i++)
	  if (!(p[i] = buf[i]))
	    break;
	p[i-2] = 'X';
	debug(DB_PKT,"XPKT",(char *)&p[1],0);
	return((*(k->txd))(k,p,k->opktlen)); /* Send it. */
    }
    debug(DB_PKT,"SPKT",(char *)&buf[1],0);
#endif /* DEBUG */

    return((*(k->txd))(k,buf,k->opktlen)); /* Send it. */
}

/*  N A K  --  Send a NAK (negative acknowledgement)  */

STATIC int
nak(struct k_data * k, short seq, short slot) {
    int rc;
    rc = spkt('N', seq, 0, (UCHAR *)0, k);
    if (k->ipktinfo[slot].rtr++ > k->retry)
      rc = X_ERROR;
    return(rc);
}

/*  A C K  --  Send an ACK (positive acknowledgement)  */

STATIC int
ack(struct k_data * k, short seq, UCHAR * text) {
    int len, rc;
    len = 0;
    if (text) {                         /* Get length of data */
        UCHAR *p;
        p = text;
        for ( ; *p++; len++) ;
    }
    rc = spkt('Y', seq, len, text, k);  /* Send the packet */
    debug(DB_LOG,"ack spkt rc",0,rc);
    if (rc == X_OK)                     /* If OK */
      k->r_seq = (k->r_seq + 1) % 64;   /* bump the packet number */
    return(rc);
}

/*  S P A R  --  Set parameters requested by other Kermit  */

STATIC void
spar(struct k_data * k, UCHAR *s, int datalen) {
    int x, y;
    UCHAR c;

    s--;                                /* Line up with field numbers. */

    if (datalen >= 1)                   /* Max packet length to send */
      k->s_maxlen = xunchar(s[1]);

    if (datalen >= 2)                   /* Timeout on inbound packets */
      k->r_timo = xunchar(s[2]);

    /* No padding */

    if (datalen >= 5)                   /* Outbound Packet Terminator */
      k->s_eom = xunchar(s[5]);

    if (datalen >= 6)                   /* Incoming control prefix */
      k->r_ctlq = s[6];

    if (datalen >= 7) {                 /* 8th bit prefix */
        k->ebq = s[7];
        if ((s[7] > 32 && s[7] < 63) || (s[7] > 95 && s[7] < 127)) {
	    if (!k->parity)		/* They want it */
	      k->parity = 1;		/* Set parity to something nonzero */
	    k->ebqflg = 1;
	} else if (s[7] == 'Y' && k->parity) {
	    k->ebqflg = 1;
	    k->ebq = '&';
	} else if (s[7] == 'N') {
	    /* WHAT? */
	}
    }
    if (datalen >= 8) {                 /* Block check */
        k->bct = s[8] - '0';
#ifdef F_CRC
        if ((k->bct < 1) || (k->bct > 3))
#endif /* F_CRC */
	  k->bct = 1;
	if (k->bctf) k->bct = 3;
    }
    if (datalen >= 9) {                 /* Repeat counts */
        if ((s[9] > 32 && s[9] < 63) || (s[9] > 95 && s[9] < 127)) {
            k->rptq = s[9];
            k->rptflg = 1;
        }
    }
    if (datalen >= 10) {                /* Capability bits */
        x = xunchar(s[10]);

#ifdef F_LP                             /* Long packets */
        if (!(x & CAP_LP))
#endif /* F_LP */
          k->capas &= ~CAP_LP;

#ifdef F_SW                             /* Sliding Windows */
        if (!(x & CAP_SW))
#endif /* F_SW */
          k->capas &= ~CAP_SW;

#ifdef F_AT                             /* Attributes */
        if (!(x & CAP_AT))
#endif /* F_AT */
          k->capas &= ~CAP_AT;

#ifdef F_RS                             /* Recovery */
        if (!(x & CAP_RS))
#endif /* F_RS */
          k->capas &= ~CAP_RS;

#ifdef F_LS                             /* Locking shifts */
        if (!(x & CAP_LS))
#endif /* F_LS */
          k->capas &= ~CAP_LS;

        /* In case other Kermit sends addt'l capas fields ... */

        for (y = 10; (xunchar(s[y]) & 1) && (datalen >= y); y++) ;
    }

#ifdef F_LP                             /* Long Packets */
    if (k->capas & CAP_LP) {
        if (datalen > y+1) {
            x = xunchar(s[y+2]) * 95 + xunchar(s[y+3]);
            k->s_maxlen = (x > P_PKTLEN) ? P_PKTLEN : x;
            if (k->s_maxlen < 10)
              k->s_maxlen = 60;
        }
    }
#endif /* F_LP */

    debug(DB_LOG,"S_MAXLEN",0,k->s_maxlen);

#ifdef F_SW
    if (k->capas & CAP_SW) {
        if (datalen > y) {
            x = xunchar(s[y+1]);
            k->window = (x > P_WSLOTS) ? P_WSLOTS : x;
            if (k->window < 1)          /* Watch out for bad negotiation */
              k->window = 1;
            if (k->window > 1)
              if (k->window > k->retry)   /* Retry limit must be greater */
                k->retry = k->window + 1; /* than window size. */
        }
    }
#endif /* F_SW */
}

/*  R P A R  --  Send my parameters to other Kermit  */

STATIC int
rpar(struct k_data * k, char type) {
    UCHAR *d;
    int rc, len;
#ifdef F_CRC
    short b;
#endif /* F_CRC */

    d = k->ack_s;                       /* Where to put it */
    d[ 0] = tochar(94);                 /* Maximum short-packet length */
    d[ 1] = tochar(k->s_timo);          /* When I want to be timed out */
    d[ 2] = tochar(0);                  /* How much padding I need */
    d[ 3] = ctl(0);                     /* Padding character I want */
    d[ 4] = tochar(k->r_eom);           /* End-of message character I want */
    d[ 5] = k->s_ctlq;                  /* Control prefix I send */
    if ((k->ebq == 'Y') && (k->parity)) /* 8th-bit prefix */
      d[ 6] = k->ebq = '&';           /* I need to request it */
    else                                /* else just agree with other Kermit */
      d[ 6] = k->ebq;
    if (k->bctf)			/* Block check type */
      d[7] = '5';			/* FORCE 3 */
    else
      d[7] = k->bct + '0';		/* Normal */
    d[ 8] = k->rptq;			/* Repeat prefix */
    d[ 9] = tochar(k->capas);           /* Capability bits */
    d[10] = tochar(k->window);          /* Window size */

#ifdef F_LP
    d[11] = tochar(k->r_maxlen / 95);   /* Long packet size, big part */
    d[12] = tochar(k->r_maxlen % 95);   /* Long packet size, little part */
    d[13] = '\0';			/* Terminate the init string */
    len = 13;
#else
    d[11] = '\0';
    len = 11;
#endif /* F_LP */

#ifdef F_CRC
    if (!(k->bctf)) {			/* Unless FORCE 3 */
	b = k->bct;
	k->bct = 1;			/* Always use block check type 1 */
    }
#endif /* F_CRC */
    switch (type) {
      case 'Y':				/* This is an ACK for packet 0 */
	rc = ack(k,0,d);
	break;
      case 'S':				/* It's an S packet */
	rc = spkt('S', 0, len, d, k);
	break;
      default:
	rc = -1;
    }
#ifdef F_CRC
    if (!(k->bctf)) {			/* Unless FORCE 3 */
	k->bct = b;
    }
#endif /* F_CRC */
    return(rc);                         /* Pass along return code. */
}

/*  D E C O D E  --  Decode data field of Kermit packet - binary mode only */
/*
  Call with:
    k = kermit data structure
    r = kermit response structure
    f = function code
      0 = decode filename
      1 = decode file data
    inbuf = pointer to packet data to be decoded
  Returns:
    X_OK on success
    X_ERROR if output function fails
*/
STATIC int
decode(struct k_data * k, struct k_response * r, short f, UCHAR *inbuf) {

    register unsigned int a, a7;        /* Current character */
    unsigned int b8;                    /* 8th bit */
    int rpt;                            /* Repeat count */
    int rc;				/* Return code */
    UCHAR *p;

    rc = X_OK;
    rpt = 0;                            /* Initialize repeat count. */
    if (f == 0)                         /* Output function... */
      p = r->filename;

    while ((a = *inbuf++ & 0xFF) != '\0') { /* Character loop */
        if (k->rptflg && a == k->rptq) { /* Got a repeat prefix? */
            rpt = xunchar(*inbuf++ & 0xFF); /* Yes, get the repeat count, */
            a = *inbuf++ & 0xFF;        /* and get the prefixed character. */
        }
        b8 = 0;                         /* 8th-bit value */
        if (k->parity && (a == k->ebq)) { /* Have 8th-bit prefix? */
            b8 = 0200;                  /* Yes, flag the 8th bit */
            a = *inbuf++ & 0x7F;        /* and get the prefixed character. */
        }
        if (a == k->r_ctlq) {           /* If control prefix, */
            a  = *inbuf++ & 0xFF;       /* get its operand */
            a7 = a & 0x7F;              /* and its low 7 bits. */
            if ((a7 >= 0100 && a7 <= 0137) || a7 == '?') /* Controllify */
              a = ctl(a);               /* if in control range. */
        }
        a |= b8;                        /* OR in the 8th bit */

        if (rpt == 0) rpt = 1;          /* If no repeats, then one */

        for (; rpt > 0; rpt--) {        /* Output the char 'rpt' times */
            if (f == 0) {
                *p++ = (UCHAR) a;       /* to memory */
            } else {                    /* or to file */
                k->obuf[k->obufpos++] = (UCHAR) a; /* Deposit the byte */
                if (k->obufpos == k->obuflen) { /* Buffer full? */
                    rc = (*(k->writef))(k,k->obuf,k->obuflen); /* Dump it. */
		    r->sofar += k->obuflen;
		    if (rc != X_OK) break;
                    k->obufpos = 0;
                }
            }
        }
    }
    if (f == 0)                         /* If writing to memory */
      *p = '\0';			/* terminate the string */
    return(rc);
}

STATIC ULONG				/* Convert decimal string to number  */
stringnum(UCHAR *s, struct k_data * k) {
    long n;
    n = 0L;
    while (*s == SP)
      s++;
    while(*s >= '0' && *s <= '9')
      n = n * 10 + (*s++ - '0');
    return(n);
}

STATIC UCHAR *				/* Convert number to string */
numstring(ULONG n, UCHAR * buf, int buflen, struct k_data * k) {
    int i, x;
    buf[buflen - 1] = '\0';
    for (i = buflen - 2; i > 0; i--) {
	x = n % 10L;
	buf[i] = x + '0';
	n /= 10L;
	if (!n)
	  break;
    }
    if (n) {
	return((UCHAR *)0);
    }
    if (i > 0) {
	UCHAR * p, * s;
	s = &buf[i];
	p = buf;
	while ((*p++ = *s++)) ;
	*(p-1) = '\0';
    }
    return((UCHAR *)buf);
}

#ifdef F_AT

/*
  G A T T R  --  Read incoming attributes.

  Returns:
   -1 if no transfer mode (text/binary) was announced.
    0 if text was announced.
    1 if binary was announced.
*/

#define SIZEBUFL 32                     /* For number conversions */

STATIC int
gattr(struct k_data * k, UCHAR * s, struct k_response * r) {
    long fsize, fsizek;                 /* File size */
    UCHAR c;                            /* Workers */
    int aln, i, rc;

    UCHAR sizebuf[SIZEBUFL];

    rc = -1;
    while ((c = *s++)) {		/* Get attribute tag */
        aln = xunchar(*s++);            /* Length of attribute string */
        switch (c) {
          case '!':                     /* File length in K */
	  case '"':			/* File type */
            for (i = 0; (i < aln) && (i < SIZEBUFL); i++) /* Copy it */
              sizebuf[i] = *s++;
            sizebuf[i] = '\0';           /* Terminate with null */
            if (i < aln) s += (aln - i); /* If field was too long for buffer */
	    if (c == '!') {		     /* Length */
		fsizek = stringnum(sizebuf,k); /* Convert to number */
	    } else {			     /* Type */
		if (sizebuf[0] == 'A')	     /* Text */
		  rc = 0;
		else if (sizebuf[0] == 'B')  /* Binary */
		  rc = 1;
		debug(DB_LOG,"gattr rc",0,rc);
		debug(DB_LOG,"gattr size",sizebuf,0);
	    }
            break;

          case '#':                     /* File creation date */
            for (i = 0; (i < aln) && (i < DATE_MAX); i++)
              r->filedate[i] = *s++;    /* Copy it into a static string */
            if (i < aln) s += (aln - i);
            r->filedate[i] = '\0';
            break;

          case '1':                     /* File length in bytes */
            for (i = 0; (i < aln) && (i < SIZEBUFL); i++) /* Copy it */
              sizebuf[i] = *s++;
            sizebuf[i] = '\0';           /* Terminate with null */
            if (i < aln) s += (aln - i);
            fsize = stringnum(sizebuf,k); /* Convert to number */
            break;

	  default:			/* Unknown attribute */
	    s += aln;			/* Just skip past it */
	    break;
        }
    }
    if (fsize > -1L) {			/* Remember the file size */
        r->filesize = fsize;
    } else if (fsizek > -1L) {
        r->filesize = fsizek * 1024L;
    }
    debug(DB_LOG,"gattr r->filesize",0,(r->filesize));
    debug(DB_LOG,"gattr r->filedate=",r->filedate,0);
    return(rc);
}

#define ATTRLEN 48

STATIC int
sattr(struct k_data *k, struct k_response *r) {	/* Build and send A packet */
    int i, x, aln;
    short tmp;
    long filelength;
    UCHAR datebuf[DATE_MAX], * p;

    debug(DB_LOG,"sattr k->zincnt 0",0,(k->zincnt));

    tmp = k->binary;
    filelength = (*(k->finfo))
	(k,k->filename,datebuf,DATE_MAX,&tmp,k->xfermode);
    k->binary = tmp;

    debug(DB_LOG,"sattr filename: ",k->filename,0);
    debug(DB_LOG,"sattr filedate: ",datebuf,0);
    debug(DB_LOG,"sattr filelength",0,filelength);
    debug(DB_LOG,"sattr binary",0,(k->binary));

    i = 0;

    k->xdata[i++] = '"';
    if (k->binary) {			/* Binary */
	k->xdata[i++] = tochar(2);	/*  Two characters */
	k->xdata[i++] = 'B';		/*  B for Binary */
	k->xdata[i++] = '8';		/*  8-bit bytes (note assumption...) */
    } else {				/* Text */
	k->xdata[i++] = tochar(3);	/*  Three characters */
	k->xdata[i++] = 'A';		/*  A = (extended) ASCII with CRLFs */
	k->xdata[i++] = 'M';		/*  M for carriage return */
	k->xdata[i++] = 'J';		/*  J for linefeed */
	k->xdata[i++] = '*';		/* Encoding */
	k->xdata[i++] = tochar(1);	/* Length of value is 1 */
	k->xdata[i++] = 'A';		/* A for ASCII */
    }
    if (filelength > -1L) {		/* File length in bytes */
	UCHAR lenbuf[16];
	r->filesize = filelength;
	p = numstring(filelength,lenbuf,16,k);
	if (p) {
	    for (x = 0; p[x]; x++) ;	/* Get length of length string */
	    if (i + x < ATTRLEN - 3) {	/* Don't overflow buffer */
		k->xdata[i++] = '1';	/* Length-in-Bytes attribute */
		k->xdata[i++] = tochar(x);
		while (*p)
		  k->xdata[i++] = *p++;
	    }
	}
    }
    debug(DB_LOG,"sattr DATEBUF: ",datebuf,0);

    if (datebuf[0]) {			/* File modtime */
	p = datebuf;
	for (x = 0; p[x]; x++) ;	/* Length of modtime */
	if (i + x < ATTRLEN - 3) {	/* If it will fit */
	    k->xdata[i++] = '#';	/* Add modtime attribute */
	    k->xdata[i++] = tochar(x);	/* Its length */
	    while (*p)			/* And itself */
	      k->xdata[i++] = *p++;
	    /* Also copy modtime to result struct */
	    for (x = 0; x < DATE_MAX-1 && datebuf[x]; x++)
	      r->filedate[x] = datebuf[x];
	    r->filedate[x] = '\0';
	}
    }
    k->xdata[i++] = '@';		/* End of Attributes */
    k->xdata[i++] = ' ';
    k->xdata[i] = '\0';			/* Terminate attribute string */
    debug(DB_LOG,"sattr k->xdata: ",k->xdata,0);
    return(spkt('A',k->s_seq,-1,k->xdata,k));
}
#endif /* F_AT */

STATIC int
getpkt(struct k_data *k, struct k_response *r) { /* Fill a packet from file */
    int i, next, rpt, maxlen;
    static int c;			/* PUT THIS IN STRUCT */

    debug(DB_LOG,"getpkt k->s_first",0,(k->s_first));
    debug(DB_LOG,"getpkt k->s_remain=",k->s_remain,0);

    maxlen = k->s_maxlen - k->bct - 3;	/* Maximum data length */
    if (k->s_first == 1) {		/* If first time thru...  */
	k->s_first = 0;			/* don't do this next time, */
	k->s_remain[0] = '\0';		/* discard any old leftovers. */
	if (k->istring) {		/* Get first byte. */
	    c = *(k->istring)++;	/* Of memory string... */
	    if (!c) c = -1;
	} else {			/* or file... */
#ifdef DEBUG
	    k->zincnt = -1234;
	    k->dummy = 0;
#endif /* DEBUG */
	    c = zgetc();

#ifdef DEBUG
	    if (k->dummy) debug(DB_LOG,"DUMMY CLOBBERED (A)",0,0);
#endif /* DEBUG */
	}
	if (c < 0) {			/* Watch out for empty file. */
	    debug(DB_CHR,"getpkt first c",0,c);
	    k->s_first = -1;
	    return(k->size = 0);
	}
	r->sofar++;
	debug(DB_LOG,"getpkt first c",0,c);
    } else if (k->s_first == -1 && !k->s_remain[0]) { /* EOF from last time? */
        return(k->size = 0);
    }
    for (k->size = 0;
	 (k->xdata[k->size] = k->s_remain[k->size]) != '\0';
	 (k->size)++)
      ;
    k->s_remain[0] = '\0';
    if (k->s_first == -1)
      return(k->size);

    rpt = 0;				/* Initialize repeat counter. */
    while (k->s_first > -1) {		/* Until end of file or string... */
	if (k->istring) {
	    next = *(k->istring)++;
	    if (!next) next = -1;
	} else {
#ifdef DEBUG
	    k->dummy = 0;
#endif /* DEBUG */
	    next = zgetc();
#ifdef DEBUG
	    if (k->dummy) debug(DB_LOG,"DUMMY CLOBBERED B",0,k->dummy);
#endif /* DEBUG */
	}
	if (next < 0) {			/* If none, we're at EOF. */
	    k->s_first = -1;
	} else {			/* Otherwise */
	    r->sofar++;			/* count this byte */
	}
        k->osize = k->size;		/* Remember current size. */
        encode(c,next,k);		/* Encode the character. */
	/* k->xdata[k->size] = '\0'; */
	c = next;			/* Old next char is now current. */

        if (k->size == maxlen)		/* Just at end, done. */
	  return(k->size);

        if (k->size > maxlen) {		/* Past end, must save some. */
            for (i = 0;
		 (k->s_remain[i] = k->xdata[(k->osize)+i]) != '\0';
		 i++)
	      ;
            k->size = k->osize;
            k->xdata[k->size] = '\0';
            return(k->size);		/* Return size. */
        }
    }
    return(k->size);			/* EOF, return size. */
}

#ifndef RECVONLY
STATIC int
sdata(struct k_data *k,struct k_response *r) { /* Send a data packet */
    int len, rc;
    if (k->cancel) {			/* Interrupted */
	debug(DB_LOG,"sdata interrupted k->cancel",0,(k->cancel));
	return(0);
    }
    len = getpkt(k,r);			/* Fill data field from input file */
    debug(DB_LOG,"sdata getpkt",0,len);
    if (len < 1)
      return(0);
    rc = spkt('D',k->s_seq,len,k->xdata,k); /* Send the packet */
    debug(DB_LOG,"sdata spkt",0,rc);
    return((rc == X_ERROR) ? rc : len);
}
#endif /* RECVONLY */

/*  E P K T  --  Send a (fatal) Error packet with the given message  */

STATIC void
epkt(char * msg, struct k_data * k) {
    if (!(k->bctf)) {			/* Unless FORCE 3 */
	k->bct = 1;
    }
    (void) spkt('E', 0, -1, (UCHAR *) msg, k);
}

STATIC int				/* Fill a packet from string s. */
encstr(UCHAR * s, struct k_data * k, struct k_response *r) {
    k->s_first = 1;			/* Start lookahead. */
    k->istring = s;			/* Set input string pointer */
    getpkt(k,r);			/* Fill a packet */
    k->istring = (UCHAR *)0;		/* Reset input string pointer */
    k->s_first = 1;			/* "Rewind" */
    return(k->size);			/* Return data field length */
}

/* Decode packet data into a string */

STATIC void
decstr(UCHAR * s, struct k_data * k, struct k_response * r) {
    k->ostring = s;			/* Set output string pointer  */
    (void) decode(k, r, 0, s);
    *(k->ostring) = '\0';		/* Terminate with null */
    k->ostring = (UCHAR *)0;		/* Reset output string pointer */
}

STATIC void
encode(int a, int next, struct k_data * k) { /* Encode character into packet */
    int a7, b8, maxlen;

    maxlen = k->s_maxlen - 4;
    if (k->rptflg) {			/* Doing run-length encoding? */
	if (a == next) {		/* Yes, got a run? */
	    if (++(k->s_rpt) < 94) {	/* Yes, count. */
		return;
	    } else if (k->s_rpt == 94) { /* If at maximum */
		k->xdata[(k->size)++] = k->rptq; /* Emit prefix, */
		k->xdata[(k->size)++] = tochar(k->s_rpt); /* and count, */
		k->s_rpt = 0;		/* and reset counter. */
	    }
	} else if (k->s_rpt == 1) {	/* Run broken, only two? */
	    k->s_rpt = 0;		/* Yes, do the character twice */
	    encode(a,-1,k);		/* by calling self recursively. */
	    if (k->size <= maxlen)	/* Watch boundary. */
	      k->osize = k->size;
	    k->s_rpt = 0;		/* Call self second time. */
	    encode(a,-1,k);
	    return;
	} else if (k->s_rpt > 1) {	/* Run broken, more than two? */
	    k->xdata[(k->size)++] = k->rptq; /* Yes, emit prefix and count */
	    k->xdata[(k->size)++] = tochar(++(k->s_rpt));
	    k->s_rpt = 0;		/* and reset counter. */
	}
    }
    a7 = a & 127;			/* Get low 7 bits of character */
    b8 = a & 128;			/* And "parity" bit */

    if (k->ebqflg && b8) {		/* If doing 8th bit prefixing */
	k->xdata[(k->size)++] = k->ebq; /* and 8th bit on, insert prefix */
	a = a7;				/* and clear the 8th bit. */
    }
    if (a7 < 32 || a7 == 127) {		   /* If in control range */
        k->xdata[(k->size)++] = k->s_ctlq; /* insert control prefix */
        a = ctl(a);			 /* and make character printable. */
    } else if (a7 == k->s_ctlq)		 /* If data is control prefix, */
      k->xdata[(k->size)++] = k->s_ctlq; /* prefix it. */
    else if (k->ebqflg && a7 == k->ebq)  /* If doing 8th-bit prefixing, */
      k->xdata[(k->size)++] = k->s_ctlq; /* ditto for 8th-bit prefix. */
    else if (k->rptflg && a7 == k->rptq) /* If doing run-length encoding, */
      k->xdata[(k->size)++] = k->s_ctlq; /* ditto for repeat prefix. */

    k->xdata[(k->size)++] = a;		/* Finally, emit the character. */
    k->xdata[(k->size)] = '\0';		/* Terminate string with null. */
}

STATIC int
nxtpkt(struct k_data * k) {		/* Get next packet to send */
    k->s_seq = (k->s_seq + 1) & 63;	/* Next sequence number */
    k->xdata = k->xdatabuf;
    return(0);
}

STATIC int
resend(struct k_data * k) {
    UCHAR * buf;
    if (!k->opktlen)			/* Nothing to resend */
      return(X_OK);
    buf = k->opktbuf;
    debug(DB_PKT,">PKT",&buf[1],k->opktlen);
    return((*(k->txd))(k,buf,k->opktlen));
}
