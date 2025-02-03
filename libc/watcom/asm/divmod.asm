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

include mdef.inc
include struct.inc

        modstart    divmod

        xdefp   "C",__divmod
        defpe    __divmod

;  divides DX:AX / [BX]
;  returns DX:AX with remainder in [BX]

        push   cx
        xor    cx,cx                ; temp CX = 0
        cmp    dx,[bx]              ; is upper 16 bits numerator less than denominator
        jb     short fast           ; yes - only one DIV needed
        xchg   ax,dx                ; AX = upper numerator, DX = lower numerator
        xchg   cx,dx                ; DX = 0, CX = lower numerator
        div    word ptr [bx]        ; AX = upper numerator / base, DX = remainder
        xchg   ax,cx                ; AX = lower numerator, CX = high quotient
    fast:
        div    word ptr [bx]        ; AX = lower numerator / base, DX = remainder
        mov    [bx],dx              ; store remainder
        mov    dx,cx                ; DX = high quotient, AX = low quotient
        pop    cx
        ret

__divmod endp

        endmod
        end
