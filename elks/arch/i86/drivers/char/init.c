#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/init.h>

void INITPROC chr_dev_init(void)
{
#ifdef CONFIG_CHAR_DEV_LP
    lp_init();
#endif

#ifdef CONFIG_CHAR_DEV_CGATEXT
    cgatext_init();
#endif

#ifdef CONFIG_CHAR_DEV_MEM
    mem_dev_init();
#endif

#ifdef CONFIG_AUDIO_SB
    dsp_init();
#endif
#ifdef CONFIG_CHAR_DEV_DSP_AD1848
    ad1848_dsp_init();
#endif

#ifdef CONFIG_INET
    tcpdev_init();
#endif

#ifdef CONFIG_ETH
    eth_init();
#endif

#ifdef CONFIG_PSEUDO_TTY    
    pty_init();    
#endif
}
