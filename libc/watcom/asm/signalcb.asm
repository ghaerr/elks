;
; signalcb.asm - signal callback from ELKS kernel
;

_TEXT   segment use16 word public 'CODE'
      ;assume  cs:_TEXT

    extrn _syscall_cb_ :far
    public __syscall_signal
__syscall_signal proc far
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

        mov ax,6[bp]    ; get signal #
        callf _syscall_cb_
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
        retf 2          ; get rid of the signum

__syscall_signal endp

_TEXT   ends
        end
