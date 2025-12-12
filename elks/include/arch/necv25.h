/*
 * NEC V25 Hardware specific I/O register definitions
 *
 * 22. Oct. 2025 swausd
 */

#define NEC_HW_SEGMENT 0xf000

// System Control
#define NEC_RFM   0xffe1      /* Refresh mode register */
#define NEC_WTC   0xffe8      /* Wait Control Register */
#define NEC_PRC   0xffeb      /* Processor control register */

// Serial Interface 0
#define NEC_RXB0  0xff60      /* Receive buffer register 0 */
#define NEC_TXB0  0xff62      /* Transmit buffer register 0 */
#define NEC_SRMS0 0xff65      /* Serial reception macro service control register 0 */
#define NEC_STMS0 0xff66      /* Serial transmission macro service control register 0 */
#define NEC_SCM0  0xff68      /* Serial mode register 0 */
#define NEC_SCC0  0xff69      /* Serial control register 0 */
#define NEC_BRG0  0xff6a      /* Baud rate generator register 0 */
#define NEC_SCE0  0xff6b      /* Serial error register 0 */
#define NEC_SEIC0 0xff6c      /* Serial error interrupt request control register 0 */
#define NEC_SRIC0 0xff6d      /* Serial reception interrupt request control register 0 */
#define NEC_STIC0 0xff6e      /* Serial transmission interrupt request control register 0 */

// Serial Interface 1
#define NEC_RXB1  0xff70      /* Receive buffer register 1 */
#define NEC_TXB1  0xff72      /* Transmit buffer register 1 */
#define NEC_SRMS1 0xff75      /* Serial reception macro service control register 1 */
#define NEC_STMS1 0xff76      /* Serial transmission macro service control register 1 */
#define NEC_SCM1  0xff78      /* Serial mode register 1 */
#define NEC_SCC1  0xff79      /* Serial control register 1 */
#define NEC_BRG1  0xff7a      /* Baud rate generator register 1 */
#define NEC_SCE1  0xff7b      /* Serial error register 1 */
#define NEC_SEIC1 0xff7c      /* Serial error interrupt request control register 1 */
#define NEC_SRIC1 0xff7d      /* Serial reception interrupt request control register 1 */
#define NEC_STIC1 0xff7e      /* Serial transmission interrupt request control register 1 */

// Timer Unit
#define NEC_TM0   0xff80      /* Timer register 0 */
#define NEC_MD0   0xff82      /* Modulo/timer register 0 */
#define NEC_TM1   0xff88      /* Timer register 1 */
#define NEC_MD1   0xff8a      /* Modulo/timer register 1 */
#define NEC_TMC0  0xff90      /* Timer control register 0 */
#define NEC_TMC1  0xff91      /* Timer control register 1 */
#define NEC_TMMS0 0xff94      /* Timer unit macro service control register 0 */
#define NEC_TMMS1 0xff95      /* Timer unit macro service control register 1 */
#define NEC_TMMS2 0xff96      /* Timer unit macro service control register 2 */
#define NEC_TMIC0 0xff9c      /* Timer unit interrupt request control register 0 */
#define NEC_TMIC1 0xff9d      /* Timer unit interrupt request control register 1 */
#define NEC_TMIC2 0xff9e      /* Timer unit interrupt request control register 2 */

// Parallel Port 1
#define NEC_P1    0xff08      /* P2   Data */
#define NEC_PM1   0xff09      /* P2M2 Direction */
#define NEC_PMC1  0xff0A      /* PMC2 Control */

// Parallel Port 2
#define NEC_P2    0xff10      /* P2   Data */
#define NEC_PM2   0xff11      /* P2M2 Direction */
#define NEC_PMC2  0xff12      /* PMC2 Control */

/************************************************************
 * Configuration of port have to be remembert because
 * PM and PMC registers can only be written to and
 * only written as a whole byte (no bit set/clear).
 *
 * So this is the config for the current NEC V25 ELKS system
 *
 * Port.Pin  Name   Function Direction Used in
 * P1.0             special  in        do not change
 * P1.1      INTP0  special  in        NE200 NIC
 * P1.2             special  in        do not change
 * P1.3             special  in        do not change
 * P1.4             I/O      in
 * P1.5             I/O      in
 * P1.6      /SCK0  special  out       spi-hw-necv25.S
 * P1.7             I/O      in
 *
 * Port.Pin  Name   Function Direction Used in
 * P2.0      SDA    I/O      in/out    i2c-ll.S
 * P2.1      SCL    I/O      out       i2c-ll.S
 * P2.2   ISA Reset I/O      out       startup code
 * P2.3      CS     I/O      out       spi-hw-necv25.S
 * P2.4      CS     I/O      out       spi-necv25.S
 * P2.5      CLK    I/O      out       spi-necv25.S
 * P2.6      MOSI   I/O      out       spi-necv25.S
 * P2.7      MISO   I/O      in        spi-necv25.S
 * *********************************************************/
// PM1 PMC1, PM2 and PMC2 have to get initialized in BIOS
// or startup code with these values before ELKS starts:
#define NEC_PM1_DEF  0xbf
#define NEC_PMC1_DEF 0x48
#define NEC_PM2_DEF  0x80
#define NEC_PMC2_DEF 0x00
 
// Other interrupt control registers
#define NEC_EXIC0 0xff4c      /* External interrupt 0 */
#define NEC_EXIC1 0xff4d      /* External interrupt 1 */
#define NEC_EXIC2 0xff4e      /* External interrupt 2 */

#define NEC_DIC0  0xffac      /* DMA interrupt 0 */
#define NEC_DIC1  0xffad      /* DMA interrupt 1 */
#define NEC_TBIC  0xffec      /* Time base interrupt */

// Interrupt Vector numbers
#define NEC_INTTU0  0x1c      /* Timer 0 Interrupt */
#define NEC_INTTU1  0x1d      /* Timer 1 Interrupt */
#define NEC_INTTU2  0x1e      /* Timer 1 Interrupt */

#define NEC_INTSE1  0x10      /* SIO1 Error Interrupt */
#define NEC_INTSR1  0x11      /* SIO1 RX Interrupt */
#define NEC_INTST1  0x12      /* SIO1 TX Interrupt */

#define NEC_INTP0   0x18      /* External Interrupt 0 */
#define NEC_INTP1   0x19      /* External Interrupt 1 */
#define NEC_INTP2   0x1A      /* External Interrupt 2 */

#define UART1_IRQ_RX  1       /* maps to interrupt NEC_INTSR1 */
#define UART1_IRQ_TX  2       /* maps to interrupt NEC_INTST1 */

// Interrupt register options
#define IRQFLAG      0x80     /* interrupt request flag */
#define IRQMSK       0x40     /* mask interrupt, vectored int, no macro service, no register bank switching */
#define IRQUMSK      0x00     /* unmask interrupt, vectored int, no macro service, no register bank switching */
#define IRQPRI0      0x00     /* interrupt priority 0 */
#define IRQPRI1      0x01     /* interrupt priority 1 */
#define IRQPRI2      0x02     /* interrupt priority 2 */
#define IRQPRI3      0x03     /* interrupt priority 3 */
#define IRQPRI4      0x04     /* interrupt priority 4 */
#define IRQPRI5      0x05     /* interrupt priority 5 */
#define IRQPRI6      0x06     /* interrupt priority 6 */
#define IRQPRI7      0x07     /* interrupt priority 7 */
#define IRQPRID      0x07     /* interrupt priority not selectable (set in base register!) */

// Baud rate prescaler
#define BR_DIV2      0x00     /* CPU clock / 2 */
#define BR_DIV4      0x01     /* CPU clock / 4 */
#define BR_DIV8      0x02     /* CPU clock / 8 */
#define BR_DIV16     0x03     /* CPU clock / 16 */
#define BR_DIV32     0x04     /* CPU clock / 32 */
#define BR_DIV64     0x05     /* CPU clock / 64 */
#define BR_DIV128    0x06     /* CPU clock / 128 */
#define BR_DIV256    0x07     /* CPU clock / 256 */
#define BR_DIV512    0x08     /* CPU clock / 512 */
