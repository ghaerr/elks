#ifndef __ARCH_8086_IRQ_H
#define __ARCH_8086_IRQ_H

#define save_flags(x)	x=__save_flags()
/* Who gets which entry in bh_base.  Things which will occur most often
   should come first - in which case NET should be up the top with SERIAL/TQUEUE! */

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
extern unsigned long __save_flags();
extern void restore_flags();
extern int request_irq();
extern void free_irq();
extern void cli();
extern void sti();
extern void do_bottom_half();

extern int bh_mask_count[32];
extern unsigned long bh_active;
extern unsigned long bh_mask;
extern void (*bh_base[32])();

#define init_bh(nr, routine) bh_base[nr] = routine; \
            bh_mask_count[nr] = 0; \
            bh_mask |= 1 << nr;

#define mark_bh(nr) set_bit(nr, &bh_active);

#define disable_bh(nr) bh_mask &= ~(1 << nr); bh_mask_count[nr]++;
#define enable_bh(nr) if (!--bh_mask_count[nr]) bh_mask |= 1 << nr;

#endif /* __ARCH_8086_IRQ_H */
