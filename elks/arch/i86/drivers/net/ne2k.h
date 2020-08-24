#ifndef NE2K_H
#define NE2K_H

#include <arch/ports.h>

// NE2K status

#define NE2K_STAT_RX    0x0001  // packet received
#define NE2K_STAT_TX    0x0002  // packet sent
#define NE2K_STAT_OF    0x0010  // RX ring overflow
#define NE2K_STAT_RDC   0x0040  // Remote DMA complete


// From low level NE2K PHY

extern word_t ne2k_phy_get (word_t reg, word_t * val);
extern word_t ne2k_phy_set (word_t reg, word_t val);


// From low level NE2K MAC

extern word_t ne2k_link_stat ();
extern word_t ne2k_int_stat ();

extern word_t ne2k_probe ();
extern void   ne2k_reset ();

extern void   ne2k_init ();
extern void   ne2k_term ();

extern void   ne2k_start ();
extern void   ne2k_stop ();

extern void   ne2k_addr_set (byte_t *addr);

extern word_t ne2k_rx_stat ();
extern word_t ne2k_tx_stat ();

extern word_t ne2k_pack_get (byte_t * pack);
extern word_t ne2k_pack_put (byte_t * pack, word_t len);

extern word_t ne2k_test ();

extern word_t ne2k_getpage(void);
extern void ne2k_get_addr(byte_t *);
extern void ne2k_get_hw_addr(word_t *);
extern word_t ne2k_clr_oflow(void);
extern void ne2k_rdc(void);
extern void ne2k_get_errstat(byte_t *);

#endif /* !NE2K_H */
