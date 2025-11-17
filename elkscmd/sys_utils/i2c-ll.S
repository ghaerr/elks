/**
 * Low level I2C driver using bit-banging for DS3231 RTC
 * Target is the NEC V25 CPU
 *
 * Uses special V25 opcodes set1 and clr1
 *
 * Note: SDA is open drain, so high output is archieved through pull up resistor
 *       while the port pin is switched to input!
 *       Low output is archieved through switching the port to output while data
 *       register remains set to low
 *
 * 14. Nov. 2025 swausd
 *
 * Limitations: SDA and CLK have to be on the same port.
 **************************************************************************
 * Attention: Direction and functions of port pins have to be set in a 
 * central position (BIOS or startup code). See definitions in necv25.h
 **************************************************************************
 */

   .code16

#include <arch/necv25.h>

// pin definitions                           /* V25 Port pin: */
#define PIN_SDA   0                          /* P2.7 */
#define PIN_SCL   1                          /* P2.6 */

#define PIN_SDA_BIT (1 << PIN_SDA)

   .text

// NEC V25 "set1" set bit instruction
.macro set1 bit addr
   .word    0x1c0f                           // set1 $bit,(addr)
   .byte    0x06
   .word    \addr
   .byte    \bit
.endm


// NEC V25 "clr1" clear bit instruction
.macro clr1 bit addr
   .word    0x1a0f                           // clr1 $bit,(addr)
   .byte    0x06
   .word    \addr
   .byte    \bit
.endm


.global i2c_init
// Initialise I2C interface
// void i2c_init(void)
i2c_init:
   push     %ds
   movw     $NEC_HW_SEGMENT, %bx             // load DS to access memmory mapped CPU registers
   movw     %bx, %ds

   movb     $NEC_PM2_DEF, (NEC_PM2)          // set SDA to input - see note in header! -> set SDA line to high by external pullup
   set1     PIN_SCL, NEC_P2                  // set SCL to high

   pop      %ds
   ret


.global i2c_start
// I2C Start condition
// void i2c_start(void)
i2c_start:
   push     %ds
   movw     $NEC_HW_SEGMENT, %bx             // load DS to access memmory mapped CPU registers
   movw     %bx, %ds

   clr1     PIN_SCL, NEC_P2                  // SCL to low
   movb     $(NEC_PM2_DEF | PIN_SDA_BIT), (NEC_PM2) // release SDA, pulled up external to high
   set1     PIN_SCL, NEC_P2                  // SCL to high
   clr1     PIN_SDA, NEC_P2
   movb     $NEC_PM2_DEF, (NEC_PM2)          // SDA to low

   pop      %ds
   ret


.global i2c_stop
// I2C Stop condition
// void i2c_stop(void)
i2c_stop:
   push     %ds
   movw     $NEC_HW_SEGMENT, %bx             // load DS to access memmory mapped CPU registers
   movw     %bx, %ds

   clr1     PIN_SCL, NEC_P2                  // SCL to low
   clr1     PIN_SDA, NEC_P2
   movb     $NEC_PM2_DEF, (NEC_PM2)          // SDA to low
   set1     PIN_SCL, NEC_P2                  // SCL to high
   movb     $(NEC_PM2_DEF | PIN_SDA_BIT), (NEC_PM2) // release SDA, pulled up external to high

   pop      %ds
   ret


.global i2c_nak
// Send I2C not acknowledgement
// void i2c_nak(void)
i2c_nak:
   push     %ds
   movw     $NEC_HW_SEGMENT, %bx             // load DS to access memmory mapped CPU registers
   movw     %bx, %ds

   clr1     PIN_SCL, NEC_P2                  // SCL to low
   movb     $(NEC_PM2_DEF | PIN_SDA_BIT), (NEC_PM2) // release SDA, pulled up external to high
   set1     PIN_SCL, NEC_P2                  // SCL to high

   pop      %ds
   ret


.global i2c_ack
// read I2C acknowledgement
// unsigned char i2c_ack(void)
// returns true if ACK
i2c_ack:
   push     %ds
   movw     $NEC_HW_SEGMENT, %bx             // load DS to access memmory mapped CPU registers
   movw     %bx, %ds

   clr1     PIN_SCL, NEC_P2                  // SCL to low
   movb     $(NEC_PM2_DEF | PIN_SDA_BIT), (NEC_PM2) // release SDA, pulled up external to high
   set1     PIN_SCL, NEC_P2                  // SCL to high

   xorw     %ax, %ax
   testb    $PIN_SDA_BIT, (NEC_P2)           // SDA read
   jnz      1f

   decw     %ax

1:
   pop      %ds
   ret


.global i2c_read
// Read a byte from the I2C bus
// unsigned char i2c_read(void)
i2c_read:
   push     %ds
   movw     $NEC_HW_SEGMENT, %bx             // load DS to access memmory mapped CPU registers
   movw     %bx, %ds

   xorw     %ax, %ax
   movw     $8, %cx                          // set loop/bit counter

1:
   clr1     PIN_SCL, NEC_P2                  // SCL to low
   set1     PIN_SCL, NEC_P2                  // SCL to high

   sal      %al
   testb    $PIN_SDA_BIT, (NEC_P2)           // SDA read
   jz       2f

   orb      $0x01, %al

2:
   loop     1b

   pop      %ds
   ret


.global i2c_write
// Write a byte to the I2C bus
// void i2c_write(unsigned char data)
i2c_write:
   push     %bp
   movw     %sp, %bp
   push     %ds

   movw     $NEC_HW_SEGMENT, %bx             // load DS to access memmory mapped CPU registers
   movw     %bx, %ds

   movw     4(%bp), %ax                      // get byte to send
   movw     $8, %cx                          // set loop/bit counter

1:
   clr1     PIN_SCL, NEC_P2                  // SCL to low
   testb    $0x80, %al                       // bit for bit...
   jz       2f

   movb     $(NEC_PM2_DEF | PIN_SDA_BIT), (NEC_PM2) // release SDA, pulled up external to high
   jmp      3f

2:
   clr1     PIN_SDA, NEC_P2
   movb     $NEC_PM2_DEF, (NEC_PM2)          // SDA to low

3:
   set1     PIN_SCL, NEC_P2                  // SCL to high
   sal      %al
   loop     1b

   pop      %ds
   pop      %bp
   ret
