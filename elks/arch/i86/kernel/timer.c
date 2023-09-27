#include <linuxmt/config.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/timer.h>
#include <linuxmt/ntty.h>
#include <linuxmt/fixedpt.h>

#include <arch/io.h>
#include <arch/irq.h>
#include <arch/param.h>
#include <arch/ports.h>

/*
 *  Timer tick routine
 *
 * 9/1999 The 100 Hz system timer 0 can configure for variable input
 *        frequency. Christian Mardm"oller (chm@kdt.de)
 */

volatile jiff_t jiffies = 0;
static int spin_on;

extern void rs_pump(void);

#ifdef CONFIG_CPU_USAGE
jiff_t uptime;

static void calc_cpu_usage(void)
{
    static int count = SAMP_FREQ;
    struct task_struct *p;

    current->ticks++;
    uptime++;
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

#if defined(CONFIG_BLK_DEV_SSD_TEST) && defined(CONFIG_ASYNCIO)
extern jiff_t ssd_timeout;
extern void ssd_io_complete();
#endif

void timer_tick(int irq, struct pt_regs *regs)
{
    do_timer();

#ifdef CONFIG_CPU_USAGE
    calc_cpu_usage();
#endif

#if defined(CONFIG_CHAR_DEV_RS) && (defined(CONFIG_FAST_IRQ4) || defined(CONFIG_FAST_IRQ3))
    rs_pump();          /* check if received serial chars and call wake_up*/
#endif

#if defined(CONFIG_BLK_DEV_SSD_TEST) && defined(CONFIG_ASYNCIO)
    if (ssd_timeout && jiffies >= ssd_timeout) {
        ssd_io_complete();
    }
#endif

#ifdef CONFIG_CONSOLE_DIRECT
    /* spin timer wheel in upper right of screen*/
    if (spin_on && !(jiffies & 7)) {
        static unsigned char wheel[4] = {'-', '\\', '|', '/'};
        static int c = 0;

        pokeb((79 + 0*80) * 2, VideoSeg, wheel[c++ & 0x03]);
    }
#endif
}

void spin_timer(int onflag)
{
#ifdef CONFIG_CONSOLE_DIRECT
    if ((spin_on = onflag) == 0)
        pokeb((79 + 0*80) * 2, VideoSeg, ' ');
#endif
}
