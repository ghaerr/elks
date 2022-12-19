#ifndef __LINUXMT_NETSTAT_H
#define __LINUXMT_NETSTAT_H

#define MAX_ETHS	3	/* max NICs */

/* Enumeration for the netif_parms array */
#define ETH_NE2K	0
#define ETH_WD		1
#define ETH_EL3		2

#ifndef __ASSEMBLER__
#include <linuxmt/types.h>

/* /bootopts parms for each NIC */
struct netif_parms {
	int	irq;
	int	port;
	unsigned int ram;
	unsigned int flags;
};

/* Should put this into the eths struct */
extern struct netif_parms netif_parms[MAX_ETHS];

struct eth {
    struct file_operations *ops;
    struct netif_stat  *stats;
};

/* status for each NIC, returned through ioctl */
struct netif_stat {
	__u16 rx_errors;	/* Receive errors, flagged by NIC */
	__u16 rq_errors;	/* Receive queue errors (in 8bit interfaces) */
	__u16 tx_errors;	/* Transmit errors, flagged by NIC */
	__u16 oflow_errors;	/* Receive buffer overflow interrupts */
	__u16 if_status;	/* Interface status flags */
	int   oflow_keep;	/* # of packets to keep if overflow */
	char  mac_addr[6];	/* Current MAC address */
};

#endif	/* __ASSEMBLER__ */

/* status flags for if_status */
/* The lower nibble has the autodetected buffer memory size,
 * 1=4k, 2=8k, 3=16k etc. */
#define	NETIF_AUTO_8BIT	0x10	
#define NETIF_IS_QEMU	0x20

/* Config flags for 8390 based NICs */
/* The first 3 make a number - for coding simplicity (a power of two),
 * the rest are regular flag bits */
#define ETHF_8K_BUF	0x01	/* Force  8K NIC (default on SMC/WD memory mapped NICs) */
#define ETHF_16K_BUF	0x02	/* Force 16k NIC buffer */
#define ETHF_32K_BUF	0x03	/* Force 32k NIC buffer */
#define ETHF_4K_BUF	0x04	/* Force  4k NIC buffer */
#define ETHF_8BIT_BUS	0x10	/* Force  8 bit bus */
#define ETHF_16BIT_BUS 	0x20	/* Force 16 bit bus */
#define ETHF_USE_AUI	0x40	/* Use AUI connection */
#define ETHF_VERBOSE	0x80U	/* turn on verbose console error messages */

#endif
