#include <linuxmt/config.h>
#include <linuxmt/init.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/types.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/ntty.h>
#include <linuxmt/memory.h>
#include <linuxmt/kernel.h>
#include <linuxmt/string.h>
#include <linuxmt/fs.h>
#include <linuxmt/utsname.h>
#include <linuxmt/netstat.h>
#include <linuxmt/trace.h>
#include <linuxmt/devnum.h>
#include <linuxmt/heap.h>
#include <linuxmt/prectimer.h>
#include <arch/system.h>
#include <arch/segment.h>
#include <arch/ports.h>
#include <arch/irq.h>
#include <arch/io.h>

/*
 *	System variable setups
 */
#define ENV             1       /* allow environ variables as bootopts*/
#define DEBUG           0       /* display parsing at boot*/

#include <linuxmt/debug.h>

#define MAX_INIT_ARGS	8
#define MAX_INIT_ENVS	8

int root_mountflags;
struct netif_parms netif_parms[MAX_ETHS] = {
    /* NOTE:  The order must match the defines in netstat.h:
     * ETH_NE2K, ETH_WD, ETH_EL3    */
    { NE2K_IRQ, NE2K_PORT, 0, NE2K_FLAGS },
    { WD_IRQ, WD_PORT, WD_RAM, WD_FLAGS },
    { EL3_IRQ, EL3_PORT, 0, EL3_FLAGS },
};
seg_t kernel_cs, kernel_ds;
int tracing;
int nr_ext_bufs, nr_xms_bufs, nr_map_bufs;
char running_qemu;
static int boot_console;
static seg_t membase, memend;
static char bininit[] = "/bin/init";
static char binshell[] = "/bin/sh";
#ifdef CONFIG_SYS_NO_BININIT
static char *init_command = binshell;
#else
static char *init_command = bininit;
#endif

#ifdef CONFIG_BOOTOPTS
/*
 * Parse /bootopts startup options
 */
static char opts;
static int args = 2;	/* room for argc and av[0] */
static int envs;
static int argv_slen;
#ifdef CONFIG_SYS_NO_BININIT
static char *argv_init[80] = { NULL, binshell, NULL };
#else
/* argv_init doubles as sptr data for sys_execv later*/
static char *argv_init[80] = { NULL, bininit, NULL };
#endif
#if ENV
static char *envp_init[MAX_INIT_ENVS+1];
#endif
static unsigned char options[OPTSEGSZ];

extern int boot_rootdev;
static char * INITPROC root_dev_name(int dev);
static int INITPROC parse_options(void);
static void INITPROC finalize_options(void);
static char * INITPROC option(char *s);

#endif

static void init_task(void);
static void INITPROC kernel_banner(seg_t start, seg_t end, seg_t init, seg_t extra);
static void INITPROC early_kernel_init(void);

/* this procedure called using temp stack then switched, no temp vars allowed */
void start_kernel(void)
{
    early_kernel_init();        /* read bootopts using kernel interrupt stack */
    task = heap_alloc(max_tasks * sizeof(struct task_struct),
        HEAP_TAG_TASK|HEAP_TAG_CLEAR);
    if (!task) panic("No task mem");
    setsp(&task->t_regs.ax);    /* change to idle task stack */
    kernel_init();              /* continue init running on idle task stack */

    /* fork and run procedure init_task() as task #1*/
    kfork_proc(init_task);
    wake_up_process(&task[1]);

    /*
     * We are now the idle task. We won't run unless no other process can run.
     */
    while (1) {
        /***unsigned int pticks = get_time_10ms();
        printk("%u,%u = %k\n", pticks, (unsigned)jiffies, pticks);***/

        schedule();
#ifdef CONFIG_TIMER_INT0F
        int0F();        /* simulate timer interrupt hooked on IRQ 7 */
#else
        idle_halt();    /* halt until interrupt to save power */
#endif
    }
}

static void INITPROC early_kernel_init(void)
{
    setup_arch(&membase, &memend);  /* initializes kernel heap */
    mm_init(membase, memend);       /* parse_options may call seg_add */
    tty_init();                     /* parse_options may call rs_setbaud */
#ifdef CONFIG_TIME_TZ
    tz_init(CONFIG_TIME_TZ);        /* parse_options may call tz_init */
#endif
#ifdef CONFIG_BOOTOPTS
    opts = parse_options();         /* parse options found in /bootops */
#endif
}

void INITPROC kernel_init(void)
{
    /* set us (the current stack) to be idle task #0*/
    sched_init();
    irq_init();

    /* set console from /bootopts console= or 0=default*/
    set_console(boot_console);

    /* init direct, bios or headless console*/
    console_init();

#ifdef CONFIG_CHAR_DEV_RS
    serial_init();
#endif

    inode_init();
    if (buffer_init())	/* also enables xms and unreal mode if configured and possible*/
        panic("No buf mem");

#ifdef CONFIG_ARCH_IBMPC
    outw(0, 0x510);
    if (inb(0x511) == 'Q' && inb(0x511) == 'E')
        running_qemu = 1;
#endif

    device_init();

#ifdef CONFIG_SOCKET
    sock_init();
#endif

    fs_init();

#ifdef CONFIG_BOOTOPTS
    finalize_options();
    if (!opts) printk("/bootopts not found or bad format/size\n");
#endif

#ifdef CONFIG_FARTEXT_KERNEL
    /* add .farinit.init section to main memory free list */
    seg_t     init_seg = ((unsigned long)(void __far *)__start_fartext_init) >> 16;
    seg_t s = init_seg + (((word_t)(void *)__start_fartext_init + 15) >> 4);
    seg_t e = init_seg + (((word_t)(void *)  __end_fartext_init + 15) >> 4);
    debug("init: seg %04x to %04x size %04x (%d)\n", s, e, (e - s) << 4, (e - s) << 4);
    seg_add(s, e);
#else
    seg_t s = 0, e = 0;
#endif

    kernel_banner(membase, memend, s, e - s);
}

static void INITPROC kernel_banner(seg_t start, seg_t end, seg_t init, seg_t extra)
{
#ifdef CONFIG_ARCH_IBMPC
    printk("PC/%cT class machine, ", (sys_caps & CAP_PC_AT) ? 'A' : 'X');
#endif

#ifdef CONFIG_ARCH_PC98
    printk("PC-9801 machine, ");
#endif

#ifdef CONFIG_ARCH_8018X
    printk("8018X machine, ");
#endif

    printk("syscaps %x, %uK base ram\n", sys_caps, SETUP_MEM_KBYTES);
    printk("ELKS %s (%u text, %u ftext, %u data, %u bss, %u heap)\n",
           system_utsname.release,
           (unsigned)_endtext, (unsigned)_endftext, (unsigned)_enddata,
           (unsigned)_endbss - (unsigned)_enddata, heapsize);
    printk("Kernel text %x:0, ", kernel_cs);
#ifdef CONFIG_FARTEXT_KERNEL
    printk("ftext %x:0, init %x:0, ", (unsigned)((long)kernel_init >> 16), init);
#endif
    printk("data %x:0, top %x:0, %uK free\n",
           kernel_ds, end, (int) ((end - start + extra) >> 6));
}

static void INITPROC try_exec_process(const char *path)
{
    int num;

    num = run_init_process(path);
    if (num) printk("Can't run %s, errno %d\n", path, num);
}

static void INITPROC do_init_task(void)
{
    int num;
    const char *s;

    mount_root();

#ifdef CONFIG_SYS_NO_BININIT
    /* when no /bin/init, force initial process group on console to make signals work*/
    current->session = current->pgrp = 1;
#endif

    /* Don't open /dev/console for /bin/init, 0-2 closed immediately and fragments heap*/
    //if (strcmp(init_command, bininit) != 0) {
        /* Set stdin/stdout/stderr to /dev/console if not running /bin/init*/
        num = sys_open(s="/dev/console", O_RDWR, 0);
        if (num < 0)
            printk("Unable to open %s (error %d)\n", s, num);
        sys_dup(num);		/* open stdout*/
        sys_dup(num);		/* open stderr*/
    //}

#ifdef CONFIG_BOOTOPTS
    /* pass argc/argv/env array to init_command */

    /* unset special sys_wait4() processing if pid 1 not /bin/init*/
    if (strcmp(init_command, bininit) != 0)
        current->ppid = 1;      /* turns off auto-child reaping*/

    /* run /bin/init or init= command, normally no return*/
    run_init_process_sptr(init_command, (char *)argv_init, argv_slen);
#else
    try_exec_process(init_command);
#endif /* CONFIG_BOOTOPTS */

    printk("No init - running %s\n", binshell);
    current->ppid = 1;			/* turns off auto-child reaping*/
    try_exec_process(binshell);
    try_exec_process("/bin/sash");
    panic("No init or sh found");
}

/* this procedure runs in user mode as task 1*/
static void init_task(void)
{
    do_init_task();
}

#ifdef CONFIG_BOOTOPTS
static struct dev_name_struct {
	const char *name;
	int num;
} devices[] = {
	/* the 4 partitionable drives must be first */
	{ "hda",     DEV_HDA },
	{ "hdb",     DEV_HDB },
	{ "hdc",     DEV_HDC },
	{ "hdd",     DEV_HDD },
	{ "fd0",     DEV_FD0 },
	{ "fd1",     DEV_FD1 },
	{ "df0",     DEV_DF0 },
	{ "df1",     DEV_DF1 },
	{ "ttyS0",   DEV_TTYS0 },
	{ "ttyS1",   DEV_TTYS1 },
	{ "tty1",    DEV_TTY1 },
	{ "tty2",    DEV_TTY2 },
	{ "tty3",    DEV_TTY3 },
	{ NULL,           0 }
};

/*
 * Convert a root device number to name.
 * Device number could be bios device, not kdev_t.
 */
static char * INITPROC root_dev_name(int dev)
{
	int i;
#define NAMEOFF	13
	static char name[18] = "ROOTDEV=/dev/";

	for (i=0; i<5; i++) {
		if (devices[i].num == (dev & 0xfff0)) {
			strcpy(&name[NAMEOFF], devices[i].name);
			if (i < 4) {
				if (dev & 0x07) {
					name[NAMEOFF+3] = '0' + (dev & 7);
					name[NAMEOFF+4] = '\0';
				}
			}
			return name;
		}
	}
	return NULL;
}

/*
 * Convert a /dev/ name to device number.
 */
static int INITPROC parse_dev(char * line)
{
	int base = 0;
	struct dev_name_struct *dev = devices;

	if (strncmp(line,"/dev/",5) == 0)
		line += 5;
	do {
		int len = strlen(dev->name);
		if (strncmp(line,dev->name,len) == 0) {
			line += len;
			base = dev->num;
			break;
		}
		dev++;
	} while (dev->name);
	return (base + atoi(line));
}

static void INITPROC comirq(char *line)
{
#if defined(CONFIG_ARCH_IBMPC) && defined(CONFIG_CHAR_DEV_RS)
	int i;
	char *l, *m, c;

	l = line;
	for (i = 0; i < MAX_SERIAL; i++) {	/* assume decimal digits only */
		m = l;
		while ((*l) && (*l != ',')) l++;
		c = *l;		/* ensure robust eol handling */
		if (l > m) {
			*l = '\0';
			set_serial_irq(i, (int)simple_strtol(m, 0));
		}
		if (!c) break;
		l++;
	}
#endif
}

static void INITPROC parse_nic(char *line, struct netif_parms *parms)
{
    char *p;

    parms->irq = (int)simple_strtol(line, 0);
    if ((p = strchr(line, ','))) {
        parms->port = (int)simple_strtol(p+1, 16);
        if ((p = strchr(p+1, ','))) {
            parms->ram = (int)simple_strtol(p+1, 16);
            if ((p = strchr(p+1, ',')))
                parms->flags = (int)simple_strtol(p+1, 0);
        }
    }
}

static void INITPROC parse_umb(char *line)
{
	char *p = line-1; /* because we start reading at p+1 */
	seg_t base, end;
	segext_t len;

	do {
		base = (seg_t)simple_strtol(p+1, 16);
		if((p = strchr(p+1, ':'))) {
			len = (segext_t)simple_strtol(p+1, 16);
			end = base + len;
			debug("umb segment from %x to %x\n", base, end);
			seg_add(base, end);
		}
	} while((p = strchr(p+1, ',')));
}

/*
 * This is a simple kernel command line parsing function: it parses
 * the command line from /bootopts, and fills in the arguments/environment
 * to init as appropriate. Any cmd-line option is taken to be an environment
 * variable if it contains the character '='.
 *
 * This routine also checks for options meant for the kernel.
 * These options are not given to init - they are for internal kernel use only.
 */
static int INITPROC parse_options(void)
{
	char *line = (char *)options;
	char *next;

	/* copy /bootopts loaded by boot loader at 0050:0000*/
	fmemcpyb(options, kernel_ds, 0, DEF_OPTSEG, sizeof(options));

#pragma GCC diagnostic ignored "-Wstrict-aliasing"
	/* check file starts with ## and max len 511 bytes*/
	if (*(unsigned short *)options != 0x2323 || options[OPTSEGSZ-1])
		return 0;

	next = line;
	while ((line = next) != NULL && *line) {
		if ((next = option(line)) != NULL) {
			if (*line == '#') {	/* skip line after comment char*/
				next = line;
				while (*next != '\n' && *next != '\0')
					next++;
				continue;
			} else *next++ = 0;
		}
		if (*line == 0)		/* skip spaces and linefeeds*/
			continue;
		debug("'%s',", line);
		/*
		 * check for kernel options first..
		 */
		if (!strncmp(line,"root=",5)) {
			int dev = parse_dev(line+5);
			debug("root %s=%D\n", line+5, dev);
			ROOT_DEV = (kdev_t)dev;
			boot_rootdev = dev;    /* stop translation in device_setup*/
			continue;
		}
		if (!strncmp(line,"console=",8)) {
			int dev = parse_dev(line+8);
			char *p = strchr(line+8, ',');
			if (p) {
				*p++ = 0;
#ifdef CONFIG_CHAR_DEV_RS
				/* set serial console baud rate*/
				rs_setbaud(dev, simple_strtol(p, 10));
#endif
			}


			debug("console %s=%D,", line+8, dev);
			boot_console = dev;
			continue;
		}
		if (!strcmp(line,"ro")) {
			root_mountflags |= MS_RDONLY;
			continue;
		}
		if (!strcmp(line,"rw")) {
			root_mountflags &= ~MS_RDONLY;
			continue;
		}
		if (!strcmp(line,"debug")) {
			dprintk_on = 1;
			continue;
		}
		if (!strcmp(line,"strace")) {
			tracing |= TRACE_STRACE;
			continue;
		}
		if (!strcmp(line,"kstack")) {
			tracing |= TRACE_KSTACK;
			continue;
		}
		if (!strncmp(line,"init=",5)) {
			line += 5;
			init_command = argv_init[1] = line;
			continue;
		}
		if (!strncmp(line,"ne0=",4)) {
			parse_nic(line+4, &netif_parms[ETH_NE2K]);
			continue;
		}
		if (!strncmp(line,"wd0=",4)) {
			parse_nic(line+4, &netif_parms[ETH_WD]);
			continue;
		}
		if (!strncmp(line,"3c0=",4)) {
			parse_nic(line+4, &netif_parms[ETH_EL3]);
			continue;
		}
		if (!strncmp(line,"buf=",4)) {
			nr_ext_bufs = (int)simple_strtol(line+4, 10);
			continue;
		}
		if (!strncmp(line,"xmsbuf=",7)) {
			nr_xms_bufs = (int)simple_strtol(line+7, 10);
			continue;
		}
		if (!strncmp(line,"cache=",6)) {
			nr_map_bufs = (int)simple_strtol(line+6, 10);
			continue;
		}
		if (!strncmp(line,"task=",5)) {
			max_tasks = (int)simple_strtol(line+5, 10);
			continue;
		}
		if (!strncmp(line,"inode=",6)) {
			nr_inode = (int)simple_strtol(line+6, 10);
			continue;
		}
		if (!strncmp(line,"file=",5)) {
			nr_file = (int)simple_strtol(line+5, 10);
			continue;
		}
		if (!strncmp(line,"comirq=",7)) {
			comirq(line+7);
			continue;
		}
		if (!strncmp(line,"umb=",4)) {
			parse_umb(line+4);
			continue;
		}
		if (!strncmp(line,"TZ=",3)) {
			tz_init(line+3);
			/* fall through and add line to environment */
		}
		
		/*
		 * Then check if it's an environment variable or an init argument.
		 */
		if (!strchr(line,'=')) {    /* no '=' means init argument*/
			if (args >= MAX_INIT_ARGS)
				break;
			argv_init[args++] = line;
		}
#if ENV
		else {
			if (envs >= MAX_INIT_ENVS)
				break;
			envp_init[envs++] = line;
		}
#endif
	}
	debug("\n");
	return 1;	/* success*/
}

static void INITPROC finalize_options(void)
{
	int i;

	/* set ROOTDEV environment variable for rc.sys fsck*/
	if (envs < MAX_INIT_ENVS)
		envp_init[envs++] = root_dev_name(ROOT_DEV);
	if (running_qemu && envs < MAX_INIT_ENVS)
		envp_init[envs++] = (char *)"QEMU=1";

#if DEBUG
	printk("args: ");
	for (i=1; i<args; i++)
		printk("'%s'", argv_init[i]);
	printk("\n");

#if ENV
	printk("envp: ");
	for (i=0; i<envs; i++)
		printk("'%s'", envp_init[i]);
	printk("\n");
#endif
#endif

	/* convert argv array to stack array for sys_execv*/
	args--;
	argv_init[0] = (char *)args;        	/* 0 = argc*/
	char *q = (char *)&argv_init[args+2+envs+1];
	for (i=1; i<=args; i++) {                   /* 1..argc = av*/
		char *p = argv_init[i];
		char *savq = q;
		while ((*q++ = *p++) != 0)
			;
		argv_init[i] = (char *)(savq - (char *)argv_init);
	}
	/*argv_init[args+1] = NULL;*/               /* argc+1 = 0*/
#if ENV
	if (envs) {
		for (i=0; i<envs; i++) {
			char *p = envp_init[i];
			char *savq = q;
			while ((*q++ = *p++) != 0)
				;
			argv_init[args+2+i] = (char *)(savq - (char *)argv_init);
		}

	}
#endif
	/*argv_init[args+2+envs] = NULL;*/
	argv_slen = q - (char *)argv_init;
}

/* return whitespace-delimited string*/
static char * INITPROC option(char *s)
{
	char *t = s;
	if (*s == '#')
		return s;
	for(; *s != ' ' && *s != '\t' && *s != '\r' && *s != '\n'; ++s, ++t) {
		if (*s == '\0')
			return NULL;
		if (*s == '"') {
			s++;
			while (*s != '"') {
				if (*s == '\0')
					return NULL;
				*t++ = *s++;
			}
			*t++ = 0;
			break;
		}
	}
	return s;
}
#endif /* CONFIG_BOOTOPTS*/
