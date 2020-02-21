/* Copyright (C) 1996 Robert de Bath <rdebath@cix.compulink.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 */

/* This is an implementation of the C standard IO package.
 */

/* Call the stdio initiliser; it's main job it to call atexit */

#ifdef __AS386_16__
#asm
  loc	1		! Make sure the pointer is in the correct segment
auto_func:		! Label for bcc -M to work.
  .word	___io_init_vars	! Pointer to the autorun function
  .text			! So the function after is also in the correct seg.
#endasm
#endif

#ifdef __AS386_32__
#asm
  loc	1		! Make sure the pointer is in the correct segment
auto_func:		! Label for bcc -M to work.
  .long	___io_init_vars	! Pointer to the autorun function
  .text			! So the function after is also in the correct seg.
#endasm
#endif
