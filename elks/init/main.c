/* $Header$ 
 */

#include <linuxmt/config.h>
#include <linuxmt/init.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/timex.h>
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

/**************************************/

static void init_task(void);
extern int run_init_process(char *, char *);

extern __ptask _reglasttask, _regnexttask;

jiff_t loops_per_sec = 1;

/*
 *	For the moment this routine _MUST_ come first.
 */

void start_kernel(void)
{
    seg_t base, end;

/* We set the scheduler up as task #0, and this as task #1 */

    setup_arch(&base, &end);
    mm_init(base, end);
    init_IRQ();
    init_console();

#if 0
    calibrate_delay();
#endif

    setup_mm();			/* Architecture specifics */
    tty_init();
    buffer_init();

#ifdef CONFIG_SOCKET
    sock_init();
#endif

    device_setup();
    inode_init();
    fs_init();
    sched_init();

    printk("ELKS version %s\n", system_utsname.release);

    task[0].t_kstackm = KSTACK_MAGIC;
    task[0].next_run = task[0].prev_run = &task[0];
    kfork_proc(&task[1], init_task);

    /* 
     * We are now the idle task. We won't run unless no other process can run.
     */
    while (1){
        schedule();
    }

}

static char args[] = "\0\0\0\0\0\0/bin/init\0\0";
/*		      ^   ^   ^		     ^
 *		      |   |   \sep	     \envp
 *		      |	  \argv[0]
 *		      \argc
 */

/*@unused@*/ static char envp[] = "\0\0";

static void init_task()
{
    int num;

    mount_root();
    printk("Loading init\n");

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

    num = run_init_process("/sbin/init", args);
	printk("sys_execve(\"/sbin/init\",args,18) => %d.\n",num);
	num = run_init_process("/etc/init", args);
	    printk("sys_execve(\"/etc/init\",args,18) => %d.\n",num);
	    num = run_init_process("/bin/init", args);
		printk("sys_execve(\"/bin/init\",args,18) => %d.\n",num);

#ifdef CONFIG_CONSOLE_SERIAL
		num = sys_open("/dev/ttyS0", 2, 0);
#else
		num = sys_open("/dev/tty1", 2, 0);
#endif
		if (num < 0)
		    printk("Unable to open /dev/tty (error %u)\n", -num);

		if (sys_dup(0) != 1)
	    	printk("dup failed\n");
		sys_dup(0);
		printk("No init - running /bin/sh\n");

		num = run_init_process("/bin/sh", args);
		printk("sys_execve(\"/bin/sh\",args,18) => %d.\n",num);
	    	    panic("No init or sh found");
}

/*
 *	Yes its the good old bogomip counter
 */

static void delay(jiff_t loops)
{
    do {
   	do {
    	} while((*((unsigned int *)(&loops)))--);
    } while((*(((unsigned int *)(&loops))+1))--)
}

int calibrate_delay(void)
{
    jiff_t ticks, bogo, sub;

    printk("Calibrating delay loop... ");
    while ((loops_per_sec <<= 1)) {
	ticks = jiffies;
	delay(loops_per_sec);
	ticks = jiffies - ticks;
	if (ticks >= HZ) {
	    loops_per_sec = (loops_per_sec / ticks) * (jiff_t) HZ;
	    bogo = loops_per_sec / 500000L;
	    sub = loops_per_sec / 5000L;
	    sub %= 100;
	    printk("ok - %lu.%s%lu BogoMips\n",
		   bogo, (sub < 10) ? "0" : "", sub);
	    return 0;
	}
    }
    printk("failed\n");

    return -1;
}
