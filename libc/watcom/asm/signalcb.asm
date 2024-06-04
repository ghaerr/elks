;
; signalcb.asm - signal callback from ELKS kernel
;
; 3 Jun 2024 Greg Haerr

_TEXT   segment use16 word public 'CODE'

        extrn   _signal_wchandler_ :far
        public  __signal_cbhandler
__signal_cbhandler proc far
        push bp
        mov bp,sp

        pushf
        push ax
        push bx
        push cx
        push dx
        push si
        push di
        push ds
        push es

        mov ax,6[bp]                ; get signal #
        callf _signal_wchandler_    ; call user function from C

        pop es
        pop ds
        pop di
        pop si
        pop dx
        pop cx
        pop bx
        pop ax
        popf

        pop bp
        retf 2                      ; get rid of the signum
__signal_cbhandler endp

_TEXT   ends
        end
