#ifndef NE2K_H
#define NE2K_H

#include <arch/ports.h>

/* NE2K interrupt status bits */

#define NE2K_STAT_RX    0x0001  // packet received
#define NE2K_STAT_TX    0x0002  // packet sent
#define NE2K_STAT_RXE	0x0004	// RX error
#define NE2K_STAT_TXE	0x0008	// TX error
#define NE2K_STAT_OF    0x0010  // RX ring overflow
#define NE2K_STAT_CNT   0x0020  // Tally counter overflow
#define NE2K_STAT_RDC   0x0040  // Remote DMA complete

/* Configuration flags */
#define NE2KF_4K_BUF	0x01	/* Force 4k buffer */
#define NE2KF_16K_BUF	0x02	/* Force 16k buffer */
#define NE2KF_8BIT_BUS	0x04	/* Force 8bit mode */
#define NE2KF_16BIT_BUS	0x08	/* Force 16bit mode */
#define NE2KF_VERBOSE	0x80U	/* Activate verbose mode */

/* From low level NE2K MAC */

extern word_t ne2k_int_stat();

extern word_t ne2k_probe();
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
extern void ne2k_clr_rxe(void);
extern void ne2k_rx_init(void);
extern void eth_pmsg(int, int, ...);

#endif /* !NE2K_H */
