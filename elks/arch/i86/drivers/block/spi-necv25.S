    .code16

/**
 * Low Level SD card SPI driver
 * for NEC V25 CPU using bit-bang over the GPIO pins.
 * Uses special V25 opcodes set1 and clr1
 * Adapted from the work of Santiago Hormazabal for 8018x CPUs
 *
 * 04. Nov. 2025 swausd
 *
 * Limitations: CLK, MOSI, MISP and CS have to be on the same port.
 **************************************************************************
 * Attention: Direction and functions of port pins have to be set in a 
 * central position (BIOS or startup code). See definitions in necv25.h
 **************************************************************************
 */

#include <arch/necv25.h>

// port definitions
#define PORT_LATCH      NEC_P2               /* read port input or write port output      */

// pin definitions                           /* V25 Port pin: */
#define PIN_CS   4                           /* P2.4 */
#define PIN_CLK  5                           /* P2.5 */
#define PIN_MOSI 6                           /* P2.6 */
#define PIN_MISO 7                           /* P2.7 */

#define PIN_MISO_BIT (1 << PIN_MISO)
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


// Toggles CLK line as fast as possible.
// Assumes %ds is $NEC_HW_SEGMENT
.macro toggle_clk
      set1 PIN_CLK, PORT_LATCH
      clr1 PIN_CLK, PORT_LATCH
.endm


// Tests if %cl's bit "bit" is set, and if so,
// set MOSI high. If not, set MOSI low. After that,
// toggle the clk effectively sending a bit via SPI.
// Assumes %ds is $NEC_HW_SEGMENT
// Destroys %al
.macro test_bit_and_transmit bit
    test    $\bit, %cl                       // is bit "bit" set?
    jz      3f                               // nope? jump to 3:

    set1    PIN_MOSI, PORT_LATCH             // bit set, so turn on MOSI
    jmp     2f                               // jump to toggle clk

3:
    clr1    PIN_MOSI, PORT_LATCH             // bit not set, so turn off MOSI
2:
    toggle_clk                               // THEN toggle the clk
.endm


// Shifts the data on %bl, then samples the MISO pin.
// If it's set, then it will "or $1" into %bl.
.macro sample_input_bit_and_shift_data
    sal     $1, %bl                          // %bl << 1
    testb   $PIN_MISO_BIT, (PORT_LATCH)      // test if PIN_MISO is on
    jz      2f                               // nope? jump to 2:

    or      $1, %bl                          // PIN_MISO is on, so or $1 into %bl
2:
    toggle_clk
.endm


.global spi_init_ll
//void spi_init_ll(void)
spi_init_ll:
    push    %ds
    movw    $NEC_HW_SEGMENT, %bx             // load DS to access memmory mapped CPU registers
    movw    %bx, %ds

    set1    PIN_CS, PORT_LATCH
    clr1    PIN_CLK, PORT_LATCH
    pop     %ds
    ret


.global spi_cs_0
//void spi_cs_0(void)
spi_cs_0:
    push    %ds
    movw    $NEC_HW_SEGMENT, %bx             // load DS to access memmory mapped CPU registers
    movw    %bx, %ds
    clr1    PIN_CS, PORT_LATCH
    pop     %ds
    ret


.global spi_cs_1
//void spi_cs_1(void)
spi_cs_1:
    push    %ds
    movw    $NEC_HW_SEGMENT, %bx             // load DS to access memmory mapped CPU registers
    movw    %bx, %ds
    set1    PIN_CS, PORT_LATCH
    pop     %ds
    ret


.global spi_transmit
//void spi_transmit(uint8_t data)
spi_transmit:
    mov     %sp, %bx
    mov     2(%bx), %cx

    push    %ds
    movw    $NEC_HW_SEGMENT, %bx             // load DS to access memmory mapped CPU registers
    movw    %bx, %ds

    test_bit_and_transmit 0x80
    test_bit_and_transmit 0x40
    test_bit_and_transmit 0x20
    test_bit_and_transmit 0x10
    test_bit_and_transmit 0x08
    test_bit_and_transmit 0x04
    test_bit_and_transmit 0x02
    test_bit_and_transmit 0x01

    pop     %ds
    ret


.global spi_receive
//uint8_t spi_receive(void)
spi_receive:
    push    %ds
    movw    $NEC_HW_SEGMENT, %bx             // load DS to access memmory mapped CPU registers
    movw    %bx, %ds

    xor     %bx, %bx
    set1    PIN_MOSI, PORT_LATCH

    sample_input_bit_and_shift_data
    sample_input_bit_and_shift_data
    sample_input_bit_and_shift_data
    sample_input_bit_and_shift_data
    sample_input_bit_and_shift_data
    sample_input_bit_and_shift_data
    sample_input_bit_and_shift_data
    sample_input_bit_and_shift_data

    mov     %bx, %ax
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

    set1    PIN_MOSI, PORT_LATCH

loop:
    toggle_clk
    toggle_clk
    toggle_clk
    toggle_clk
    toggle_clk
    toggle_clk
    toggle_clk
    toggle_clk

    dec     %cx
    jnz     loop

    pop     %ds
    ret
