#include <linuxmt/prectimer.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <arch/param.h>
#include <arch/ports.h>
#include <arch/irq.h>
#include <arch/io.h>

/*
 * Precision timer routines
 * 2 Aug 2024 Greg Haerr
 */

#define TIMER_TEST      0   /* =1 to include timer_*() test routines */

/*
 * Each ptick corresponds to the elapsed time for a countdown in the 8254 PIT.
 * The PC master oscillator frequency is 14.31818 MHz, and is divided
 * by 12 for the PIT input frequency: 14.31818/12 = 1.1931818 Mhz. Divide
 * that by 1000 and you get 11932 for the PIT countdown start value;
 * 1/11932 = 0.8381 usecs per PIT 'ptick'.
 */
#define MAX_PTICK        11932      /* PIT reload value for 10ms (100 HZ) */

/* error check with master PIT frequency */
#define PITFREQ         11931818L   /* PIT input frequency * 10 */
#define PITTICK         ((5+(PITFREQ/(HZ)))/10)
#if PITTICK != MAX_PTICK
#error Incorrect MAX_PTICK!
#endif

static unsigned long lastjiffies;

/*
 * Each PIT count (ptick) is 0.8381 usecs each for 10ms jiffies timer (= 1/11932)
 *
 * To display a ptick in usecs use ptick * 838 / 1000 (= 1 mul, 1 div )
 *
 * Return type   Name           Resolution  Max
 * unsigned int  get_time_10ms  pticks      10ms  (< 11932 pticks)
 * unsigned int  get_time_50ms  pticks      50ms  (< 5 jiffies = 59660 pticks)
 * unsigned long get_time       pticks      1 hr  (< 359953 jiffies = 2^32 / 11932)
 * unsigned long jiffies        jiffies   497 days(< 2^32 jiffies)
 */

/* read PIT diff count, returns < 11932 pticks (10ms), no overflow checking */
unsigned int get_time_10ms(void)
{
    unsigned int lo, hi, count;
    int pdiff;
    static unsigned int lastcount;

    outb(0, TIMER_CMDS_PORT);       /* latch timer value */
    lo = inb(TIMER_DATA_PORT);
    hi = inb(TIMER_DATA_PORT) << 8;
    count = lo | hi;
    pdiff = lastcount - count;
    if (pdiff < 0)                  /* wrapped */
        pdiff += MAX_PTICK;         /* = MAX_PTICK - count + lastcount */
    lastcount = count;
    return pdiff;
}

/* count up to 5 jiffies in pticks, returns < 59660 pticks (50ms), w/overflow check */
unsigned int get_time_50ms(void)
{
    int jdiff;
    unsigned int pticks;

    clr_irq();
    jdiff = (unsigned)jiffies - (unsigned)lastjiffies;
    lastjiffies = jiffies;          /* 32 bit save required after ~10.9 mins */
    outb(0, TIMER_CMDS_PORT);       /* latch timer value */
    set_irq();

    pticks = get_time_10ms();
    if (jdiff < 0)                  /* lower half wrapped */
        jdiff = -jdiff;             /* = 0x10000000 - lastjiffies + jiffies */
    if (jdiff >= 2) {
        if (jdiff >= 5)
            return 0xffff;          /* overflow */
        return jdiff * 11932U + pticks;
    }
    return pticks;
}

/* return unknown elapsed time in pticks, precision < 50ms then ~10ms accuracy */
unsigned long get_time(void)
{
    unsigned int pticks;
    static unsigned long lasttime;

    clr_irq();                  /* for saving lasttime below */
    lasttime = lastjiffies;
    pticks = get_time_50ms();
    if (pticks != 0xffff)
        return pticks;          /* < 50ms */
    /* current jiffies is in lastjiffies, last jiffies is in lasttime */
    return (lastjiffies - lasttime) * MAX_PTICK;
}

#if TIMER_TEST
/* sample timer routines */
void timer_10ms(void)
{
    unsigned int pticks;

    pticks = get_time_10ms();
    printk("%u %u = %k\n", pticks, (unsigned)jiffies, pticks);
}

void timer_50ms(void)
{
    unsigned long timeout = jiffies + 4;
    unsigned int pticks = get_time_50ms();
    unsigned int usecs = pticks * 838UL / 1000;     /* use unsigned _udivsi3 for speed */
    unsigned int usecs2 = pticks * 8381UL / 10000;  /* use unsigned _udivsi3 for speed */
    printk("%u %u = %u,%u usecs = %k\n", pticks, (unsigned)lastjiffies, usecs, usecs2, pticks);
    while (jiffies < timeout)
        ;
}

void timer_4s(void)
{
    unsigned long timeout = jiffies + 400;
    unsigned long pticks = get_time();
    printk("%lu %u = %lk\n", pticks, (unsigned)lastjiffies, pticks);
    while (jiffies < timeout)
        ;
}

void timer_test(void)
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
