; Fast 32-bit combined divide and modulo routine
;
; unsigned long __divmod(unsigned long val, unsigned int *baserem)
;  Unsigned divide 32-bits by 16-bits
;      Store denominator in *baserem before calling
;      Returns 32-bit quotient in DX:AX and remainder in *baserem
;
;  Designed for a fast replacement of the following code
;      unsigned int rem, base;
;      rem = val % base;
;      val = val / base;
;  New code:
;      rem = base;
;      val = __divmod(val, &rem);
;
;  from by OpenWatcom ltoa.c __uldiv routine
;  14 Sep 2024 Greg Haerr
;  26 Dec 2024 ported to C86

        use16   86
        .text

;  divides DX:AX / [BX]
;  returns DX:AX with remainder in [BX]

        .global ___divmod
___divmod:
        mov    bx,sp
        mov    ax,[bx+2]            ; low val
        mov    dx,[bx+4]            ; high val
        mov    bx,[bx+6]            ; &rem

        xor    cx,cx                ; temp CX = 0
        cmp    dx,[bx]              ; is upper 16 bits numerator less than denominator
        jb     .fast                ; yes - only one DIV needed
        xchg   ax,dx                ; AX = upper numerator, DX = lower numerator
        xchg   cx,dx                ; DX = 0, CX = lower numerator
        div    word ptr [bx]        ; AX = upper numerator / base, DX = remainder
        xchg   ax,cx                ; AX = lower numerator, CX = high quotient
.fast:
        div    word ptr [bx]        ; AX = lower numerator / base, DX = remainder
        mov    [bx],dx              ; store remainder
        mov    dx,cx                ; DX = high quotient, AX = low quotient
        ret
