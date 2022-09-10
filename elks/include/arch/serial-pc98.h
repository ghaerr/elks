/*
 * Serial Port assignments for PC-98
 * 8251
 *
 */

#ifndef __LINUXMT_SERIAL_REG_H
#define __LINUXMT_SERIAL_REG_H

/* 8251
 * Offset from the base 0x30
 */
#define UART_RX		0	/* In:  Receive buffer */
#define UART_TX		0	/* Out: Transmit buffer */
#define UART_LSR	2	/* In:  Status Register */
#define UART_LCR	2	/* Out: Mode Register */
#define UART_MSR	3	/* In:  Read Signal Register */
#define UART_IER	5	/* Out: Interrupt Mask Set Register */

/* The definitions for the Mode Register */
#define UART_LCR_WLEN5  0x00	/* Wordlength: 5 bits */
#define UART_LCR_WLEN6  0x01	/* Wordlength: 6 bits */
#define UART_LCR_WLEN7  0x02	/* Wordlength: 7 bits */
#define UART_LCR_WLEN8  0x03	/* Wordlength: 8 bits */

/* The definitions for the Status Register */
#define UART_LSR_TEMT	0x04	/* Transmitter empty */
#define UART_LSR_THRE	0x01	/* 2nd Transmitter buffer empty */
#define UART_LSR_BI	0x40	/* Break interrupt indicator */
#define UART_LSR_FE	0x20	/* Frame error indicator */
#define UART_LSR_PE	0x08	/* Parity error indicator */
#define UART_LSR_OE	0x10	/* Overrun error indicator */
#define UART_LSR_DR	0x02	/* Receiver data ready */

/* The definitions for the Interrupt Mask Set Register */
#define UART_IER_THRI	0x04	/* Enable 2nd Transmitter buffer empty int. */
#define UART_IER_RDI	0x01	/* Enable receiver data interrupt */

#endif
