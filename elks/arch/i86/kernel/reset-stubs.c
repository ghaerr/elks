#include <arch/system.h>

/*
 * This function gets called by the keyboard interrupt handler.
 * As it's called within an interrupt, it may NOT sync.
 */
void ctrl_alt_del(void)
{
}

void hard_reset_now(void)
{
}

void apm_shutdown_now(void)
{
}
