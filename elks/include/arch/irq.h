#ifndef LX86_ARCH_IRQ_H
#define LX86_ARCH_IRQ_H

#include <arch/types.h>

#define save_flags(x)	x=__save_flags()

/* Who gets which entry in bh_base. Things which will occur most often
   should come first - in which case NET should be up the top with
   SERIAL/TQUEUE! */

enum {
    TIMER_BH = 0,
    CONSOLE_BH,
    TQUEUE_BH,
    DIGI_BH,
    SERIAL_BH,
    RISCOM8_BH,
    SPECIALIX_BH,
    BAYCOM_BH,
    NET_BH,
    IMMEDIATE_BH,
    KEYBOARD_BH,
    CYCLADES_BH,
    CM206_BH
};

extern void disable_irq(void);
extern void enable_irq(void);
extern void do_IRQ(void);
extern flag_t __save_flags(void);
extern void restore_flags(void);
extern int request_irq(void);
extern void free_irq(void);

extern void do_bottom_half(void);

extern int bh_mask_count[16];
extern unsigned bh_active;
extern unsigned bh_mask;
extern void (*bh_base[16]) (void);

#ifdef __KERNEL__
#define i_cli() asm("cli")
#define i_sti() asm("sti")
#else
#define i_cli()
#define i_sti()
#endif

#ifdef ENDIS_BH

#define init_bh(nr, routine) do { \
	bh_base[nr] = routine; \
	bh_mask_count[nr] = 0; \
	bh_mask |= 1 << nr; \
	} until 0

#define disable_bh(nr) do { \
	bh_mask &= ~(1 << nr); \
	bh_mask_count[nr]++; \
	} until 0

#define enable_bh(nr) if (!--bh_mask_count[nr]) \
	bh_mask |= 1 << nr

#else

#define init_bh(nr, routine) bh_base[nr] = routine

#endif

#define mark_bh(nr) set_bit(nr, &bh_active)

#endif
