/*
 * Philips :YES timer tick
 *
 * Port-specific companion to the dual-use irq-8018x.c.
 *
 * The :YES ROM owns the timers: Timer0 runs at the BIOS tick rate on
 * interrupt type 8, and types 18 and 19 (Timer1/Timer2) are the ROM's
 * INT 12h/13h entry points. The kernel chains the existing Timer0
 * vector instead of programming the TCU, so there is nothing to set
 * up or tear down here.
 *
 * See timer-8018x.c for the 80C188EB, which drives its own timers.
 */

#include <linuxmt/config.h>
#include <arch/80186.h>

void enable_timer_tick(void)
{
}

void disable_timer_tick(void)
{
}
