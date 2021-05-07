This directory contains two exomizer decrunchers in assembly for 32bit
ARM with thumb2 instructions, contributed by ALeX Kazik (alex@kazik.de). 
They are useful for embedded systems.

The difference between the two files, universal.S and speed.S, is universal 
supports all combinations of exomizer protocol flags as compile options, while
speed only supports a subset but is faster.

They use only the stack and no global variables.

Optionally two security checks can be enabled at compile flags:
- CHECK_BUFFER_SIZE - for input and output a buffer size has to be specified
- CHECK_OVERRUN - the input and output MUST be in the same buffer - check if the output overruns the input
(In case of a failure the output pointer is NULL.)

To compile using gcc you can use the following command line:
> arm-none-eabi-gcc -c speed.S -mcpu=cortex-m4

(a ARM cpu with thumb2 instructions is required)

The parameters are switched in contrast to the C version, this is
because the out pointer is passed on in r0 and never leaves - so the
result (r0) is the new out pointer.

Here follows some benchmark numbers from an NUCLEO-64 board with a
STM32F411 on it.  These numbers where measured decrunching test3 and was done
using 0 waitstates and without caching.
The speed and universal tests are, unless noted, without security checks, 
identical to the C version.

exodecr.c, -Os (P39):
 464 bytes, 8233408 cycles

exodecr.c, -O3 (P39):
1716 bytes, 3800500 cycles

exodecr.c, -Os (exomizer 3.0 = P7):
 468 bytes, 7993103 cycles

exodecr.c, -O3 (exomizer 3.0 = P7):
1700 bytes, 3675802 cycles

universal.S, P39:
 288 bytes, 2707475 cycles

universal.S, P7:
 248 bytes, 2634823 cycles

universal.S, P13:
 216 bytes, 2976219 cycles

speed.S, P13:
 244 bytes, 1818600 cycles

speed.S, P13, with both security checks enabled:
 284 bytes, 1941419 cycles
