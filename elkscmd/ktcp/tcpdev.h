#ifndef TCPDEV_H
#define TCPDEV_H

#define TCPDEV_BUFSIZE	2048	/* must be at least as big as CB_IN_BUF_SIZE*/

#if 0
#define MAX_ADDR_LEN    7
#define IFNAMSIZ        16
struct net_device {
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
#endif

extern int tcpdevfd;

void tcpdev_process(void);
int tcpdev_init(char *fdev);
void retval_to_sock(void *sock, short int r);
void tcpdev_checkread(struct tcpcb_s *cb);
void tcpdev_sock_state(struct tcpcb_s *cb, int state);
void tcpdev_checkaccept(struct tcpcb_s *cb);

#endif
