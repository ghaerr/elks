// peek[bwl], poke[bwl], setup[bw] routines
//
// segment parameter after offset to allow LDS from the stack
// assume DS=SS, save ES, for GCC-IA16

// for SETUP_DATA segment
#include <linuxmt/config.h>

#define ARG0	2
#define ARG1	4
#define ARG2	6

	.code16
	.text

	.global peekb
	.global peekw
	.global peekl
	.global pokeb
	.global pokew
	.global pokel
	.global	setupb
	.global	setupw

// byte_t peekb (word_t off, seg_t seg)
// returns the byte at the far pointer segment:offset

peekb:
	mov    %ds,%dx
	mov    %sp,%bx
	lds    ARG0(%bx),%bx  // far pointer
	mov    (%bx),%al      // DS by default
	xor    %ah,%ah
	mov    %dx,%ds
	ret

// word_t peekw (word_t off, seg_t seg)
// returns the word at the far pointer segment:offset

peekw:
	mov    %ds,%dx
	mov    %sp,%bx
	lds    ARG0(%bx),%bx  // far pointer
	mov    (%bx),%ax      // DS by default
	mov    %dx,%ds
	ret

// long_t peekl (word_t off, seg_t seg)
// returns the word at the far pointer segment:offset

peekl:
	mov    %ds,%cx
	mov    %sp,%bx
	lds    ARG0(%bx),%bx  // far pointer
	mov    (%bx),%ax      // DS by default
	mov    2(%bx),%dx
	mov    %cx,%ds
	ret

// void pokeb (word_t off, seg_t seg, byte__t val)
// compiler pushes byte_t as word_t
// writes the byte at the far pointer segment:offset

pokeb:
	mov    %ds,%dx
	mov    %sp,%bx
	mov    ARG2(%bx),%ax  // value
	lds    ARG0(%bx),%bx  // far pointer
	mov    %al,(%bx)      // DS by default
	mov    %dx,%ds
	ret

// void pokew (word_t off, seg_t seg, word_t val)
// writes the word at the far pointer segment:offset

pokew:
	mov    %ds,%dx
	mov    %sp,%bx
	mov    ARG2(%bx),%ax  // value
	lds    ARG0(%bx),%bx  // far pointer
	mov    %ax,(%bx)      // DS by default
	mov    %dx,%ds
	ret

// void pokel (word_t off, seg_t seg, long_t val)
// writes the long at the far pointer segment:offset

pokel:
	mov    %ds,%cx
	mov    %sp,%bx
	mov    ARG2(%bx),%ax  // low word value
	mov    ARG2+2(%bx),%dx// high word value
	lds    ARG0(%bx),%bx  // far pointer
	mov    %ax,(%bx)      // DS by default
	mov    %dx,2(%bx)
	mov    %cx,%ds
	ret

// int setupb(word_t off)
// returns the unsigned byte at the far pointer SETUP_DATA:offset

setupb:
	mov	%ds,%dx
	mov	%sp,%bx
	mov	ARG0(%bx),%bx   // off
	mov	$SETUP_DATA,%ax
	mov	%ax,%ds
	mov	(%bx),%al
	xor	%ah,%ah
	mov	%dx,%ds
	ret

// int setupw(word_t off)`
// returns the word at the far pointer SETUP_DATA:offset

setupw:
	mov	%ds,%dx
	mov	%sp,%bx
	mov	ARG0(%bx),%bx   // off
	mov	$SETUP_DATA,%ax
	mov	%ax,%ds
	mov	(%bx),%ax
	mov	%dx,%ds
	ret
