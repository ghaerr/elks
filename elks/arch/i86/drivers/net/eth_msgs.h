/*
 * eth_msgs.h
 * Ethernet driver error message strings
 */


#define EMSG_OFLOW	"%s: Rcv oflow (0x%x), keep %d\n"
#define EMSG_TXERR	"%s: TX-error, status %#x\n"
#define EMSG_RXERR	"%s: RX-error, status %#x\n"
#define EMSG_IRQERR	"%s: Unable to use IRQ %d (errno %d)\n"
#define EMSG_DMGPKT	"%s: Damaged packet, hdr %#x %u, buffer cleared\n"
#define EMSG_BGSPKT	"%s: Bogus packet: status %#x nxpg %#x size %d\n"
#define EMSG_ERROR	"%s: NIC under-/overrun, status %#x\n"
