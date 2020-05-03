#ifndef TCPDEV_H
#define TCPDEV_H

#define TCPDEV_BUFSIZE	2046

#define MAX_ADDR_LEN    7
#define IFNAMSIZ        16

struct net_device
 {
    char name[ IFNAMSIZ ];	/* name of the device */

    unsigned long mem_start;
    unsigned long mem_end;
    unsigned short base_addr;
    unsigned char irq;
    unsigned char dma;
    unsigned char port;

	int	ifindex;			            /* interface index */
	unsigned short hard_header_len; 	/* value of hard_header_len is 14 (ETH_HLEN) for Ethernet interfaces */
	unsigned int	mtu;	      	    /* maximum transmission unit */
	unsigned char dev_addr[MAX_ADDR_LEN]; /* must be read from the interface board */
	unsigned short type; /* hardware type of the interface. Type field is used by ARP. Value for Ethernet interfaces is ARPHRD_ETHER */
	unsigned char addr_len; /* Hardware (MAC) address length and device hardware addresses. The Ethernet address length is six octets*/
	unsigned char broadcast[MAX_ADDR_LEN];

	unsigned short flags; /* bit mask including the IFF_ prefixed interface flags */

};

struct net_device *dev; /* global */

 #define ARPHRD_ETHER    1 /* for net_device.type */

 /* Standard interface flags (netdevice->flags). */
  #define IFF_UP          0x1             /* interface is up              */
  #define IFF_BROADCAST   0x2             /* broadcast address valid      */
  #define IFF_DEBUG       0x4             /* turn on debugging            */
  #define IFF_LOOPBACK    0x8             /* is a loopback net            */
  #define IFF_POINTOPOINT 0x10            /* interface is has p-p link    */
  #define IFF_NOTRAILERS  0x20            /* avoid use of trailers        */
  #define IFF_RUNNING     0x40            /* interface RFC2863 OPER_UP    */
  #define IFF_NOARP       0x80            /* no ARP protocol              */
  #define IFF_PROMISC     0x100           /* receive all packets          */
  #define IFF_ALLMULTI    0x200           /* receive all multicast packets*/

  #define IFF_MASTER      0x400           /* master of a load balancer    */
  #define IFF_SLAVE       0x800           /* slave of a load balancer     */

  #define IFF_MULTICAST   0x1000          /* Supports multicast           */

  #define IFF_PORTSEL     0x2000          /* can set media type           */
  #define IFF_AUTOMEDIA   0x4000          /* auto media select active     */
  #define IFF_DYNAMIC     0x8000          /* dialup device with changing addresses*/

  #define IFF_LOWER_UP    0x10000         /* driver signals L1 up         */
  #define IFF_DORMANT     0x20000         /* driver signals dormant       */

  #define IFF_ECHO        0x40000         /* echo sent packets            */

void tcpdev_process(void);
int tcpdev_init(char *fdev);
void retval_to_sock(__u16 sock, short int r);
void tcpdev_checkread(struct tcpcb_s *cb);
void tcpdev_sock_state(struct tcpcb_s *cb, int state);
void tcpdev_checkaccept(struct tcpcb_s *cb);

#endif /* !TCPDEV_H */
