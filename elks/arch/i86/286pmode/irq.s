!
! irq.s
!
! low-level entry points for interrupts and exceptions
!

PMODE_DATASEL equ 0x10

    .text

    .extern _do_exc
    .extern _do_irq

    .globl _exc_00
_exc_00:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    xor ax, ax
    push ax
    call _do_exc
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _exc_01
_exc_01:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x01
    push ax
    call _do_exc
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _exc_02
_exc_02:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x02
    push ax
    call _do_exc
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _exc_03
_exc_03:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x03
    push ax
    call _do_exc
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _exc_04
_exc_04:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x04
    push ax
    call _do_exc
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _exc_05
_exc_05:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x05
    push ax
    call _do_exc
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _exc_06
_exc_06:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x06
    push ax
    call _do_exc
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _exc_07
_exc_07:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x07
    push ax
    call _do_exc
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _exc_08
_exc_08:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x08
    push ax
    call _do_exc
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _exc_09
_exc_09:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x09
    push ax
    call _do_exc
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _exc_0a
_exc_0a:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x0a
    push ax
    call _do_exc
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _exc_0b
_exc_0b:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x0b
    push ax
    call _do_exc
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _exc_0c
_exc_0c:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x0c
    push ax
    call _do_exc
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _exc_0d
_exc_0d:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x0d
    push ax
    call _do_exc
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _exc_0e
_exc_0e:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x0e
    push ax
    call _do_exc
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _exc_10
_exc_10:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x10
    push ax
    call _do_exc
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _exc_11
_exc_11:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x11
    push ax
    call _do_exc
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _exc_xx
_exc_xx:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0xffff
    push ax
    call _do_exc
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _irq_0
_irq_0:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    xor ax, ax
    push ax
    call _do_irq
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _irq_1
_irq_1:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x01
    push ax
    call _do_irq
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _irq_2
_irq_2:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x02
    push ax
    call _do_irq
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _irq_3
_irq_3:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x03
    push ax
    call _do_irq
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _irq_4
_irq_4:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x04
    push ax
    call _do_irq
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _irq_5
_irq_5:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x05
    push ax
    call _do_irq
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _irq_6
_irq_6:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x06
    push ax
    call _do_irq
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _irq_7
_irq_7:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x07
    push ax
    call _do_irq
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _irq_8
_irq_8:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x08
    push ax
    call _do_irq
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _irq_9
_irq_9:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x09
    push ax
    call _do_irq
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _irq_a
_irq_a:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x0a
    push ax
    call _do_irq
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _irq_b
_irq_b:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x0b
    push ax
    call _do_irq
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _irq_c
_irq_c:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x0c
    push ax
    call _do_irq
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _irq_d
_irq_d:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x0d
    push ax
    call _do_irq
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _irq_e
_irq_e:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x0e
    push ax
    call _do_irq
    pop ax
    pop es
    pop ds
    popa
    iret

    .globl _irq_f
_irq_f:
    pusha
    push ds
    push es
    mov ax, #PMODE_DATASEL
    mov ds, ax
    mov es, ax
    mov ax, #0x0f
    push ax
    call _do_irq
    pop ax
    pop es
    pop ds
    popa
    iret
