;  void fmemcpy(void __far *s1, void __far *s2, size_t n)
;
; 7 Apr 2025 Greg Haerr

include mdef.inc
include struct.inc

        modstart    fmemcpy

        xdefp   "C",fmemcpy
        defpe    fmemcpy

; s1 in DX:AX, s2 in CX:BX, n on stack
        push    bp
        mov     bp,sp
        push    si
        push    di

        mov     ds,cx
        mov     si,bx
        mov     es,dx
        mov     di,ax
if _MODEL and _BIG_CODE
        mov     cx,6[bp]    ; n
else
        mov     cx,4[bp]    ; n
endif
        cld
        shr     cx,1        ; copy words
        rep movsw
        rcl     cx,1        ; then possibly final byte
        rep movsb
        pop     di
        pop     si
        pop     bp
        ret     2

fmemcpy endp
        endmod
        end
