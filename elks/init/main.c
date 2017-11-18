/* $Header$
 */

#include <linuxmt/config.h>
#include <linuxmt/init.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/types.h>
#include <linuxmt/utsname.h>

#include <arch/system.h>

/*
 *	System variable setups
 */
#ifdef CONFIG_FS_RO

int root_mountflags = MS_RDONLY;

#else

int root_mountflags = 0;

#endif

#if (CONFIG_BOGOMIPS == 0)
unsigned long loops_per_sec = 1;
#else
unsigned long loops_per_sec = CONFIG_BOGOMIPS;
#endif

/**************************************/

static void init_task(void);
extern int run_init_process(char *);

/*
 *	For the moment this routine _MUST_ come first.
 */

void start_kernel(void)
{
    seg_t base, end;

/* We set the idle task as #0, and init_task() will be task #1 */

    sched_init();	/* This block of functions don't need console */
    setup_arch(&base, &end);
    mm_init(base, end);
    buffer_init();
    inode_init();
    init_IRQ();
    tty_init();

    init_console();

#if (CONFIG_BOGOMIPS == 0)
    calibrate_delay();
#endif

    setup_mm(base, end);		/* Architecture specifics */
    device_setup();

#ifdef CONFIG_SOCKET
    sock_init();
#endif

    fs_init();

    printk("ELKS version %s\n", system_utsname.release);

    kfork_proc(init_task);
    wake_up_process(&task[1]);

    /*
     * We are now the idle task. We won't run unless no other process can run.
     */
    while (1) {
        schedule();

#ifdef CONFIG_IDLE_HALT
        idle_halt ();
#endif
    }
}

static void init_task()
{
    int num;

    mount_root();
#ifndef CONFIG_SMALL_KERNEL
    printk("Loading init\n");
#endif

    /* The Linux kernel traditionally attempts to start init from 4 locations,
     * as indicated by this code:
     *
     * run_init_process("/sbin/init");
     * run_init_process("/etc/init");
     * run_init_process("/bin/init");
     * run_init_process("/bin/sh");
     *
     * So, I've modified the ELKS kernel to follow this tradition.
     */

    run_init_process("/sbin/init");
    run_init_process("/etc/init");
    run_init_process("/bin/init");

#ifdef CONFIG_CONSOLE_SERIAL
    num = sys_open("/dev/ttyS0", 2, 0);		/* These are for stdin */
#else
    num = sys_open("/dev/tty1", 2, 0);
#endif
    if (num < 0)
	printk("Unable to open /dev/tty1 (error %u)\n", -num);

    if (sys_dup(num) != 1)			/* This is for stdout */
	printk("dup failed\n");
    sys_dup(num);				/* This is for stderr */
    printk("No init - running /bin/sh\n");

    run_init_process("/bin/sh");
    panic("No init or sh found");
}

#if (CONFIG_BOGOMIPS == 0)
/*
 *	Yes its the good old bogomip counter
 */

static void delay(jiff_t loops)
{
    register char *hw = *(((unsigned char **)(&loops))+1);
    register char *lw = *((unsigned char **)(&loops));

    do {
   	do {
    	} while (lw--);
    } while (hw--);
}

int calibrate_delay(void)
{
    jiff_t ticks;

    printk("Calibrating delay loop... ");
    do {
	ticks = jiffies;
	delay(loops_per_sec);
	ticks = jiffies - ticks;
	if (!ticks)
	    ticks = 1L;
	loops_per_sec = (loops_per_sec * (jiff_t)HZ) / ticks;
	if (ticks >= (jiff_t)HZ) {
	    printk("ok - %u.%02u BogoMips\n",
		    (__u16)(loops_per_sec / 391500L),
		    (__u16)((loops_per_sec / 3915L) % 100L));
	    return 0;
	}
    } while (loops_per_sec < (4294967296L/((jiff_t)HZ)));
    printk("failed\n");

    return -1;
}
#endif
