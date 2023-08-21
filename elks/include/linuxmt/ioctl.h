#ifndef __LINUXMT_IOCTL_H
#define __LINUXMT_IOCTL_H

/* Use MAJOR as high-level byte for numbering to avoid mixing operations */

/* Block device generic driver operations */
#define IOCTL_BLK_GET_SECTOR_SIZE 0x0330    /* ioctl get drive sector size */

/* Ethernet generic driver operations */
#define IOCTL_ETH_ADDR_GET      0x0901
#define IOCTL_ETH_ADDR_SET      0x0902
#define IOCTL_ETH_HWADDR_GET    0x0903  /* get physical HW address from NIC */
#define IOCTL_ETH_GETSTAT       0x0904  /* get error stats from NIC */
#define IOCTL_ETH_OFWSKIP_SET   0x0906  /* Set # of packets to skip on buffer overflow */
#define IOCTL_ETH_OFWSKIP_GET   0x0905  /* get current overrflow skip value */

#endif
