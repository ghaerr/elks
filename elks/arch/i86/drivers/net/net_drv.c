/* Initialize ethernet drivers */

#include <linuxmt/config.h>

void eth_init(void)
{
#ifdef CONFIG_ETH_NE2K
	ne2k_drv_init();
#endif
}
