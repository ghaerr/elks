/*
 *	Assembly user access routines for the kernel.
 */
 
#include <arch/asm-offsets.h>

#ifndef S_SPLINT_S
#asm

    .text

/* void memcpy_fromfs(void *daddr, void *saddr, size_t len);*/
    .globl  _memcpy_fromfs

_memcpy_fromfs:

    push    bp
    mov	    bp,sp
    mov	    bx,_current
    mov	    dx,es
    mov	    ax,ds
    mov	    es,ax
    mov	    ds,TASK_USER_DS[bx]
    mov	    ax,di
    mov	    di,4[bp]
    mov	    bx,si
    mov	    si,6[bp]
    mov	    cx,8[bp]
    cld
    rep
    movsb
    mov	    si,bx
    mov	    di,ax
    mov	    ax,es
    mov	    ds,ax
    mov	    es,dx
    pop	    bp
    ret

/* void memcpy_tofs(void *daddr, void *saddr, size_t len);*/
    .globl  _memcpy_tofs

_memcpy_tofs:

    push    bp
    mov	    bp,sp
    mov	    bx,_current
    mov	    dx,es
    mov	    es,TASK_USER_DS[bx]
    mov	    ax,di
    mov	    di,4[bp]
    mov	    bx,si
    mov	    si,6[bp]
    mov	    cx,8[bp]
    cld
    rep
    movsb
    mov	    si,bx
    mov	    di,ax
    mov	    es,dx
    pop	    bp
    ret

/* void fmemcpy(dseg, dest, sseg, src, size); */
    .globl  _fmemcpy

_fmemcpy:

    push    bp
    mov	    bp,sp
    push    ds
    mov	    dx,es
    mov	    es,4[bp]
    mov	    ax,di
    lds	    di,6[bp]
    mov	    bx,si
    mov	    si,10[bp]
    mov	    cx,12[bp]
    cld
    rep
    movsb
    mov	    si,bx
    mov	    di,ax
    mov	    es,dx
    pop	    ds
    pop	    bp
    ret

/* int strlen_fromfs(void *saddr); */

    /*  scasb uses es:di, not ds:si, so it is not necessary
     *  to save and restore ds
     */
    .globl  _strlen_fromfs

_strlen_fromfs:

    push    bp
    mov	    bp,sp
    mov	    bx,_current
    mov	    dx,es
    mov	    es,TASK_USER_DS[bx]	! source segment
    mov	    bx,di
    mov	    di,4[bp]		! source address
    xor	    al,al		! search for NULL byte
    cld
    mov	    cx,#-1
    repne
    scasb
    sub	    di,4[bp]		! calc len +1
    dec	    di
    mov	    ax,di		! save in local var ds
    mov	    di,bx
    mov	    es,dx
    pop	    bp
    ret

    .data
    .extern _current

#endasm
#endif

