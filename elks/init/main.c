#include <linuxmt/config.h>
#include <linuxmt/init.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/types.h>
#include <linuxmt/fcntl.h>
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

    device_setup();

#ifdef CONFIG_SOCKET
    sock_init();
#endif

    fs_init();

    mm_stat(base, end);
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
	char *s;

    mount_root();

	/* run init, normally no return*/
	run_init_process("/bin/init");

	/* No init, open stdin and try running shell*/
#ifdef CONFIG_CONSOLE_SERIAL
    num = sys_open(s="/dev/ttyS0", O_RDWR, 0);
#else
    num = sys_open(s="/dev/tty1", O_RDWR, 0);
#endif
    if (num < 0)
		printk("Unable to open %s (error %d)\n", s, -num);

    sys_dup(num);		/* open stdout*/
    sys_dup(num);		/* open stderr*/
    printk("No init - running /bin/sh\n");

    run_init_process("/bin/sh");
    run_init_process("/bin/sash");
    panic("No init or sh found");
}
