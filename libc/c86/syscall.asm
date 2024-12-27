; ELKS system call library for C86/NASM
; Must be kept synchronized with elks/arch/i86/kernel/syscall.dat
; PART I
;
; 23 Nov 24 Greg Haerr
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        cpu     8086

        section .data
        align   2
        global  _errno
_errno: dw      0               ; global C errno

        section .text
        align   2
        global  callsys
callsys:                        ; common routine for ELKS system call
        push    bp
        mov     bp, sp
        push    si
        push    di
        mov     bx, [bp+4]
        mov     cx, [bp+6]
        mov     dx, [bp+8]
        mov     di, [bp+10]
        mov     si, [bp+12]
        int     0x80
        cmp     ax, 0
        jae     L_1             ; success
        neg     ax
        mov     [_errno], ax
        mov     ax, -1
L_1:
        pop     di
        pop     si
        pop     bp
        ret

        global _exit
        global __exit
_exit:                          ; C exit temp comes here
__exit:                         ; _exit
        mov     ax, 1
        jmp     callsys

        global _read
_read:
        mov     ax, 3
        jmp     callsys

        global _write
_write:
        mov     ax, 4
        jmp     callsys

        global _open
_open:
        mov     ax, 5
        jmp     callsys

        global _close
_close:
        mov     ax, 6
        jmp     callsys
