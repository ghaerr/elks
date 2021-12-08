    .code16

/**
 * Low Level SD card SPI driver
 * for any 80c18x CPU using bit-bang over the GPIO pins.
 *
 * October 2021 Santiago Hormazabal.
 *
 * Limitations: CLK, MOSI and CS have to be on the same port.
 */

#define PORT_LATCH_CLK_MOSI_CS 0xff56   /* P1LTCH */
#define PORT_CONTROL_CLK_MOSI_CS 0xff54 /* P1CON */
#define PORT_DIR_CLK_MOSI_CS 0xff50     /* P1DIR */
#define PIN_CS 5                        /* P1.5 */
#define PIN_CLK 1                       /* P1.1 */
#define PIN_MOSI 0                      /* P1.0 */

#define PORT_PIN_MISO 0xff5a            /* P2PIN */
#define PORT_CONTROL_MISO 0xff5c        /* P2CON */
#define PORT_DIR_MISO 0xff58            /* P2DIR */
#define PIN_MISO 6                      /* P2.6 */


#define PIN_CS_BIT (1 << PIN_CS)
#define PIN_CS_MASK (~(PIN_CS_BIT))

#define PIN_CLK_BIT (1 << PIN_CLK)
#define PIN_CLK_MASK (~(PIN_CLK_BIT))

#define PIN_MOSI_BIT (1 << PIN_MOSI)
#define PIN_MOSI_MASK (~(PIN_MOSI_BIT))

#define PIN_MISO_BIT (1 << PIN_MISO)
#define PIN_MISO_MASK (~(PIN_MISO_BIT))

    .text

// Toggles CLK line as fast as possible.
// Assumes %dx is $PORT_LATCH_CLK_MOSI_CS and
// %ax has the cached contents of $PORT_LATCH_CLK_MOSI_CS
.macro toggle_clk
    or      $PIN_CLK_BIT, %al
    out     %ax, %dx    // write clk high
    and     $PIN_CLK_MASK, %al
    out     %ax, %dx    // write clk low
.endm

// Tests if %cl's bit "bit" is set, and if so,
// set MOSI high. If not, set MOSI low. After that,
// toggle the clk effectively sending a bit via SPI.
// Assumes %dx is $PORT_LATCH_CLK_MOSI_CS
// Destroys %ax
.macro test_bit_and_transmit bit
    test    $\bit, %cl          // is bit "bit" set?
    jz      3f                  // nope? jump to 3:

    or      $PIN_MOSI_BIT, %al  // bit set, so turn on MOSI
    jmp     2f                  // jump to toggle clk

3:
    and     $PIN_MOSI_MASK, %al // bit not set, so turn off MOSI
2:
    out     %ax, %dx            // write the MOSI value to $PORT_LATCH_CLK_MOSI_CS
    toggle_clk                  // THEN toggle the clk
.endm

// Shifts the data on %bl, then samples the MISO pin.
// If it's set, then it will "or $1" into %bl.
// It also sets MOSI high when toggling the clock.
.macro sample_input_bit_and_shift_data
    sal     $1, %bl                         // %bl << 1

    mov     $PORT_PIN_MISO, %dx             // fetch current MISO from $PORT_PIN_MISO
    in      %dx, %ax
    test    $PIN_MISO_BIT, %al              // test if PIN_MISO is on
    jz      2f                              // nope? jump to 2:
    or      $1, %bl                         // PIN_MISO is on, so or $1 into %bl
2:
    mov     $PORT_LATCH_CLK_MOSI_CS, %dx    // fetch current $PORT_LATCH_CLK_MOSI_CS
    in      %dx, %ax                        // because toggle_clk requires %dx and %ax
    toggle_clk                              // set with $PORT_LATCH_CLK_MOSI_CS and its contents
.endm

.global spi_init_ll
//void spi_init_ll(void)
spi_init_ll:
    cli
    mov     $PORT_CONTROL_CLK_MOSI_CS, %dx
    in      %dx, %ax
    and     $(PIN_CS_MASK & PIN_MOSI_MASK & PIN_CLK_MASK), %al
    out     %ax, %dx

    mov     $PORT_CONTROL_MISO, %dx
    in      %dx, %ax
    and     $PIN_MISO_MASK, %al
    out     %ax, %dx


    mov     $PORT_DIR_CLK_MOSI_CS, %dx
    in      %dx, %ax
    and     $(PIN_CS_MASK & PIN_MOSI_MASK & PIN_CLK_MASK), %al
    out     %ax, %dx

    mov     $PORT_DIR_MISO, %dx
    in      %dx, %ax
    or      $PIN_MISO_BIT, %al
    out     %ax, %dx

    sti

    call    spi_cs_1
    call    spi_sck_0

    ret


.global spi_cs_0
//void spi_cs_0(void)
spi_cs_0:
    cli
    mov     $PORT_LATCH_CLK_MOSI_CS, %dx
    in      %dx, %ax
    and     $PIN_CS_MASK, %al
    out     %ax, %dx
    sti
    ret

.global spi_cs_1
//void spi_cs_1(void)
spi_cs_1:
    cli
    mov     $PORT_LATCH_CLK_MOSI_CS, %dx
    in      %dx, %ax
    or      $PIN_CS_BIT, %al
    out     %ax, %dx
    sti
    ret

.global spi_sck_0
//void spi_sck_0(void)
spi_sck_0:
    cli
    mov     $PORT_LATCH_CLK_MOSI_CS, %dx
    in      %dx, %ax
    and     $PIN_CLK_MASK, %al
    out     %ax, %dx
    sti
    ret

.global spi_transmit
//void spi_transmit(uint8_t data)
spi_transmit:
    cli
    mov     %sp, %bx
    mov     2(%bx), %cx

    mov     $PORT_LATCH_CLK_MOSI_CS, %dx

    in      %dx, %ax     // fetch current P1LTCH value

    test_bit_and_transmit 0x80
    test_bit_and_transmit 0x40
    test_bit_and_transmit 0x20
    test_bit_and_transmit 0x10
    test_bit_and_transmit 0x08
    test_bit_and_transmit 0x04
    test_bit_and_transmit 0x02
    test_bit_and_transmit 0x01

    sti
    ret

.global spi_receive
//uint8_t spi_receive(void)
spi_receive:
    cli
    xor     %bx, %bx

    mov     $PORT_LATCH_CLK_MOSI_CS, %dx    // fetch current $PORT_LATCH_CLK_MOSI_CS
    in      %dx, %ax
    or      $PIN_MOSI_BIT, %al              // and set MOSI high
    out     %ax, %dx                        // write clk high

    sample_input_bit_and_shift_data
    sample_input_bit_and_shift_data
    sample_input_bit_and_shift_data
    sample_input_bit_and_shift_data
    sample_input_bit_and_shift_data
    sample_input_bit_and_shift_data
    sample_input_bit_and_shift_data
    sample_input_bit_and_shift_data

    mov     %bx, %ax
    sti
    ret

.global spi_send_ffs
//void spi_send_ffs(uint16_t bytes)
spi_send_ffs:
    cli
    mov     %sp, %bx
    mov     2(%bx), %cx

    mov     $PORT_LATCH_CLK_MOSI_CS, %dx    // fetch current $PORT_LATCH_CLK_MOSI_CS
    in      %dx, %ax
    or      $PIN_MOSI_BIT, %al              // set MOSI high
    out     %ax, %dx                        // write $PORT_LATCH_CLK_MOSI_CS back

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
    sti
    ret
