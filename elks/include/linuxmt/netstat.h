#ifndef __LINUXMT_NETSTAT_H
#define __LINUXMT_NETSTAT_H

#include <linuxmt/types.h>

#define MAX_ETHS	3	/* max NICs */

/* /bootopts parms for each NIC */
struct netif_parms {
	int	irq;
	int	port;
	unsigned int ram;
    unsigned int flags;
};
extern struct netif_parms netif_parms[MAX_ETHS];

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

/* status flags for if_status */
#define	NETIF_IS_8BIT	1
#define NETIF_FORCE_4K	2
#define NETIF_IS_OPEN	4
#define NETIF_IS_QEMU	8
#define NETIF_FOUND	16	/* To avoid looping in QEMU */





#endif
