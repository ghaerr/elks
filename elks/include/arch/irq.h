#ifndef __ARCH_8086_IRQ_H__
#define __ARCH_8086_IRQ_H__

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

extern void disable_irq();
extern void enable_irq();
extern void do_IRQ();
extern flag_t __save_flags();
extern void restore_flags();
extern int request_irq();
extern void free_irq();

#if 0
extern void icli();
extern void isti();
#endif

extern void do_bottom_half();

extern int bh_mask_count[16];
extern unsigned bh_active;
extern unsigned bh_mask;
extern void (*bh_base[16])();

#ifdef __KERNEL__
#define icli() asm("cli")
#define isti() asm("sti")
#else
#define icli()
#define isti()
#endif

#ifdef ENDIS_BH

#define init_bh(nr, routine) bh_base[nr] = routine; \
            bh_mask_count[nr] = 0; \
            bh_mask |= 1 << nr;

#define disable_bh(nr) bh_mask &= ~(1 << nr); bh_mask_count[nr]++;
#define enable_bh(nr) if (!--bh_mask_count[nr]) bh_mask |= 1 << nr;

#else

#define init_bh(nr, routine) bh_base[nr] = routine;

#endif

#define mark_bh(nr) set_bit(nr, &bh_active);

#endif
