#ifndef NE2K_H
#define NE2K_H

#include <arch/ports.h>

/* NE2K interrupt status bits */

#define NE2K_STAT_RX    0x0001  /* packet received */
#define NE2K_STAT_TX    0x0002  /* packet sent */
#define NE2K_STAT_RXE	0x0004	/* RX error */
#define NE2K_STAT_TXE	0x0008	/* TX error */
#define NE2K_STAT_OF    0x0010  /* RX ring overflow */
#define NE2K_STAT_CNT   0x0020  /* Tally counter overflow */
#define NE2K_STAT_RDC   0x0040  /* Remote DMA complete */

/* 8390 Page 0 register offsets (from net_port) */
#define EN0_STARTPG	0x01U	/* Starting page of ring bfr WR */
#define EN0_STOPPG	0x02U	/* Ending page +1 of ring bfr WR */
#define EN0_BOUNDARY	0x03U	/* Boundary page of ring bfr RD WR */
#define EN0_TSR		0x04U	/* Transmit status reg RD */
#define EN0_TPSR	0x04U	/* Transmit starting page WR */
#define EN0_ISR		0x07U	/* Interrupt status reg RD WR */
#define EN0_RSR		0x0cU	/* rx status reg RD */
#define EN0_RXCR	0x0cU	/* RX configuration reg WR */
#define EN0_TXCR	0x0dU	/* TX configuration reg WR */
#define EN0_CNTR0	0x0dU	/* Counter 0 reg RD */
#define EN0_DCFG	0x0eU	/* Data configuration reg WR */
#define EN0_IMR		0x0fU	/* Interrupt mask reg WR */

/* From low level NE2K MAC */

extern word_t ne2k_int_stat();

extern void   ne2k_reset();

extern void   ne2k_init();

extern void   ne2k_start();
extern void   ne2k_stop();

extern void   ne2k_addr_set(byte_t *);

extern word_t ne2k_rx_stat();
extern word_t ne2k_tx_stat();

extern word_t ne2k_pack_get(char *, word_t, word_t *);
extern word_t ne2k_pack_put(char *, word_t);

extern word_t ne2k_test();

extern word_t ne2k_getpage(void);
extern word_t ne2k_clr_oflow(word_t);
extern word_t ne2k_get_tx_stat(void);
extern word_t ne2k_get_rx_stat(void);

extern void ne2k_get_addr(byte_t *);
extern void ne2k_get_hw_addr(word_t *);
extern void ne2k_rdc(void);
extern void ne2k_get_errstat(byte_t *);
extern void ne2k_clr_err_cnt(void);
extern void ne2k_rx_init(void);

#endif /* !NE2K_H */
