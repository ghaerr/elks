; int fmemcmp(void __far *s1, void __far *s2, size_t n)
;   returns 0 on match, otherwise nonzero
;
; 2 Jan 2025 Greg Haerr

include mdef.inc
include struct.inc

        modstart    fmemcmp

        xdefp   "C",fmemcmp
        defpe    fmemcmp

; s1 in DX:AX, s2 in CX:BX, n on stack
        push    bp
        mov     bp,sp
        push    si
        push    di

        mov     ds,dx
        mov     si,ax
        mov     es,cx
        mov     di,bx
if _MODEL and _BIG_CODE
        mov     cx,6[bp]    ; n
else
        mov     cx,4[bp]    ; n
endif
        cld
        repz cmpsb
        jz     L1           ; equal

        mov     ax,1        ; not equal, returns nonzero
        pop     di
        pop     si
        pop     bp
        ret     2
L1:     xor     ax,ax
        pop     di
        pop     si
        pop     bp
        ret     2

fmemcmp endp
        endmod
        end
