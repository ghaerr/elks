/*
 * National Semiconductor DP8390/83C690 NIC core register definitions.
 *
 * Shared by the WD80x3 (wd.c) and SMC Ultra (ultra.c) drivers: both cards
 * use the same 8390 NIC core, and differ only in the surrounding ASIC.
 * Card-specific ASIC registers stay in the individual drivers.
 */

#ifndef _ARCH_8390_H
#define _ARCH_8390_H

#define E8390_RXCONFIG	0x04U	/* EN0_RXCR: broadcasts, no multicast or errors */
#define E8390_RXOFF	0x20U	/* EN0_RXCR: Accept no packets */
#define E8390_TXCONFIG	0x00U	/* EN0_TXCR: Normal transmit mode */
#define E8390_TXOFF	0x02U	/* EN0_TXCR: Transmitter off */
#define E8390_STOP	0x01U	/* Stop and reset the chip */
#define E8390_START	0x02U	/* Start the chip, clear reset */
#define E8390_TRANS	0x04U	/* Transmit a frame */
#define E8390_RREAD	0x08U	/* Remote read */
#define E8390_RWRITE	0x10U	/* Remote write  */
#define E8390_NODMA	0x20U	/* Remote DMA */
#define E8390_PAGE0	0x00U	/* Select page chip registers */
#define E8390_PAGE1	0x40U	/* using the two high-order bits */
#define E8390_PAGE2	0x80U	/* Page 3 is invalid */
#define E8390_CMD	0x00U	/* The command register (for all pages) */

/* For register EN0_ISR. */
#define E8390_TX_IRQ_MASK	0x0aU
#define E8390_RX_IRQ_MASK	0x05U

/* Page 0 register offsets. */
#define EN0_CLDALO	0x01	/* Low byte of current local dma addr  RD */
#define EN0_STARTPG	0x01	/* Starting page of ring bfr WR */
#define EN0_CLDAHI	0x02	/* High byte of current local dma addr  RD */
#define EN0_STOPPG	0x02	/* Ending page +1 of ring bfr WR */
#define EN0_BOUNDARY	0x03	/* Boundary page of ring bfr RD WR */
#define EN0_TSR		0x04	/* Transmit status reg RD */
#define EN0_TPSR	0x04	/* Transmit starting page WR */
#define EN0_NCR		0x05	/* Number of collision reg RD */
#define EN0_TCNTLO	0x05	/* Low  byte of tx byte count WR */
#define EN0_FIFO	0x06	/* FIFO RD */
#define EN0_TCNTHI	0x06	/* High byte of tx byte count WR */
#define EN0_ISR		0x07	/* Interrupt status reg RD WR */
#define EN0_CRDALO	0x08	/* low byte of current remote dma address RD */
#define EN0_RSARLO	0x08	/* Remote start address reg 0 */
#define EN0_CRDAHI	0x09	/* high byte, current remote dma address RD */
#define EN0_RSARHI	0x09	/* Remote start address reg 1 */
#define EN0_RCNTLO	0x0a	/* Remote byte count reg WR */
#define EN0_RCNTHI	0x0b	/* Remote byte count reg WR */
#define EN0_RSR		0x0c	/* rx status reg RD */
#define EN0_RXCR	0x0c	/* RX configuration reg WR */
#define EN0_TXCR	0x0d	/* TX configuration reg WR */
#define EN0_COUNTER0	0x0d	/* Rcv alignment error counter RD */
#define EN0_DCFG	0x0e	/* Data configuration reg WR */
#define EN0_COUNTER1	0x0e	/* Rcv CRC error counter RD */
#define EN0_IMR		0x0f	/* Interrupt mask reg WR */
#define EN0_COUNTER2	0x0f	/* Rcv missed frame error counter RD */

/* Page 1 register offsets. */
#define EN1_PHYS	0x01U	/* This board's physical enet addr RD WR */
#define EN1_CURPAG	0x07U	/* Current memory page RD WR */
#define EN1_MULT	0x08U	/* Multicast filter mask array (8 bytes) RDWR */

/* Bits in received packet status byte and EN0_RSR */
#define ENRSR_RXOK	0x01U	/* Received a good packet */
#define ENRSR_CRC	0x02U	/* CRC error */
#define ENRSR_FAE	0x04U	/* frame alignment error */
#define ENRSR_FO	0x08U	/* FIFO overrun */
#define ENRSR_MPA	0x10U	/* missed pkt */
#define ENRSR_PHY	0x20U	/* physical/multicase address */
#define ENRSR_DIS	0x40U	/* receiver disable. set in monitor mode */
#define ENRSR_DEF	0x80U	/* deferring */

/* Bits in EN0_ISR - Interrupt status register */
#define ENISR_RX	0x01U	/* Packet received */
#define ENISR_TX	0x02U	/* Transmit packet completed */
#define ENISR_RX_ERR	0x04U	/* Receive error */
#define ENISR_TX_ERR	0x08U	/* Transmit error */
#define ENISR_OFLOW	0x10U	/* Receiver out of buffer space */
#define ENISR_COUNTERS	0x20U	/* Counters need emptying */
#define ENISR_RDC	0x40U	/* Remote dma complete */
#define ENISR_RESET	0x80U	/* Reset completed */
#define ENISR_ALL	0x1fU	/* Enable these interrupts, skip RDC and stats */

#endif /* _ARCH_8390_H */
