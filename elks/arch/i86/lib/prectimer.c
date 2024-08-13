#include <linuxmt/prectimer.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <arch/param.h>
#include <arch/ports.h>
#include <arch/irq.h>
#include <arch/io.h>

/*
 * Precision timer routines for IBM PC and compatibles
 * 2 Aug 2024 Greg Haerr
 *
 * Use 8254 PIT to measure elapsed time in pticks = 0.8381 usecs.
 */

/*
 * Each ptick corresponds to the elapsed time for a countdown in the 8254 PIT.
 * The PC master oscillator frequency is 14.31818 MHz, and is divided
 * by 12 for the PIT input frequency: 14.31818/12 = 1.1931818 Mhz. Divide
 * that by 1000 and you get 11932 for the PIT countdown start value;
 * 1/11932 = 0.8381 usecs per PIT 'ptick'.
 */
#define MAX_PTICK        11932U     /* PIT reload value for 10ms (100 HZ) */

/* error check with master PIT frequency */
#define PITFREQ         11931818L   /* PIT input frequency * 10 */
#define PITTICK         ((5+(PITFREQ/(HZ)))/10)
#if PITTICK != MAX_PTICK
#error Incorrect MAX_PTICK!
#endif

static unsigned int lastjiffies;    /* only 16 bits required within ~10.9 mins */

/*
 * Each PIT count (ptick) is 0.8381 usecs each for 10ms jiffies timer (= 1/11932)
 *
 * To display a ptick in usecs use ptick * 838 / 1000U (= 1 mul, 1 div )
 *
 * Return type   Name           Resolution  Max
 * unsigned long get_ptime      0.838us     42.85s (uncaught overflow at ~10.9 mins)
 * unsigned long jiffies        10ms        497 days (=2^32 jiffies)
 */

/* return up to 42.85 seconds in pticks, return 0 if overflow, no check > ~10.9 mins */
unsigned long get_ptime(void)
{
    unsigned int pticks, jdiff;
    unsigned int lo, hi, count;
    flag_t flags;
    static unsigned int lastcount;

    save_flags(flags);
    clr_irq();                      /* synchronize countdown and jiffies */
    outb(0, TIMER_CMDS_PORT);       /* latch timer value */
    /* 16-bit subtract handles low word wrap automatically */
    jdiff = (unsigned)jiffies - (unsigned)lastjiffies;
    lastjiffies = (unsigned)jiffies; /* 16 bit save works for ~10.9 mins */
    restore_flags(flags);

    lo = inb(TIMER_DATA_PORT);
    hi = inb(TIMER_DATA_PORT) << 8;
    count = lo | hi;
    pticks = lastcount - count;
    if ((int)pticks < 0)            /* wrapped */
        pticks += MAX_PTICK;        /* = MAX_PTICK - count + lastcount */
    lastcount = count;

    if (jdiff < 2)                  /* < 10ms: 1..11931 */
        return pticks;
    if (jdiff < 4286)               /* < ~42.86s */
        return (jdiff - 1) * (unsigned long)MAX_PTICK + pticks;
    return 0;                       /* overflow displays 0s */
}

#if TIMER_TEST
void test_ptime_idle_loop(void)
{
    static int v;
    unsigned long timeout = jiffies + v;
    unsigned long pticks = get_ptime();
    printk("%lu %u = %lk\n", pticks, (unsigned)lastjiffies, pticks);
    if (++v > 5) v = 0;
    /* idle_halt() must be commented out to vary timings */
    while (jiffies < timeout)
        ;
}

void test_ptime_print(void)
{
    printk("1 = %k\n", 1);
    printk("2 = %k\n", 2);
    printk("3 = %k\n", 3);
    printk("100 = %k\n", 100);
    printk("500 = %k\n", 500);
    printk("1000 = %k\n", 1000);
    printk("1192 = %k\n", 1192);
    printk("1193 = %k\n", 1193);
    printk("1194 = %k\n", 1194);
    printk("10000 = %k\n", 10000);
    printk("59659 = %k\n", 59659U);
    printk("59660 = %k\n", 59660U);
    printk("59661 = %k\n", 59661U);
    printk("100000 = %lk\n", 100000L);
    printk("5*59660 = %lk\n", 5*59660L);
    printk("359953 = %lk\n", 359953L);
    printk("19*59660 = %lk\n", 19*59660L);
    printk("20*59660 = %lk\n", 20*59660L);
    printk("21*59660 = %lk\n", 21*59660L);
    printk("60*59660 = %lk\n", 60*59660L);
    printk("84*59660 = %lk\n", 84*59660L);
    printk("400*11932 = %lk\n", 400*11932L);
    printk("500*11932 = %lk\n", 500*11932L);
    printk("600*11932 = %lk\n", 600*11932L);
    printk("900*11932 = %lk\n", 900*11932L);
    printk("1000*11932 = %lk\n", 1000*11932L);
    printk("600*59660 = %lk\n", 600*59660L);
    printk("3000000 = %lk\n", 3000000L);
    printk("30000000 = %lk\n", 30000000UL);
    printk("35995300 = %lk\n", 35995300UL);
    printk("36000000 = %lk\n", 36000000UL);
    printk("51000000 = %lk\n", 51000000UL);
    printk("51130563 = %lk\n", 51130563UL);
    printk("51130564 = %lk\n", 51130564UL);
}
#endif
