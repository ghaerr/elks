#include <linuxmt/config.h>
#include <linuxmt/init.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/types.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/ntty.h>

#include <arch/system.h>
#include <arch/segment.h>

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

#if defined(CONFIG_CONSOLE_DIRECT) || defined(CONFIG_CONSOLE_BIOS)
	console_init();
#endif

	/* set console from bootopts console device, 0=default*/
    set_console((dev_t)setupw(0x1e8));

#ifdef CONFIG_CHAR_DEV_RS
	serial_console_init();
#endif

    device_setup();

#ifdef CONFIG_SOCKET
    sock_init();
#endif

    fs_init();

    mm_stat(base, end);

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
    num = sys_open(s="/dev/console", O_RDWR, 0);
    if (num < 0)
		printk("Unable to open %s (error %d)\n", s, -num);

    sys_dup(num);		/* open stdout*/
    sys_dup(num);		/* open stderr*/
    printk("No init - running /bin/sh\n");

	/* unset special sys_wait4() processing for init, set ppid to 1*/
	current->ppid = 1;

    run_init_process("/bin/sh");
    run_init_process("/bin/sash");
    panic("No init or sh found");
}
