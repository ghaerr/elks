;  int fmemcmp(void __far *s1, void __far *s2, size_t n)
;   returns 0 on match, otherwise -1/1 on the difference
;   of the first differing bytes as unsigned char values
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
        xor     ax,ax       ; assume match
        cld
        repz cmpsb
        jz      L1          ; equal
        sbb     ax,ax       ; not equal, AX now 0 or -1
        or      ax,1        ; AX now 1 or -1
L1:     pop     di
        pop     si
        pop     bp
        ret     2

fmemcmp endp
        endmod
        end
