/*
net/gen/vjhc.h

Defines for Van Jacobson TCP/IP Header Compression as defined in RFC-1144

Created:	Nov 15, 1993 by Philip Homburg <philip@cs.vu.nl>
*/

/*
   Adaptation to SLIP driver and Minix 2.0.0
   Sep 1997 - Claudio Tantignone (c_tantignone@hotmail.com)
*/

/* Adaptation to ELKS. Jul 2002 - Harry Kalogirou <harkal@gmx.net>
 */

#ifndef __NET__GEN__VJHC_H__
#define __NET__GEN__VJHC_H__

#define VJHC_FLAG_U	0x01
#define VJHC_FLAG_W	0x02
#define VJHC_FLAG_A	0x04
#define VJHC_FLAG_S	0x08
#define VJHC_FLAG_P	0x10
#define VJHC_FLAG_I	0x20
#define VJHC_FLAG_C	0x40

#define VJHC_SPEC_I	(VJHC_FLAG_S | VJHC_FLAG_W | VJHC_FLAG_U)
#define VJHC_SPEC_D	(VJHC_FLAG_S | VJHC_FLAG_A | VJHC_FLAG_W | VJHC_FLAG_U)
#define VJHC_SPEC_MASK	(VJHC_FLAG_S | VJHC_FLAG_A | VJHC_FLAG_W | VJHC_FLAG_U)

#endif /* __NET__GEN__VJHC_H__ */


/*
pkt.h

Created:	May 25, 1993 by Philip Homburg <philip@cs.vu.nl>
*/

#ifndef PKT_H
#define PKT_H

typedef struct pkt
{
	char *p_data;
	size_t p_offset;
	size_t p_size;
	size_t p_maxsize;
} pkt_ut;

#endif /* PKT_H */


/*
vjhc.h

Created:	Nov 11, 1993 by Philip Homburg <philip@cs.vu.nl>
*/

#ifndef VJHC_H
#define VJHC_H

extern int ip_snd_vjhc;
extern int ip_snd_vjhc_state_nr;
extern int ip_snd_vjhc_compress_cid;
extern int ip_rcv_vjhc;
extern int ip_rcv_vjhc_state_nr;
extern int ip_rcv_vjhc_compress_cid;

void ip_vjhc_init(void);
int ip_vjhc_compress(pkt_ut *pkt);
void ip_vjhc_arr_uncompr(pkt_ut *pkt);
void ip_vjhc_arr_compr(pkt_ut *pkt);

#endif /* VJHC_H */


/*
ppp.h

Created:	May 25, 1993 by Philip Homburg <philip@cs.vu.nl>
*/

#ifndef PPP_H
#define PPP_H
/*
void ppp_snd_init ARGS(( void ));
void ppp_snd_init_callback ARGS(( ppp_snd_callback_ut *callback, 
	psc_cbf_ut cb_f, int cb_arg ));
void ppp_snd_sethdr ARGS(( struct pkt *pkt, __u16 proto ));
int ppp_snd ARGS(( struct pkt *pkt, ppp_snd_callback_ut *callback ));
void ppp_snd_completed ARGS(( int result, int error ));
void ppp_snd_restart ARGS(( void ));

extern int ppp_snd_inprogress;
extern int ppp_snd_more2send;
*/
extern int ppp_snd_maxsize;
extern int ppp_snd_hc_proto;
extern int ppp_snd_hc_address;

#define TYPE_IP			0x40
#define TYPE_COMPRESSED_TCP	0x80
#define TYPE_UNCOMPRESSED_TCP	0x70

#define PPP_TYPE_IP             TYPE_IP
#define PPP_TYPE_VJHC_UNCOMPR   TYPE_UNCOMPRESSED_TCP
#define PPP_TYPE_VJHC_COMPR     TYPE_COMPRESSED_TCP
#define TYPE_ERROR 0x00
/*
#define PPP_TYPE_IP		0x0021
#define PPP_TYPE_VJHC_COMPR	0x002D
#define PPP_TYPE_VJHC_UNCOMPR	0x002F
*/
#define PPP_TYPE_IPCP		0x8021
#define PPP_TYPE_LCP		0xc021

#define PPP_BYTE_FLAG		0x7E
#define PPP_BYTE_ESCAPE		0x7D
#define PPP_BYTE_BIT6		0x20	/* warning bits are counted 87654321 */

#define PPP_HDR_ALLSTATIONS	0xFF
#define PPP_HDR_UI		0x03

#define PPP_DFLT_MAXSIZE	1500

#define PPP_MAX_HDR_SIZE	4

#define PPP_OPT_FORCED_OFF	-2
#define PPP_OPT_OFF		-1
#define PPP_OPT_NOCHANGE	 0
#define PPP_OPT_ON		 1
#define PPP_OPT_FORCED_ON	 2

#define MOD_IP			"ip"
#define MOD_LCP			"lcp"

#endif /* PPP_H */



