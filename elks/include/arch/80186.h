/*
 * Classic 80186 peripheral control block, as used by the Philips :YES.
 * Same FF00 base as the 80C188EB in 8018x.h, different register offsets.
 * Offsets follow Table 4-1 of the 80C186XL user manual for the NMOS
 * 80186/80188 and CMOS 80C186/80C188.
 */

/* INTERRUPT CONTROL REGISTERS */
#define PCB_EOI     0xff22
#define PCB_POLL    0xff24
#define PCB_POLLSTS 0xff26
#define PCB_IMASK   0xff28
#define PCB_PRIMSK  0xff2a
#define PCB_INSERV  0xff2c
#define PCB_REQST   0xff2e
#define PCB_INTSTS  0xff30      /* Intel calls this INSTS; name matches 8018x.h */
#define PCB_TCUCON  0xff32
#define PCB_DMA0CON 0xff34      /* interrupt control for DMA ch0 */
#define PCB_DMA1CON 0xff36      /* interrupt control for DMA ch1 */
#define PCB_I0CON   0xff38
#define PCB_I1CON   0xff3a
#define PCB_I2CON   0xff3c
#define PCB_I3CON   0xff3e

/* TIMER CONTROL REGISTERS */
#define PCB_T0CNT   0xff50
#define PCB_T0CMPA  0xff52
#define PCB_T0CMPB  0xff54
#define PCB_T0CON   0xff56
#define PCB_T1CNT   0xff58
#define PCB_T1CMPA  0xff5a
#define PCB_T1CMPB  0xff5c
#define PCB_T1CON   0xff5e
#define PCB_T2CNT   0xff60
#define PCB_T2CMPA  0xff62
#define PCB_T2CON   0xff66

/* CHIP SELECT UNIT REGISTERS */
#define PCB_UMCS    0xffa0
#define PCB_LMCS    0xffa2
#define PCB_PACS    0xffa4
#define PCB_MMCS    0xffa6
#define PCB_MPCS    0xffa8

/* DMA REGISTERS */
#define PCB_D0SRCL  0xffc0
#define PCB_D0SRCH  0xffc2
#define PCB_D0DSTL  0xffc4
#define PCB_D0DSTH  0xffc6
#define PCB_D0TC    0xffc8
#define PCB_D0CON   0xffca      /* DMA ch0 channel control register */
#define PCB_D1SRCL  0xffd0
#define PCB_D1SRCH  0xffd2
#define PCB_D1DSTL  0xffd4
#define PCB_D1DSTH  0xffd6
#define PCB_D1TC    0xffd8
#define PCB_D1CON   0xffda      /* DMA ch1 channel control register */

/* DMA interrupt types; the 80C188EB has a serial unit here instead */
#define CPU_VEC_DMA0 10
#define CPU_VEC_DMA1 11

/* CMOS ONLY (80C186/80C188 and later); reserved on the NMOS 80186 */
#define PCB_RFBASE  0xffe0
#define PCB_RFTIME  0xffe2
#define PCB_RFCON   0xffe4
#define PCB_PWRSAV  0xfff0
#define PCB_PWRCON  0xfff2
#define PCB_STEPID  0xfff6

/* PERIPHERAL CONTROL BLOCK RELOCATION REGISTER */
#define PCB_RELREG  0xfffe
