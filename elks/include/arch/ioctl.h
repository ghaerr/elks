#ifndef __ARCH_8086_IOCTL_H
#define __ARCH_8086_IOCTL_H


// Use MAJOR as high-level byte for numbering
// to avoid mixing operations


// Ethernet generic driver operations

#define IOCTL_ETH_TEST		0x0900
#define IOCTL_ETH_ADDR_GET	0x0901
#define IOCTL_ETH_ADDR_SET	0x0902
#define IOCTL_ETH_HWADDR_GET	0x0903  // get physical HW address from NIC
#define IOCTL_ETH_GETSTAT	0x0904  // get error stats from NIC
#define IOCTL_ETH_OFWSKIP_SET	0x0906  // Set # of packets to skip in case of buffer overflow
#define IOCTL_ETH_OFWSKIP_GET	0x0905  // get current overrflow skip value
 


#endif
