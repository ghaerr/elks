// include file for NEC V25 inline assembly
//
// 1. Nov. 2025 swausd

.set NEC_HW_SEGMENT,  0xf000  // Segment register of NEC V25 internal periphery

// Index of registers based on RXB for Serial Interface
.set RXB,      0x00     // Receive buffer register
.set TXB,      0x02     // Transmit buffer register
.set SRMS,     0x05     // Serial reception macro service control register
.set STMS,     0x06     // Serial transmission macro service control register
.set SCM,      0x08     // Serial mode register
.set SCC,      0x09     // Serial control register
.set BRG,      0x0a     // Baud rate generator register
.set SCE,      0x0b     // Serial error register
.set SEIC,     0x0c     // Serial error interrupt request control register
.set SRIC,     0x0d     // Serial reception interrupt request control register
.set STIC,     0x0e     // Serial transmission interrupt request control register

// Serial interface Rx/Tx registers
.set RXB0,     0xff60   // Receive buffer register 1
.set TXB0,     0xff62   // Transmit buffer register 0
.set RXB1,     0xff70   // Receive buffer register 1
.set TXB1,     0xff72   // Transmit buffer register 1

// Serial mode register options
.set TXE,      0x80     // enable Tx
.set RXE,      0x40     // enable Rx
.set NOPRTY,   0x00     // no parity
.set ZPRTY,    0x10     // 0 parity
.set ODPRTY,   0x20     // odd parity
.set EVPRTY,   0x30     // even parity
.set CL7,      0x00     // 7 data bits
.set CL8,      0x08     // 8 data bits
.set SL1,      0x01     // 1 stop bit
.set SL2,      0x05     // 2 stop bit


// Index of registers based on TM0 for Timer Unit
.set TM0,      0x00     // Timer register 0
.set MD0,      0x02     // Modulo/timer register 0
.set TM1,      0x08     // Timer register 1
.set MD1,      0x0a     // Modulo/timer register 1
.set TMC0,     0x10     // Timer control register 0
.set TMC1,     0x11     // Timer control register 1
.set TMMS0,    0x14     // Timer unit macro service control register 0
.set TMMS1,    0x15     // Timer unit macro service control register 1
.set TMMS2,    0x16     // Timer unit macro service control register 2
.set TMIC0,    0x1c     // Timer unit interrupt request control register 0
.set TMIC1,    0x1d     // Timer unit interrupt request control register 1
.set TMIC2,    0x1e     // Timer unit interrupt request control register 2

// Timer mode register options
.set TSTOP,      0x00     // TM Stop countdown
.set TLOAD,      0x80     // TM Load countdown
.set TCLK6,      0x00     // Fclkl/6 prescaler
.set TCLK128,    0x40     // Fclkl/128 prescaler
.set MSTOP,      0x00     // MD Stop countdown
.set TOUTF,      0x00     // TOUT fixed
.set TOUTI,      0x08     // TOUT inverted
.set TOUT0,      0x00     // TOUT low
.set TOUT1,      0x04     // TOUT high
.set TMODEI,     0x00     // Interval mode
.set TMODES,     0x00     // One shot mode


// Interrupt register options
.set IRQFLAG,  0x80     // interrupt request flag
.set IRQMSK,   0x40     // mask interrupt, vectored int, no macro service, no register bank switching
.set IRQUMSK,  0x00     // unmask interrupt, vectored int, no macro service, no register bank switching
.set IRQPRI0,  0x00     // interrupt priority 0
.set IRQPRI1,  0x01     // interrupt priority 1
.set IRQPRI2,  0x02     // interrupt priority 2
.set IRQPRI3,  0x03     // interrupt priority 3
.set IRQPRI4,  0x04     // interrupt priority 4
.set IRQPRI5,  0x05     // interrupt priority 5
.set IRQPRI6,  0x06     // interrupt priority 6
.set IRQPRI7,  0x07     // interrupt priority 7
.set IRQPRID,  0x07     // interrupt priority not selectable (set in base register!)


// Interrupt control registers
.set SEIC0,    0xff6c   // Serial error interrupt request control register 0
.set SRIC0,    0xff6d   // Serial reception interrupt request control register 0
.set STIC0,    0xff6e   // Serial transmission interrupt request control register 0

.set SEIC1,    0xff7c   // Serial error interrupt request control register 1
.set SRIC1,    0xff7d   // Serial reception interrupt request control register 1
.set STIC1,    0xff7e   // Serial transmission interrupt request control register 1

.set TMIC0,    0xff9c   // Timer unit interrupt request control register 0
.set TMIC1,    0xff9d   // Timer unit interrupt request control register 1
.set TMIC2,    0xff9e   // Timer unit interrupt request control register 2

.set EXIC0,    0xff4c   // External interrupt 0
.set EXIC1,    0xff4d   // External interrupt 1
.set EXIC2,    0xff4e   // External interrupt 2

.set DIC0,     0xffac   // DMA interrupt 0
.set DIC1,     0xffad   // DMA interrupt 1
.set TBIC,     0xffec   // Time base interrupt
