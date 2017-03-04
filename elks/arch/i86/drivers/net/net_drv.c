/* Initialize ethernet drivers */

void eth_init(void)
{
#ifdef CONFIG_ETH_NE2K
	ne2k_drv_init();
#endif
}
