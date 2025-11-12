    .code16

/**
 * Low Level SD card SPI driver
 * for NEC V25 CPU using serial port 0 for hardware SPI in poll mode
 * driver only works with SPI Moe 3 (can't be changed)
 * SD cards require SPI Mode 3 but seem to work alos with Mode 3
 * see https://elm-chan.org/docs/mmc/mmc_e.html on SPI Mode
 *
 * Uses special V25 opcodes set1 and clr1
 * Adapted from the work of Santiago Hormazabal for 8018x CPUs
 *
 * 10. Nov. 2025 swausd
 *
 * Setup:
 * CS on Port 2.3
 * serial has to be serial 0
 * /SCK0 connected with /CTS0, Tx clock (out) to Rx clock (in)
 * configure baud rate by setting BR_PRSCLR to desired value
 */

#include <arch/necv25.h>

// pin definitions                           /* V25 Port pin: */
#define PIN_CS    3                          /* P2.3 */
#define PIN_SCK0  6                          /* P1.6 do not change! */
#define BR_PRSCLR BR_DIV2                    /* baud rate prescaler = SPI clock */

    .text

// NEC V25 "set1" set bit instruction
.macro set1 bit addr
      .word   0x1c0f                           // set1 $bit,(addr)
      .byte   0x06
      .word   \addr
      .byte   \bit
.endm


// NEC V25 "clr1" clear bit instruction
.macro clr1 bit addr
      .word   0x1a0f                           // clr1 $bit,(addr)
      .byte   0x06
      .word   \addr
      .byte   \bit
.endm


.global spi_init_ll
//void spi_init_ll(void)
spi_init_ll:
    push    %ds
    movw    $NEC_HW_SEGMENT, %bx             // load DS to access memmory mapped CPU registers
    movw    %bx, %ds

    // prepare serial port 0 for synchronous mode with max baud rate
    movb    $0x00, (NEC_SCM0)                // SCM0:  synchronous mode, disable Rx/Tx, external Rx clock
    movb    $BR_PRSCLR, (NEC_SCC0)           // SCM0:  baud rat preselector fclk/2 = max clock
    movb    $0x01, (NEC_BRG0)                // BRG0:  max Baud rate
    movb    $0x47, (NEC_SEIC0)               // SEIC0: clear Error flag
    movb    $0x47, (NEC_SRIC0)               // SRIC0: clear Rx ready flag
    movb    $0x47, (NEC_STIC0)               // STIC0: clear Tx ready flag
    movb    $0xc0, (NEC_SCM0)                // SCM0:  synchronous mode, enable Rx/Tx, external Rx clock

    clr1    PIN_CS, NEC_PMC2                 // set to IO, not special function
    clr1    PIN_CS, NEC_PM2                  // set to output
    set1    PIN_CS, NEC_P2                   // de-select SPI
    set1    PIN_SCK0, NEC_PMC1               // set to special function
    clr1    PIN_SCK0, NEC_PM1                // set to output
    set1    PIN_CS, NEC_P2                   // de-select SPI

    pop     %ds
    ret


.global spi_cs_0
//void spi_cs_0(void)
spi_cs_0:
    push    %ds
    movw    $NEC_HW_SEGMENT, %bx             // load DS to access memmory mapped CPU registers
    movw    %bx, %ds
    clr1    PIN_CS, NEC_P2
    pop     %ds
    ret


.global spi_cs_1
//void spi_cs_1(void)
spi_cs_1:
    push    %ds
    movw    $NEC_HW_SEGMENT, %bx             // load DS to access memmory mapped CPU registers
    movw    %bx, %ds
    set1    PIN_CS, NEC_P2
    pop     %ds
    ret


.global spi_transmit
//void spi_transmit(uint8_t data)
spi_transmit:
    mov     %sp, %bx
    mov     2(%bx), %ax

    push    %ds
    movw    $NEC_HW_SEGMENT, %bx             // load DS to access memmory mapped CPU registers
    movw    %bx, %ds

    movb    $BR_PRSCLR, (NEC_SCC0)           // reset RX/TX buffers

1:
    testb   $IRQFLAG, (NEC_STIC0)            // wait for Tx ready
    jz      1b
                                             // clearing the Tx ready flag can be ignored
    movb    %al, (NEC_TXB0)                  // send byte

2:
    testb   $IRQFLAG, (NEC_SRIC0)            // wait for byte ready
    jz      2b

    movb    $(IRQMSK+IRQPRID), (NEC_SRIC0)   // ignore received byte and clear Rx byte ready flag

    pop     %ds
    ret


.global spi_receive
//uint8_t spi_receive(void)
spi_receive:
    push    %ds
    movw    $NEC_HW_SEGMENT, %bx             // load DS to access memmory mapped CPU registers
    movw    %bx, %ds

    movb    $BR_PRSCLR, (NEC_SCC0)           // reset RX/TX buffers

1:  // request byte from client by sending 0xff
    testb   $IRQFLAG, (NEC_STIC0)            // wait for Tx ready
    jz   1b
                                             // clearing the Tx ready flag can be ignored
    movb    $0xff, (NEC_TXB0)                // send 0xff to request answer

2:  // receive byte
    testb   $IRQFLAG, (NEC_SRIC0)            // wait for byte ready
    jz      2b

    xorb    %ah, %ah
    movb    (NEC_RXB0), %al                  // get byte
    movb    $(IRQMSK+IRQPRID), (NEC_SRIC0)   // clear Rx byte ready flag

    pop     %ds
    ret


.global spi_send_ffs
//void spi_send_ffs(uint16_t bytes)
spi_send_ffs:
    mov     %sp, %bx
    mov     2(%bx), %cx

    push    %ds
    movw    $NEC_HW_SEGMENT, %bx             // load DS to access memmory mapped CPU registers
    movw    %bx, %ds

    movb    $BR_PRSCLR, (NEC_SCC0)           // reset RX/TX buffers

    movb     $0xff, %al

1:
    testb   $IRQFLAG, (NEC_STIC0)            // wait for Tx ready
    jz      1b
                                             // clearing the Tx ready flag can be ignored
    movb    %al, (NEC_TXB0)                  // send 0xff byte

2:
    testb   $IRQFLAG, (NEC_SRIC0)            // wait for byte ready
    jz      2b

    movb    $(IRQMSK+IRQPRID), (NEC_SRIC0)   // ignore received byte and clear Rx byte ready flag

    loop    1b

    pop     %ds
    ret


.global spi_read_block
//uint8_t spi_read_block(char *buf, ramdesc_t seg, uint16_t count)
spi_read_block:
    mov     %sp, %bx
    push    %es
    push    %di
    push    %ds

    movw    6(%bx), %cx
    movw    4(%bx), %es
    movw    2(%bx), %di

    movw    $NEC_HW_SEGMENT, %bx             // load DS to access memmory mapped CPU registers
    movw    %bx, %ds

    movb    $BR_PRSCLR, (NEC_SCC0)           // reset RX/TX buffers

    movb    $IRQFLAG, %dl                    // setup some constant values
    movb    $(IRQMSK+IRQPRID), %bh
    movb    $0xff, %bl

1:                                           // request byte from client by sending 0xff
    testb   %dl, (NEC_STIC0)                 // wait for Tx ready, only for first byte in block
    jz      1b

2:                                           // clearing the Tx ready flag can be ignored
    movb    %bl, (NEC_TXB0)                  // send byte

3:  // receive byte
    testb   %dl, (NEC_SRIC0)                 // wait for byte ready
    jz      3b

    movb    (NEC_RXB0), %al                  // get byte
    movb    %al, %es:(%di)                   // store to buffer
    inc     %di
    movb    %bh, (NEC_SRIC0)                 // clear Rx byte ready flag

    loop    2b

    pop     %ds
    pop     %di
    pop     %es
    ret


.global spi_write_block
//uint8_t spi_write_block(char *buf, ramdesc_t seg, uint16_t count)
spi_write_block:
    mov     %sp, %bx
    push    %es
    push    %di
    push    %ds

    movw    6(%bx), %cx
    movw    4(%bx), %es
    movw    2(%bx), %di

    movw    $NEC_HW_SEGMENT, %bx             // load DS to access memmory mapped CPU registers
    movw    %bx, %ds

    movb    $BR_PRSCLR, (NEC_SCC0)           // reset RX/TX buffers

    movb    $IRQFLAG, %dl                    // setup some constant values
    movb    $(IRQMSK+IRQPRID), %bh

1:
    testb   %dl, (NEC_STIC0)                 // wait for Tx ready, only for first byte in block
    jz      1b

2:                                           // clearing the Tx ready flag can be ignored
    movb    %es:(%di), %al                   // get byte from buffer
    movb    %al, (NEC_TXB0)                  // send byte
    inc     %di

3:
    testb   %dl, (NEC_SRIC0)                 // wait for byte ready
    jz      3b

    movb    %bh, (NEC_SRIC0)                 // ignore received byte and clear Rx byte ready flag

    loop    2b

    pop     %ds
    pop     %di
    pop     %es
    ret

