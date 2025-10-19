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
#include <linuxmt/debug.h>
#include <arch/system.h>
#include <arch/segment.h>
#include <arch/ports.h>
#include <arch/irq.h>
#include <arch/io.h>

/*
 *  System variable setups
 */
#define ENV             1       /* allow environ variables as bootopts*/
#define DEBUG           0       /* display parsing at boot*/

#define MAX_INIT_ARGS   6       /* max # arguments to /bin/init or init= program */
#define MAX_INIT_ENVS   12      /* max # environ variables passed to /bin/init */
#define MAX_INIT_SLEN   80      /* max # words of args + environ passed to /bin/init */
#define MAX_UMB         3       /* max umb= segments in /bootopts */

#define ARRAYLEN(a)     (sizeof(a)/sizeof(a[0]))

struct netif_parms netif_parms[MAX_ETHS] = {

    /* NOTE:  The order must match the defines in netstat.h:
     * ETH_NE2K, ETH_WD, ETH_EL3    */
    { NE2K_IRQ, NE2K_PORT, 0, NE2K_FLAGS },
    { WD_IRQ, WD_PORT, WD_RAM, WD_FLAGS },
    { EL3_IRQ, EL3_PORT, 0, EL3_FLAGS },
};
seg_t kernel_cs, kernel_ds;
int root_mountflags;
int tracing;
int nr_ext_bufs, nr_xms_bufs, nr_map_bufs;
int xms_bootopts;
int ata_mode = -1;              /* =AUTO default set ATA CF driver mode automatically */
char running_qemu;
static int boot_console;
static segext_t umbtotal;
static kdev_t disabled[4];      /* disabled devices using disable= */
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
#define STR(x)          __STRING(x)
/* bootopts error message are duplicated below so static here for space */
char errmsg_initargs[] = "init args > " STR(MAX_INIT_ARGS) "\n";
char errmsg_initenvs[] = "init envs > " STR(MAX_INIT_ENVS) "\n";
char errmsg_initslen[] = "init words > " STR(MAX_INIT_SLEN) "\n";

#ifdef CONFIG_SYS_NO_BININIT
static char *argv_init[MAX_INIT_SLEN] = { NULL, binshell, NULL };
#else
/* argv_init doubles as sptr data for sys_execv later*/
static char *argv_init[MAX_INIT_SLEN] = { NULL, bininit, NULL };
#endif
static char hasopts;
static int args = 2;    /* room for argc and av[0] */
static int envs;
static int argv_slen;
#if ENV
static char *envp_init[MAX_INIT_ENVS];
#endif

/* this entire structure is released to kernel heap after /bootopts parsing */
static struct {
    struct umbseg {                     /* save umb= lines during /bootopts parse */
        seg_t base;
        segext_t len;
    } umbseg[MAX_UMB], *nextumb;
    unsigned char options[OPTSEGSZ];    /* near data parsing buffer */
} opts;

extern int boot_rootdev;
static int INITPROC parse_options(void);
static void INITPROC finalize_options(void);
static char * INITPROC option(char *s);
#endif /* CONFIG_BOOTOPTS */

static void INITPROC early_kernel_init(void);
static void INITPROC kernel_init(void);
static void INITPROC kernel_banner(seg_t init, seg_t extra);
static void init_task(void);

/* this procedure called using temp stack then switched, no local vars allowed */
void start_kernel(void)
{
    printk("START\n");
    early_kernel_init();        /* read bootopts using kernel temp stack */
    task = heap_alloc(max_tasks * sizeof(struct task_struct),
        HEAP_TAG_TASK|HEAP_TAG_CLEAR);
    if (!task) panic("No task mem");

    sched_init();               /* set us (the current stack) to be idle task #0*/
    setsp(&task->t_regs.ax);    /* change to idle task stack */
    kernel_init();              /* continue init running on idle task stack */

    /* fork and setup procedure init_task() to run as task #1 on reschedule */
    kfork_proc(init_task);
    wake_up_process(&task[1]);

    /*
     * We are now the idle task. We won't run unless no other process can run.
     * The idle task always runs with _gint_count == 1 (switched from user mode syscall)
     */
    while (1) {
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
    unsigned int heapofs;

    /* Note: no memory allocation available until after heap_init */
    tty_init();                     /* parse_options may call rs_setbaud */
#ifdef CONFIG_TIME_TZ
    tz_init(CONFIG_TIME_TZ);        /* parse_options may call tz_init */
#endif
    ROOT_DEV = SETUP_ROOT_DEV;      /* default root device from boot loader */
#ifdef CONFIG_BOOTOPTS
    opts.nextumb = opts.umbseg;     /* init static structure variables */
    hasopts = parse_options();      /* parse options found in /bootops */
#endif

    /* create near heap at end of kernel bss */
    heap_init();                    /* init near memory allocator */
    heapofs = setup_arch();          /* sets membase and memend globals */
    heap_add((void *)heapofs, heapsize);
    mm_init(membase, memend);       /* init far/main memory allocator */

#ifdef CONFIG_BOOTOPTS
    struct umbseg *p;
    /* now able to add umb memory segments */
    for (p = opts.umbseg; p < &opts.umbseg[MAX_UMB]; p++) {
        if (p->base) {
            debug("umb segment from %x to %x\n", p->base, p->base + p->len);
            seg_add(p->base, p->base + p->len);
            umbtotal += p->len;
        }
    }
#endif
}

static void INITPROC kernel_init(void)
{
    irq_init();                     /* installs timer and div fault handlers */

    /* set console from /bootopts console= or 0=default*/
    set_console(boot_console);
    console_init();                 /* init direct, bios or headless console*/

#ifdef CONFIG_CHAR_DEV_RS
    serial_init();
#endif

    inode_init();
    if (buffer_init())  /* also enables xms and unreal mode if configured and possible*/
        panic("No buf mem");

#ifdef CONFIG_ARCH_IBMPC
    outw(0, 0x510);
    if (inb(0x511) == 'Q' && inb(0x511) == 'E')
        running_qemu = 1;
#endif

#ifdef CONFIG_SOCKET
    sock_init();
#endif

    device_init();                  /* interrupts enabled here for possible disk I/O */

#ifdef CONFIG_BOOTOPTS
    finalize_options();
    if (!hasopts) printk("/bootopts not found or bad format/size\n");
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

    kernel_banner(s, e - s);
}

static void INITPROC kernel_banner(seg_t init, seg_t extra)
{
#ifdef CONFIG_ARCH_IBMPC
    printk("PC/%cT class cpu %d, ", (sys_caps & CAP_PC_AT) ? 'A' : 'X', arch_cpu);
#endif

#ifdef CONFIG_ARCH_PC98
    printk("PC-9801 cpu %d, ", arch_cpu);
#endif

#ifdef CONFIG_ARCH_8018X
    printk("8018X machine, ");
#endif

#ifdef CONFIG_ARCH_SWAN
    printk("WonderSwan, ");
#endif

#ifdef CONFIG_ARCH_SOLO86
    printk("Solo/86 machine, ");
#endif

    printk("syscaps %x, %uK base ram, %d tasks, %d files, %d inodes\n",
        sys_caps, SETUP_MEM_KBYTES, max_tasks, nr_file, nr_inode);
    printk("ELKS %s (%u text, %u ftext, %u data, %u bss, %u heap)\n",
           system_utsname.release,
           (unsigned)_endtext, (unsigned)_endftext, (unsigned)_enddata,
           (unsigned)_endbss - (unsigned)_enddata, heapsize);
    printk("Kernel text %x ", kernel_cs);
#ifdef CONFIG_FARTEXT_KERNEL
    printk("ftext %x init %x ", (unsigned)((long)kernel_init >> 16), init);
#endif
    printk("data %x end %x top %x %u+%u+%uK free\n",
           kernel_ds, membase, memend, (int) ((memend - membase) >> 6),
           extra >> 6, umbtotal >> 6);
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
        sys_dup(num);       /* open stdout*/
        sys_dup(num);       /* open stderr*/
    //}

#ifdef CONFIG_BOOTOPTS
    /* Release options parsing buffers and setup data seg */
    heap_add(&opts, sizeof(opts));
#ifdef CONFIG_FS_XMS
    if (xms_enabled == XMS_LOADALL) {
        seg_add(DEF_OPTSEG, 0x80);  /* carve out LOADALL buf 0x800-0x865 from release! */
        seg_add(0x87, DMASEG);
    } else  /* fall through */
#endif
    seg_add(DEF_OPTSEG, DMASEG);    /* DEF_OPTSEG through REL_INITSEG */

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
    current->ppid = 1;          /* turns off auto-child reaping*/
    try_exec_process(binshell);
    try_exec_process("/bin/sash");
    panic("No init or sh found");
}

/* this procedure runs in user mode as task 1*/
static void init_task(void)
{
    do_init_task();
}

static struct dev_name_struct {
    const char *name;
    int num;
} devices[] = {
	/* the 6 partitionable drives must be first */
	{ "hda",     DEV_HDA },         /* 0 */
	{ "hdb",     DEV_HDB },
	{ "hdc",     DEV_HDC },
	{ "hdd",     DEV_HDD },
	{ "cfa",     DEV_CFA },
	{ "cfb",     DEV_CFB },
	{ "fd0",     DEV_FD0 },         /* 6 */
	{ "fd1",     DEV_FD1 },
	{ "df0",     DEV_DF0 },         /* 8 */
	{ "df1",     DEV_DF1 },
	{ "rom",     DEV_ROM },
	{ "ttyS0",   DEV_TTYS0 },       /* 11 */
	{ "ttyS1",   DEV_TTYS1 },
	{ "tty1",    DEV_TTY1 },
	{ "tty2",    DEV_TTY2 },
	{ "tty3",    DEV_TTY3 },
	{ "tty4",    DEV_TTY4 },
	{ NULL,           0 }
};

/*
 * Convert a root device number to name.
 * Device number could be bios device, not kdev_t.
 */
char *root_dev_name(kdev_t dev)
{
    int i;
    unsigned int mask;
#define NAMEOFF 13
    static char name[18] = "ROOTDEV=/dev/";

    name[8] = '/';
    for (i=0; i<11; i++) {
        mask = (i < 6)? 0xfff8: 0xffff;
        if (devices[i].num == (dev & mask)) {
            strcpy(&name[NAMEOFF], devices[i].name);
            if (i < 6) {
                if (dev & 0x07) {
                    name[NAMEOFF+3] = '0' + (dev & 7);
                    name[NAMEOFF+4] = '\0';
                }
            }
            return name;
        }
    }
    name[8] = '\0';     /* just return "ROOTDEV=" on not found */
    return name;
}

/* return true if device disabled in disable= list */
int INITPROC dev_disabled(int dev)
{
    int i;

    for (i=0; i < ARRAYLEN(disabled); i++)
        if (disabled[i] == dev)
            return 1;
    return 0;
}

#ifdef CONFIG_BOOTOPTS
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
    for (i = 0; i < MAX_SERIAL; i++) {  /* assume decimal digits only */
        m = l;
        while ((*l) && (*l != ',')) l++;
        c = *l;     /* ensure robust eol handling */
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

/* umb= settings have to be saved and processed after parse_options */
static void INITPROC parse_umb(char *line)
{
    char *p = line-1; /* because we start reading at p+1 */
    seg_t base;

    do {
        base = (seg_t)simple_strtol(p+1, 16);
        if((p = strchr(p+1, ':'))) {
            if (opts.nextumb < &opts.umbseg[MAX_UMB]) {
                opts.nextumb->len = (segext_t)simple_strtol(p+1, 16);
                opts.nextumb->base = base;
                opts.nextumb++;
            }
        }
    } while((p = strchr(p+1, ',')));
}

static void INITPROC parse_disable(char *line)
{
    char *p = line;
    kdev_t dev;
    int n = 0;

    do {
        dev = parse_dev(p);
        disabled[n++] = dev;
        p = strchr(p+1, ',');
        if (p)
            *p++ = 0;
    } while (p && n < ARRAYLEN(disabled));
}

/*
 * Boot-time kernel and /bin/init configuration - /bootopts options parser,
 * read early in kernel startup.
 *
 * Known options of the form option=value are handled during kernel init.
 *
 * Unknown options of the same form are saved as var=value and
 * passed as /bin/init's envp array when it runs.
 *
 * Remaining option strings without the character '=' are passed in the order seen
 * as /bin/init's argv array.
 *
 * Note: no memory allocations allowed from this routine.
 */
static int INITPROC parse_options(void)
{
    char *line = (char *)opts.options;
    char *next;

    /* copy /bootopts loaded by boot loader at 0050:0000*/
    fmemcpyb(opts.options, kernel_ds, 0, DEF_OPTSEG, sizeof(opts.options));

#pragma GCC diagnostic ignored "-Wstrict-aliasing"
    /* check file starts with ##, one or two sectors, max 1023 bytes or 511 one sector */
    if (*(unsigned short *)opts.options != 0x2323 ||
        (opts.options[511] && opts.options[OPTSEGSZ-1]))
        return 0;

    next = line;
    while ((line = next) != NULL && *line) {
        if ((next = option(line)) != NULL) {
            if (*line == '#') { /* skip line after comment char*/
                next = line;
                while (*next != '\n' && *next != '\0')
                    next++;
                continue;
            } else *next++ = 0;
        }
        if (*line == 0)     /* skip spaces and linefeeds*/
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
        if (!strncmp(line,"debug=", 6)) {
            debug_level = (int)simple_strtol(line+6, 10);
            continue;
        }
        if (!strncmp(line,"buf=",4)) {
            nr_ext_bufs = (int)simple_strtol(line+4, 10);
            continue;
        }
        if (!strncmp(line,"xms=",4)) {
            if (!strcmp(line+4, "on"))    xms_bootopts = XMS_UNREAL;
            if (!strcmp(line+4, "int15")) xms_bootopts = XMS_INT15;
            continue;
        }
        if (!strncmp(line,"xmsbuf=",7)) {
            nr_xms_bufs = (int)simple_strtol(line+7, 10);
            continue;
        }
        if (!strncmp(line,"xtide=",6)) {
            ata_mode = (int)simple_strtol(line+6, 10);
            continue;
        }
        if (!strncmp(line,"cache=",6)) {
            nr_map_bufs = (int)simple_strtol(line+6, 10);
            continue;
        }
        if (!strncmp(line,"heap=",5)) {
            heapsize = (unsigned int)simple_strtol(line+5, 10);
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
        if (!strncmp(line,"disable=",8)) {
            parse_disable(line+8);
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
            if (args < MAX_INIT_ARGS)
                argv_init[args++] = line;
            else printk(errmsg_initargs);
        }
#if ENV
        else {
            if (envs < MAX_INIT_ENVS)
                envp_init[envs++] = line;
            else printk(errmsg_initenvs);
        }
#endif
    }
    debug("\n");
    return 1;   /* success*/
}

static void INITPROC finalize_options(void)
{
    int i;

#if ENV
    /* set ROOTDEV environment variable for rc.sys fsck*/
    if (envs + running_qemu < MAX_INIT_ENVS) {
        envp_init[envs++] = root_dev_name(ROOT_DEV);
        if (running_qemu)
            envp_init[envs++] = (char *)"QEMU=1";
    } else printk(errmsg_initenvs);
#endif

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
    argv_init[0] = (char *)args;            /* 0 = argc*/
    char *q = (char *)&argv_init[args+2+envs+1];
    for (i=1; i<=args; i++) {               /* 1..argc = av*/
        char *p = argv_init[i];
        char *savq = q;
        while ((*q++ = *p++) != 0)
            ;
        argv_init[i] = (char *)(savq - (char *)argv_init);
    }
    /*argv_init[args+1] = NULL;*/           /* argc+1 = 0*/
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
    if (argv_slen > sizeof(argv_init))
        panic(errmsg_initslen);
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
