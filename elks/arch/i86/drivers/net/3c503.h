/* DECLARE SUB DumpIP (A AS STRING)
void el2init (struct dpeth *dep);
DECLARE FUNCTION inetntoa$ (A AS STRING)
DECLARE FUNCTION BuildIP$ (Dta AS ANY)
DECLARE FUNCTION htons$ (A!, B!, C!, D!)
DECLARE FUNCTION el2memtest! (patt AS LONG)
DECLARE FUNCTION el2probe! ()
DECLARE FUNCTION inb! (reg AS LONG)
DECLARE SUB outb (reg AS LONG, Dta AS LONG) */

#define eth_IP 0x0800
#define eth_ARP 0x0806
#define eth_VINES 0x0bad
#define eth_DECNET 0x6003
#define ETH_DECLAT 0x6004	/* Local area transport */
#define ETH_AT 0x809b		/* Appletalk */
#define ETH_IPX 0x8137

#define IP_ICMP 0x1  /* Internet Control Message Protocol */
#define IP_GGP  0x3  /* Gateway-Gateway protocol */
#define IP_TCP  0x6  /* Transmission Control Protocol */
#define IP_EGP  0x8  /* Exterior Gateway Protocol */
#define IP_UDP  0x11 /* User Datagram Protocol */

struct sendq {
  int sq_filled;
  int sq_size;
  int sq_sendpage;
};

struct dpeth {
  int de_baseport;
  int de_irq;
  int de_linmem;
  unsigned char de_macaddr[6];
  int de_16bit;
  int de_ramsize;
  int de_offsetpage;
  int de_startpage;
  int de_stoppage;

  int de_memsegm;

  int de_dataport;
  int de_dp8390port;
  int de_sendq_nr;
  int de_sendq_head;
  int de_sendq_tail;
};

struct ethframe {
  unsigned char dest[6];
  unsigned char src[6];
  int type;
  char *data;
};

/*   7 6 5 4 3 2 1 0

     |-VER-| |-IHL-| */

typedef struct IPframe {
  int ver;
  int ihl;
  int tos;
  int tol;  /* total length <64k incl. header */
  int id;
  int flags;
  int fragoffset;
  int ttl;
  int protocol;
  int checksum;
  unsigned char src[4];
  unsigned char dest[4];
  unsigned char options[4];  /* Not used generally, but must exist for padding */
};
