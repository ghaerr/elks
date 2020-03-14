#ifndef __ARCH_8086_IOCTL_H
#define __ARCH_8086_IOCTL_H


// Use MAJOR as high-level byte for numbering
// to avoid mixing operations


// Ethernet generic driver operations

#define IOCTL_ETH_TEST      0x0900
#define IOCTL_ETH_ADDR_GET  0x0901
#define IOCTL_ETH_ADDR_SET  0x0902


#endif
