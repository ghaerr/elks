#include <linuxmt/config.h>
#include <linuxmt/timer.h>

/*
 *	Test timer tick routine
 */

unsigned long jiffies=0;

#ifndef CONFIG_ARCH_SIBO
void timer_tick(/*struct pt_regs * regs*/)
{
	do_timer(/*regs*/); 

#ifdef NEED_RESCHED	/* need_resched is not checked anywhere */
	if (((int)jiffies & 7) == 0)
		need_resched=1;	/* how primitive can you get? */
#endif

#if 0
#asm
	! rotate the 20th character on the 3rd screen line
	push es
	mov ax,#0xb80a
	mov es,ax
	seg es
	inc 40
	pop es
#endasm
#endif /* 0 */
#ifdef CONFIG_DEBUG_TIMER
        #asm
        mov al,_jiffies
        out 0x80,al 
        #endasm

#endif /* CONFIG_DEBUG_TIMER */
}
#else /* CONFIG_ARCH_SIBO */
void timer_tick(/*struct pt_regs * regs*/)
{
	jiffies++;

	/* As We're now responsible for clearing interrupt */
#asm
	cli
	mov ax, #0x0000

	out 0x0a, al
	out 0x0c, al
	
	out 0x10, al
	sti
#endasm
	
#if 0	
#ifdef NEED_RESCHED	/* need_resched is not checked anywhere */
	if (((int)jiffies & 7) == 0)
		need_resched=1;	/* how primitive can you get? */
#endif
#endif

	keyboard_irq();
}

void enable_timer_tick()
{
#asm
	mov al, #0x00
	out 0x15, al
	
	mov al, #0x02
	out 0x08, al
#endasm
	printk("Timer enabled...\n");
}
#endif /* CONFIG_ARCH_SIBO */
