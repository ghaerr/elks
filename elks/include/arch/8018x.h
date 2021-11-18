

/* serial, serial-8018x.c */
#define UART0_IRQ_RX  1 /* maps to interrupt type 20 */
#define UART0_IRQ_TX  2 /* maps to interrupt type 21 */

#define CPU_VEC_DIVIDE_ERROR 0
#define CPU_VEC_SINGLE_STEP 1
#define CPU_VEC_NMI 2
#define CPU_VEC_BREAKPOINT 3
#define CPU_VEC_OVERFLOW 4
#define CPU_VEC_ARRAY_BOUNDS 5
#define CPU_VEC_UNUSED_OPCODE 6
#define CPU_VEC_ESC_OPCODE 7
#define CPU_VEC_TIMER0 8
#define CPU_VEC_INT0 12
#define CPU_VEC_INT1 13
#define CPU_VEC_INT2 14
#define CPU_VEC_INT3 15
#define CPU_VEC_NUMERICS 16
#define CPU_VEC_INT4 17
#define CPU_VEC_TIMER1 18
#define CPU_VEC_TIMER2 19
#define CPU_VEC_S0_RX 20
#define CPU_VEC_S0_TX 21

/* INTERRUPT CONTROL REGISTERS */
#define PCB_EOI     0xff02
#define PCB_POLL	0xff04
#define PCB_POLLSTS 0xff06
#define PCB_IMASK   0xff08
#define PCB_PRIMSK  0xff0a
#define PCB_INSERV  0xff0c
#define PCB_REQST   0xff0e
#define PCB_INTSTS  0xff10
#define PCB_TCUCON  0xff12
#define PCB_SCUCON  0xff14
#define PCB_I4CON   0xff16
#define PCB_I0CON   0xff18
#define PCB_I1CON   0xff1a
#define PCB_I2CON   0xff1c
#define PCB_I3CON   0xff1e

/* TIMER CONTROL REGISTERS */
#define PCB_T0CNT   0xff30
#define PCB_T0CMPA  0xff32
#define PCB_T0CMPB  0xff34
#define PCB_T0CON   0xff36
#define PCB_T1CNT   0xff38
#define PCB_T1CMPA  0xff3a
#define PCB_T1CMPB  0xff3c
#define PCB_T1CON   0xff3e
#define PCB_T2CNT   0xff40
#define PCB_T2CMPA  0xff42
#define PCB_T2CON   0xff46

/* INPUT/OUTPUT PORT UNIT REGISTERS */
#define PCB_P1DIR   0xff50
#define PCB_P1PIN   0xff52
#define PCB_P1CON   0xff54
#define PCB_P1LTCH  0xff56
#define PCB_P2DIR   0xff58
#define PCB_P2PIN   0xff5a
#define PCB_P2CON   0xff5c
#define PCB_P2LTCH  0xff5e

/* SERIAL COMMUNICATION UNIT REGISTERS */
#define PCB_B0CMP   0xff60
#define PCB_B0CNT   0xff62
#define PCB_S0CON   0xff64
#define PCB_S0STS   0xff66
#define PCB_R0BUF   0xff68
#define PCB_T0BUF   0xff6a
#define PCB_B1CMP   0xff70
#define PCB_B1CNT   0xff72
#define PCB_S1CON   0xff74
#define PCB_S1STS   0xff76
#define PCB_R1BUF   0xff78
#define PCB_T1BUF   0xff7a

/* CHIP SELECT UNIT REGISTERS */
#define PCB_GCS0ST  0xff80
#define PCB_GCS0SP  0xff82
#define PCB_GCS1ST  0xff84
#define PCB_GCS1SP  0xff86
#define PCB_GCS2ST  0xff88
#define PCB_GCS2SP  0xff8a
#define PCB_GCS3ST  0xff8c
#define PCB_GCS3SP  0xff8e
#define PCB_GCS4ST  0xff90
#define PCB_GCS4SP  0xff92
#define PCB_GCS5ST  0xff94
#define PCB_GCS5SP  0xff96
#define PCB_GCS6ST  0xff98
#define PCB_GCS6SP  0xff9a
#define PCB_GCS7ST  0xff9c
#define PCB_GCS7SP  0xff9e
#define PCB_LCSST   0xffa0
#define PCB_LCSSP   0xffa2
#define PCB_UCSST   0xffa4
#define PCB_UCSSP   0xffa6

/* PERIPHERAL CONTROL BLOCK RELOCATION REGISTER */
#define PCB_RELREG  0xffA8

/* REFRESH CONTROL UNIT REGISTERS */
#define PCB_RFBASE  0xffb0
#define PCB_RFTIME  0xffb2
#define PCB_RFCON   0xffb4
#define PCB_RFADDR  0xffb6

/* POWER MANAGEMENT REGISTERS */
#define PCB_PWRCON  0xffb8

/* STEPPING ID REGISTER */
#define PCB_STEPID  0xffbc
