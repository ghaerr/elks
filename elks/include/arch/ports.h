/*
 * ELKS I/O port and IRQ mappings
 *
 * PC IRQ default mappings (* = possible conflicting uses)
 *
 *  IRQ	ELKS Device         Config Option           Status
 *  0   Timer                                       Required
 *  1   Keyboard            CONFIG_CONSOLE_DIRECT   Optional
 *  2*  Cascade -> 9 on AT  CONFIG_ETH_WD           Optional on XT, not available on AT
 *  3   Com2 (/dev/ttyS1)   CONFIG_CHAR_DEV_RS      Optional
 *  4   Com1 (/dev/ttyS0)   CONFIG_CHAR_DEV_RS      Optional
 *  5*  Unused
 *  5*  Com3 (/devb/ttyS2)  CONFIG_CHAR_DEV_RS      Optional
 *  5*  HW IDE hard drive   CONFIG_BLK_DEV_HD       Driver doesn't work
 *  6*  Unused
 *  6*  HW floppy drive     CONFIG_BLK_DEV_FD       Driver doesn't compile
 *  7   Unused (LPT, Com4)
 *  8   Unused (RTC)
 *  9   3C509/EL3 (/dev/eth) CONFIG_ETH_EL3         Optional
 * 10   Unused (USB)                                Turned off
 * 11   Unused (Sound)                              Turned off
 * 12   NE2K (/dev/eth)     CONFIG_ETH_NE2K         Optional
 * 12*  Unused (Mouse)                              Turned off
 * 13   Unused (Math coproc.)                       Turned off
 * 14*  AT HD IDE (/dev/hda) CONFIG_BLK_DEV_HD      Driver doesn't work
 * 15*  AT HD IDE (/dev/hdb) CONFIG_BLK_DEV_HD      Driver doesn't work
 *
 * Edit settings below to change port address or IRQ:
 *   Change I/O port and driver IRQ number to match your hardware
 */

#ifdef CONFIG_ARCH_IBMPC
/* timer, timer-8254.c*/
#define TIMER_CMDS_PORT 0x43		/* command port */
#define TIMER_DATA_PORT 0x40		/* data port    */
#define TIMER_IRQ	0		/* can't change*/

/* bell, bell-8254.c*/
#define TIMER2_PORT	0x42		/* timer 2 data port for speaker frequency*/
#define SPEAKER_PORT	0x61

/* 8259 interrupt controller, irq-8259.c*/
#define PIC1_CMD   0x20			/* master */
#define PIC1_DATA  0x21
#define PIC2_CMD   0xA0			/* slave */
#define PIC2_DATA  0xA1
#endif

#ifdef CONFIG_ARCH_8018X
#define TIMER_IRQ	0		/* logical IRQ number, NOT related to the actual IRQ vector! */
#endif

#ifdef CONFIG_ARCH_PC98
/* timer, timer-8254.c*/
#define TIMER_CMDS_PORT 0x77		/* command port */
#define TIMER_DATA_PORT 0x71		/* data port    */
#define TIMER_IRQ	0		/* can't change*/

/* bell*/
#define TIMER1_PORT 0x3FDB		/* timer 1 data port for speaker frequency*/
#define PORTC_CONTROL 0x37		/* 8255A Port C control*/

/* serial, serial-pc98.c*/
#define TIMER2_PORT	0x75		/* timer 2 data port for serial port*/

/* 8259 interrupt controller, irq-8259.c*/
#define PIC1_CMD   0x00			/* master */
#define PIC1_DATA  0x02
#define PIC2_CMD   0x08			/* slave */
#define PIC2_DATA  0x0A
#endif

/* keyboard, kbd-scancode.c*/
#define KBD_IO		0x60
#define KBD_CTL		0x61
#define KBD_IRQ		1

/* serial, serial.c*/
#ifdef CONFIG_CHAR_DEV_RS
//#define CONFIG_FAST_IRQ4             /* COM1 very fast serial driver, no ISIG handling*/
//#define CONFIG_FAST_IRQ3             /* COM2 very fast serial driver, no ISIG handling*/
#endif

#ifdef CONFIG_ARCH_PC98
#define COM1_PORT	0x30
#define COM1_IRQ	4		/* unregistered unless COM1_PORT found*/
#else
#define COM1_PORT	0x3f8
#define COM1_IRQ	4		/* unregistered unless COM1_PORT found*/

#define COM2_PORT	0x2f8
#define COM2_IRQ	3		/* unregistered unless COM2_PORT found*/

#define COM3_PORT	0x3e8
#define COM3_IRQ	5		/* unregistered unless COM3_PORT found*/

#define COM4_PORT	0x2e8
#define COM4_IRQ	7		/* unregistered unless COM4_PORT found*/
#endif

/* Ethernet card settings may be overridden in /bootopts using netirq= and netport= */ 
/* ne2k, ne2k.c */ 
#define NE2K_PORT	0x300
#define NE2K_IRQ	12
#define NE2K_FLAGS	0x80

/* wd, wd.c*/
#define WD_PORT		0x240
#define WD_IRQ		2
#define WD_RAM		0xCE00
#define WD_FLAGS	0x80

/* el3/3C509, el3.c */
#define EL3_PORT	0x330
#define EL3_IRQ		11
#define EL3_FLAGS	0x80

/* bioshd.c*/
#define FDC_DOR		0x3F2		/* floppy digital output register*/

/* obsolete - experimental IDE hard drive, directhd.c (broken)*/
#define HD1_PORT	0x1f0
#define HD2_PORT	0x170
#define HD_IRQ		5		/* missing request_irq call*/
#define HD1_AT_IRQ	14		/* missing request_irq call*/
#define HD2_AT_IRQ	15		/* missing request_irq call*/

/* direct floppy driver, directfd.c */
#define FLOPPY_IRQ	6
