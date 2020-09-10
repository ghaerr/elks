/* IA-16 far -> near call thunks without argument popping

   Copyright (C) 2019 Free Software Foundation, Inc.
   Written By TK Chia

This file is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

This file is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */

//#include "ia16-preamble.h"

	/* These routines must go into the near text segment, if at all.  */
	.code16
	.text

	.global	__ia16_far_call_thunk.bx.si
	.type	__ia16_far_call_thunk.bx.si, @function
	.global	__ia16_far_call_thunk.bx.di
	.type	__ia16_far_call_thunk.bx.di, @function
__ia16_far_call_thunk.bx.si:
	// fast version when si is zero
	and	%si,%si
	jnz	00f
	call	%bx
	retf
00:
	pushw	%bp			/* set up stack frame */
	movw	%sp,	%bp
	pushw	%bx			/* preserve %bx and %cx */
	pushw	%cx
	movw	%si,	%cx		/* move stack args. size to %cx */
	jmp	0f
__ia16_far_call_thunk.bx.di:
	// fast version when di is zero
	and	%di,%di
	jnz	00f
	call	%bx
	retf
00:
	pushw	%bp
	movw	%sp,	%bp
	pushw	%bx
	pushw	%cx
	movw	%di,	%cx
0:					/* re-push stack arguments */
	jcxz	2f
	leaw	4(%bp),	%bx
	addw	%cx,	%bx
	shrw	%cx
1:
	pushw	(%bx)			// SS: prefix removed
	decw	%bx
	decw	%bx
	loop	1b
2:
	movw	-4(%bp), %cx		/* reload %cx for callee */
	callw	*-2(%bp)		/* call function */
	movw	%bp,	%sp		/* tear down stack frame and return */
	popw	%bp
	lretw
