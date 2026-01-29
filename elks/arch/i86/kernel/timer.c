#include <linuxmt/config.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/timer.h>
#include <linuxmt/ntty.h>
#include <linuxmt/fixedpt.h>
#include <linuxmt/trace.h>
#include <linuxmt/init.h>
#include <arch/io.h>
#include <arch/irq.h>
#include <arch/param.h>
#include <arch/ports.h>
#include "../drivers/block/ssd.h"   /* for CONFIG_BLK_DEV_SSD_TEST */

/*
 *  Timer tick routine
 *
 * 9/1999 The 100 Hz system timer 0 can configure for variable input
 *        frequency. Christian Mardm"oller (chm@kdt.de)
 */

volatile jiff_t jiffies;
static int spin_on;
static int delaytick;

#ifdef CONFIG_CPU_USAGE
static void FARPROC calc_cpu_usage(void)
{
    static int count = SAMP_FREQ;
    struct task_struct *p;

    current->ticks++;
    if (--count <= 0) {
        count = SAMP_FREQ;
        //unsigned int total = 0;
        for_each_task(p) {
            if (p->state != TASK_UNUSED) {
                CALC_USAGE(p->average, EXP_E, (unsigned long)p->ticks << FSHIFT);
#if DEBUG_CPU_USAGE
                if (p->ticks || p->average) {
                    unsigned long u;
                    printk("pid %d state %d ticks %d ", p->pid, p->state, p->ticks);
                    /* Round up, then divide by 2 for %. Change if SAMP_FREQ not 2 */
                    u = (p->average + FIXED_HALF) >> 1;
                    printk("CPU %d\n", FIXED_INT(u));
                    total += p->ticks;
                }
#endif
                p->ticks = 0;
            }
        }
        //printk("total %d ticks\n", total);
        return;
    }
}
#endif

void timer_tick(int irq, struct pt_regs *regs)
{
    jiffies++;

#ifdef CHECK_ISTACK
    delaytick++;
#endif

    mark_bh(TIMER_BH);

    /***if (!((int) jiffies & 7))
        need_resched = 1;***/       /* how primitive can you get? */

#ifdef CONFIG_ARCH_SWAN
    ack_irq(7);
#endif
}

/* timer tick bottom half, always runs at intr_count == 1 */
void timer_bh(void)
{
    run_timer_list();

#ifdef CHECK_ISTACK
    if (tracing & TRACE_ISTACK) {
        if (delaytick > 1)
            printk("TIMER_BH DELAY %d\n", delaytick);
        delaytick = 0;
    }
#endif

    /*  Test timer_bh delay message and BH reentrancy when running loop program */
    //for (volatile long i=0; i<30000L; i++);

#if (defined(CONFIG_CHAR_DEV_RS) && defined(CONFIG_ARCH_IBMPC)) \
    || defined(CONFIG_FAST_IRQ1_NECV25)
    /* call serial bottom half every 10ms instead of after every byte received */
    serial_bh();        /* process serial input and call wake_up */
#endif

#if defined(CONFIG_BLK_DEV_SSD_TEST) && defined(CONFIG_ASYNCIO)
    if (ssd_timeout && time_after(jiffies, ssd_timeout))
        ssd_io_complete();
#endif

#ifdef CONFIG_CPU_USAGE
    calc_cpu_usage();
#endif

#if defined(CONFIG_CONSOLE_DIRECT) && !defined(CONFIG_ARCH_SWAN)
    /* spin timer wheel in upper right of screen*/
    if (spin_on && !(jiffies & 7)) {
        static unsigned char wheel[4] = {'-', '\\', '|', '/'};
        static int c;

        pokeb((79 + 0*80) * 2, VideoSeg, wheel[++c & 0x03]);
    }
#endif
}

void spin_timer(int onflag)
{
#if defined(CONFIG_CONSOLE_DIRECT) && !defined(CONFIG_ARCH_SWAN)
    if ((spin_on = onflag) == 0)
        pokeb((79 + 0*80) * 2, VideoSeg, ' ');
#endif
}
