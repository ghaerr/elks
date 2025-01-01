; ELKS system call library and startup code for C86 - AS86 version
; Must be kept synchronized with elks/arch/i86/kernel/syscall.dat
; PART I
;
; 23 Nov 24 Greg Haerr
; 26 Dec 24 Added C startup code for argc/argv and exit return
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        use16   86

        .sect   1               ; first data seg
        dw      0,0             ; prevent data having address 0
        .data                   ; default data seg 3
        .align  2
        .comm   _errno,2
        .comm   _environ,2
        .comm   ___stacklow,2
        ;.comm   ___argc,2
        ;.comm   ___argv,2
        ;.comm   ___program_filename,2

        .text
        .align  2
        entry   start
        .extern _main
start:                          ; C program entry point
        mov     ax,sp
        sub     ax,dx           ; DX is stack size
        mov     [___stacklow],ax
        pop     ax              ; get argc
        ;mov     [___argc],ax
        mov     bx, sp
        ;mov     [___argv],bx
        mov     dx,bx           ; save argc in DX
.1:     cmp     [bx],#1
        inc     bx
        inc     bx
        jnc     .1
        mov     [_environ],bx
        ;mov     bx,sp
        ;mov     bx,[bx]
        ;mov     [___program_filename], bx
        ;push    [___argv]
        push    dx              ; push argv
        push    ax              ; restore argc
        ;call   initrtns
        call    _main
        push    ax              ; pass return value to exit
        call    _exit           ; no return

        .align  2
        .global callsys
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
        cmp     ax,#0
        jae     .L1             ; success
        neg     ax
        mov     [_errno], ax
        mov     ax,#-1
.L1:
        pop     di
        pop     si
        pop     bp
        ret

        .global _exit
        .global __exit
_exit:                          ; C exit temp comes here
__exit:                         ; _exit
        mov     ax,#1
        jmp     callsys

        .global _read
_read:
        mov     ax,#3
        jmp     callsys

        .global _write
_write:
        mov     ax,#4
        jmp     callsys

        .global _open
_open:
        mov     ax,#5
        jmp     callsys

        .global _close
_close:
        mov     ax,#6
        jmp     callsys

        .global __lseek
__lseek:
        mov     ax,#19
        jmp     near callsys
        .global _ioctl

_ioctl:
        mov     ax,#54
        jmp     near callsys

