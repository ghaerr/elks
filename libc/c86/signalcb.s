;
; signalcb.s - signal callback from ELKS kernel for C86
;
; 15 Feb 2025 Greg Haerr
;
        use16   86

        .data
        .extern __sigtable
        .text
        .align  2

        .global __signal_cbhandler
__signal_cbhandler:
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
        mov ax,ss                   ; ensure valid DS (=SS)
        mov ds,ax

        mov ax,6[bp]                ; get signal #
        push ax
        dec ax
        shl ax,#1
        mov si,ax
        add si,#__sigtable
        mov bx,[si]
        call bx                     ; call _sigtable[sig-1]
        add sp,#2

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
