#include <linuxmt/config.h>
#include <linuxmt/timer.h>

/*
 *	Test timer tick routine
 */

unsigned long jiffies=0;

extern void do_timer(struct pt_regs *);
extern void keyboard_irq(int, struct pt_regs *, void *);

void timer_tick(int irq, struct pt_regs *regs, void *data)
{
#ifndef CONFIG_ARCH_SIBO

	do_timer(regs);

#ifdef NEED_RESCHED	/* need_resched is not checked anywhere */
	if (((int)jiffies & 7) == 0)
		need_resched=1;	/* how primitive can you get? */
#endif

#ifndef S_SPLINT_S

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
#endif

#ifdef CONFIG_DEBUG_TIMER
#asm
        mov al,_jiffies
        out 0x80,al 
#endasm
#endif

#endif

#else

	jiffies++;


#ifndef S_SPLINT_S

	/* As we are now responsible for clearing interrupt */
#asm
	cli
	mov ax, #0x0000

	out 0x0a, al
	out 0x0c, al
	
	out 0x10, al
	sti
#endasm

#endif
	
#if 0	

#ifdef NEED_RESCHED	/* need_resched is not checked anywhere */

    if (!((int) jiffies & 7))
	need_resched=1;	/* how primitive can you get? */

#endif

#endif

    keyboard_irq(1, regs, NULL);

#endif
}

#ifdef CONFIG_ARCH_SIBO

void enable_timer_tick(void)
{
#ifndef S_SPLINT_S
#asm
	mov	al, #0x00
	out	0x15, al

	mov	al, #0x02
	out	0x08, al
#endasm
#endif

    printk("Timer enabled...\n");
}

#endif
