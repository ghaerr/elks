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

    /* Make sure the correct exec stack is in place for init. */

    unsigned short int *pip = (unsigned short int *) args;

    *++pip = (unsigned short int) &args[5];

    mount_root();

    printk("Loading init\n");

    if ((num = sys_execve("/bin/init", args, 18))) {

	printk("sys_execve(\"/bin/init\",args,18) => %d.\n",num);

#ifdef CONFIG_CONSOLE_SERIAL
	num = sys_open("/dev/ttyS0", 2, 0);
#else
	num = sys_open("/dev/tty0", 2, 0);
#endif

	if (num < 0)
	    printk("Unable to open /dev/tty (error %u)\n", -num);

	if (sys_dup(0) != 1)
	    printk("dup failed\n");

	sys_dup(0);

	printk("No init - running /bin/sh\n");

	if (sys_execve("/bin/sh", args, 0))
	    panic("No init or sh found");
    }

#ifndef S_SPLINT_S

    /* Brackets round the following code are required as a work around
     * for a bug in the compiler which causes it to jump past the asm
     * code if they are not there.
     *
     * This kludge is here because we called sys_execve directly, rather
     * than via syscall_int (a BIOS interrupt). So we simulate the last
     * part of syscall_int, which restores context back to the user process.
     */
    {
#asm
	cli
	mov bx, _current
	mov sp, 2[bx]		! user stack offset
	mov ax, 4[bx]		! user stack segment
	mov ss, ax
	mov ds, ax
	mov es, ax
	iret			! reloads flags = >reenables interrupts
#endasm
    }
#endif

    panic("iret failed!");
}

/*
 *	Yes its the good old bogomip counter
 */

#ifdef USE_C

static void delay(jiff_t loops)
{
    jiff_t i;
    for (i = loops; i >= 0; i--)
	/* Do nothing */ ;
}

#else

static void delay(jiff_t loops);

#ifndef S_SPLINT_S

/*
 *	The C one just shows bcc isnt always[often] a very good 
 *	compiler. This is a non optimal but fairly passable assembler
 *	bogomips that should be constant over compilers.
 */

#asm
    .text

_delay:

! Create the stack frame

	push bp
	mov bp,sp

! Get the high word

	mov ax,6[bp]

! Delay the higher word

	or ax,ax
	jz  dellow

axlp:
	xor bx,bx

! Delay a complete low word loop time

bxlp:
	dec bx
	jnz bxlp

! Now back around for the next high word

	dec ax
	jnz axlp

! Delay for the low part of the time

dellow:
	mov ax,4[bp]
	or ax,bx
	jz deldone

dellp:
	dec ax
	jnz dellp

! Recover stack frame and return

deldone:
	pop bp
	ret
#endasm

#endif

#endif

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
