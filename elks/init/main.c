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

#include <arch/system.h>
#include <arch/segment.h>

/*
 *	System variable setups
 */
#define ENV		1		/* allow environ variables as bootopts*/
#define DEBUG		0		/* display parsing at boot*/

#define MAX_INIT_ARGS	8
#define MAX_INIT_ENVS	8

#if defined(CONFIG_FS_RO) || defined(CONFIG_ROOT_READONLY)
int root_mountflags = MS_RDONLY;
#else
int root_mountflags = 0;
#endif
int net_irq, net_port;
static int boot_console;
static char bininit[] = "/bin/init";
static char *init_command = bininit;

#ifdef CONFIG_BOOTOPTS
/*
 * Parse /bootopts startup options
 */
static int args, envs;
static int argv_slen;
/* argv_init doubles as sptr data for sys_execv later*/
static char *argv_init[80] = { NULL, bininit, NULL };
#if ENV
static char *envp_init[MAX_INIT_ENVS+1];
#endif
static unsigned char options[OPTSEGSZ];

extern int boot_rootdev;
extern int boot_bufs;
extern int dprintk_on;
static char * INITPROC root_dev_name(int dev);
static int INITPROC parse_options(void);
static void INITPROC finalize_options(void);
static char * INITPROC option(char *s);
#endif

static void init_task(void);
extern int run_init_process(char *cmd);
extern int run_init_process_sptr(char *cmd, char *sptr, int slen);


void start_kernel(void)
{
    kernel_init();

    /* fork and run procedure init_task() as task #1*/
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

void INITPROC kernel_init(void)
{
    seg_t base, end;

    /* sched_init sets us (the current stack) to be idle task #0*/
    sched_init();
    setup_arch(&base, &end);
    mm_init(base, end);
    irq_init();
    tty_init();

#ifdef CONFIG_BOOTOPTS
    /* parse options found in /bootops */
    int opts = parse_options();
#endif

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

    device_init();

#ifdef CONFIG_SOCKET
    sock_init();
#endif

    fs_init();

#ifdef CONFIG_BOOTOPTS
    if (opts)
	finalize_options();
    else printk("/bootopts ignored: header not ##, size > %d or FAT boot\n", OPTSEGSZ-1);
#endif

    mm_stat(base, end);
}

/* this procedure runs in user mode as task 1*/
static void init_task(void)
{
    int num;
    char *s;

    mount_root();

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
    /* special handling if argc/argv array setup*/
    if (argv_init[0]) {
	/* unset special sys_wait4() processing if pid 1 not /bin/init*/
	if (strcmp(init_command, bininit) != 0)
	    current->ppid = 1;		/* turns off auto-child reaping*/

	/* run /bin/init or init= command, normally no return*/
	run_init_process_sptr(init_command, (char *)argv_init, argv_slen);
    } else
#endif
	run_init_process(bininit);

    printk("No init - running /bin/sh\n");
    current->ppid = 1;			/* turns off auto-child reaping*/
    run_init_process("/bin/sh");
    run_init_process("/bin/sash");
    panic("No init or sh found");
}

#ifdef CONFIG_BOOTOPTS
static struct dev_name_struct {
	char *name;
	int num;
} devices[] = {
	/* root_dev_name needs first 5 in order*/
	{ "hda",     0x0300 },
	{ "hdb",     0x0320 },
	{ "hdc",     0x0340 },
	{ "fd0",     0x0380 },
	{ "fd1",     0x03a0 },
	{ "ttyS",    0x0440 },
	{ "tty1",    0x0400 },
	{ "tty2",    0x0401 },
	{ "tty3",    0x0402 },
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
			if (i < 3) {
				if (dev & 0x03) {
					name[NAMEOFF+3] = '0' + (dev & 3);
					name[NAMEOFF+4] = 0;
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

	/* copy /bootops loaded by boot loader at 0050:0000*/
	fmemcpyb(options, kernel_ds, 0, DEF_OPTSEG, sizeof(options));

	/* check file starts with ## and max len 511 bytes*/
	if (*(unsigned short *)options != 0x2323 || options[OPTSEGSZ-1])
		return 0;
#if DEBUG
	printk("/bootopts: %s", &options[3]);
#endif
	args = 2;	/* room for argc and av[0]*/
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
		/*
		 * check for kernel options first..
		 */
		if (!strncmp(line,"root=",5)) {
			int dev = parse_dev(line+5);
#if DEBUG
			printk("root %s=0x%04x\n", line+5, dev);
#endif
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


#if DEBUG
			printk("console %s=0x%04x\n", line+8, dev);
#endif
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
		if (!strncmp(line,"init=",5)) {
			line += 5;
			init_command = argv_init[1] = line;
			continue;
		}
		if (!strncmp(line,"netirq=",7)) {
			net_irq = atoi(line+7);
			continue;
		}
		if (!strncmp(line,"netport=",8)) {
			net_port = (int)simple_strtol(line+8, 16);
			continue;
		}
		if (!strncmp(line,"bufs=",5)) {
			boot_bufs = (int)simple_strtol(line+5, 10);
			continue;
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
	return 1;	/* success*/
}

static void INITPROC finalize_options(void)
{
	int i;

	/* set ROOTDEV environment variable for rc.sys fsck*/
	if (envs < MAX_INIT_ENVS)
		envp_init[envs++] = root_dev_name(ROOT_DEV);

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
	argv_init[0] = (char *)args;                /* 0 = argc*/
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
	for(; *s != ' ' && *s != '\t' && *s != '\n'; ++s, ++t) {
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

char *strchr(char *s, int c)
{
	for(; *s != (char)c; ++s) {
		if (*s == '\0')
			return NULL;
	}
	return s;
}
#endif /* CONFIG_BOOTOPTS*/
