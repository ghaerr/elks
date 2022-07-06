/*
 * eth_msgs.h
 * Ethernet driver error message strings
 */


#define EMSG_OFLOW	"%s: Rcv oflow (0x%x), keep %d\n"
#define EMSG_TXERR	"%s: TX-error, status 0x%x\n"
#define EMSG_RXERR	"%s: RX-error, status 0x%x\n"
#define EMSG_IRQERR	"%s: Unable to use IRQ %d (errno %d)\n"
#define EMSG_DMGPKT	"%s: Damaged packet, hdr 0x%x %u, buffer cleared\n"
#define EMSG_BGSPKT	"%s: Bogus packet: status %x nxpg 0x%x size %d\n"
#define EMSG_ERROR	"%s: NIC under-/overrun, status 0x%x\n"
