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

// Parallel Port 2
#define NEC_P2    0xff10      /* P2   Data */
#define NEC_PM2   0xff11      /* P2M2 Direction */
#define NEC_PMC2  0xff12      /* PMC2 Control */

// Other interrupt control registers
#define NEC_EXIC0 0xff4c      /* External interrupt 0 */
#define NEC_EXIC1 0xff4d      /* External interrupt 1 */
#define NEC_EXIC2 0xff4e      /* External interrupt 2 */

#define NEC_DIC0  0xffac      /* DMA interrupt 0 */
#define NEC_DIC1  0xffad      /* DMA interrupt 1 */
#define NEC_TBIC  0xffec      /* Time base interrupt */

// Interrupt Vector numbers
#define NEC_INTTU0  0x1e      /* Timer 0 Interrupt */
#define NEC_INTTU1  0x1e      /* Timer 1 Interrupt */
#define NEC_INTTU2  0x1e      /* Timer 1 Interrupt */

#define NEC_INTSE1  0x10      /* SIO1 Error Interrupt */
#define NEC_INTSR1  0x11      /* SIO1 RX Interrupt */
#define NEC_INTST1  0x12      /* SIO1 TX Interrupt */

#define UART1_IRQ_RX  1       /* maps to interrupt NEC_INTSR1 */
#define UART1_IRQ_TX  2       /* maps to interrupt NEC_INTST1 */
