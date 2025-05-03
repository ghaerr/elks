/*
 * WonderSwan timer unit
 */

#include <linuxmt/config.h>
#include <arch/io.h>
#include <arch/param.h> /* For the definition of HZ */
#include <arch/swan.h>

#define TIMER_INTERVAL (12000 / HZ)

void enable_timer_tick(void)
{
    outw(TIMER_INTERVAL, HBL_TMR_RELOAD_PORT);
    outw(HBL_TMR_REPEAT, TMR_CONTROL_PORT);
}

void disable_timer_tick(void)
{
    outw(0, TMR_CONTROL_PORT);
}
