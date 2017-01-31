;-------------------------------------------------------------------------------
; NE2K driver - low part - PHY routines
;-------------------------------------------------------------------------------


; TODO: move definitions to ne2k.h

; I/O base @ 300h

io_eth_mdio     EQU $314
io_eth_gpio     EQU $317

; MDIO register

mdio_out    EQU $08
mdio_in     EQU $04
mdio_dir    EQU $02  ; 0 = output / 1 = input
mdio_clock  EQU $01

; GPIO register

phy_stat_link    EQU $01
phy_stat_duplex  EQU $02
phy_stat_speed   EQU $04

; PHY registers

phy_mode_speed   EQU $2000
phy_mode_duplex  EQU $0100


	.TEXT

;-------------------------------------------------------------------------------
; Delay routines
;-------------------------------------------------------------------------------

; This is calibrated for CPU @ 20 MHz

delay_2:

	push    cx
	mov     cx, #2

d2_loop:

	loop    d2_loop
	pop     cx
	ret


;-------------------------------------------------------------------------------
; MDIO - Send word (up to 16 bits)
;-------------------------------------------------------------------------------

; AX : word to send (MSB first)
; CX : size of word
; DX : MDIO register

mdio_tx_word:

	push    ax
	push    bx
	push    cx
	push    dx

	; TODO: optimize with BX as word to send
	; TODO: to free AX for I/O data

	mov     bx, ax

mtb_loop:

	; align MSB to data out bit

	mov     ax, bx
	shr     ax, 12
	and     al, #mdio_out

	; output the data bit
	; with clock low

	out     dx, al
	call    delay_2

	; now rise the clock

	or	al, #mdio_clock
	out     dx, al
	call    delay_2

	; next data bit

	shl     bx, 1
	loop    mtb_loop

	pop     dx
	pop     cx
	pop     bx
	pop     ax
	ret


;-------------------------------------------------------------------------------
; MDIO - Send frame (up to 32 bits)
;-------------------------------------------------------------------------------

; AX : data word (second)
; CX : size of frame
; DX : control word (first)

mdio_tx_frame:

	push    ax
	push    bx
	push    cx
	push    dx
	push    si

	; could be optimized there...

	mov     bx, dx
	mov     dx, #io_eth_mdio
	push    ax
	mov     ax, bx

	; send begin of frame

	mov     si, #0
	cmp     cx, #16
	jbe     mtf_next

	sub     cx, #16
	mov     si, cx
	mov     cx, #16

mtf_next:

	call    mdio_tx_word

	; send remaining frame

	pop     ax
	mov     cx, si
	or      cx, cx
	jz      mtf_exit
	call    mdio_tx_word

mtf_exit:

	pop     si
	pop     dx
	pop     cx
	pop     bx
	pop     ax
	ret


;-------------------------------------------------------------------------------
; MDIO - Turnaround from TX to RX
;-------------------------------------------------------------------------------

mdio_tx_rx:

	push    ax
	push    dx

	mov     dx, #io_eth_mdio
	mov     al, #mdio_dir
	out     dx, al                   ; clock low
	call    delay_2

	mov     al, #(mdio_dir | mdio_clock)
	out     dx, al
	call    delay_2

	pop     dx
	pop     ax
	ret


;-------------------------------------------------------------------------------
; MDIO - Write frame (32 bits)
;-------------------------------------------------------------------------------

; AX : register value
; CX : PHY identifier (10h for internal)
; DX : register address

mdio_write:

	push    ax
	push    bx
	push    cx
	push    dx

	; align PHY address
	; align register address
	; 5002h for head (start + write) & tail

	shl     cx, 7
	shl     dx, 2
	or      dx, cx
	or      dx, #$5002

	; send preamble
	; first 16 low bits are really needed ?

	push    ax
	push    dx

	xor     dx, dx
	xor     ax, ax
	mov     cx, #16                   ; 16 bits low
	call    mdio_tx_frame

	; second 32 high bits are standard

	mov     dx, #$FFFF
	mov     ax, #$FFFF
	mov     cx, #32
	call    mdio_tx_frame

	; send frame

	pop     dx
	pop     ax

	; 32 bits - control & data words

	mov     cx, #32
	call    mdio_tx_frame

	; is this really needed ?

	call    mdio_tx_rx

	pop     dx
	pop     cx
	pop     bx
	pop     ax
	ret


;-------------------------------------------------------------------------------
; MDIO - Read frame (32 bits)
;-------------------------------------------------------------------------------

; CX : PHY identifier (10h for internal)
; DX : register address

; returns

; CF : 0=success 1=no response
; AX : register value (returned)

mdio_read:

	push    bx
	push    cx
	push    dx

	; align PHY address
	; align register address
	; 6000h for head (start + read)

	shl     cx, #7
	shl     dx, #2
	or      dx, cx
	or      dx, #$6000

	; send preamble
	; first 16 low bits are really needed ?

	push    ax
	push    dx

	xor     dx, dx
	xor     ax, ax
	mov     cx, #16
	call    mdio_tx_frame

	; second 32 low bits are standard

	mov     dx, #$FFFF
	mov     ax, #$FFFF
	mov     cx, #32
	call    mdio_tx_frame

	; send MAC turn frame

	pop     dx
	pop     ax

	mov     cx, #14
	call    mdio_tx_frame

	; turn around
	; and wait for PHY to drive data down

	mov     cx, #2

mr_wait:

	call    mdio_tx_rx
	mov     dx, #io_eth_mdio
	in      al, dx
	test    al, #mdio_in
	jz      mr_next
	loop    mr_wait
	stc
	jmp     mr_exit

mr_next:

	mov     cx, #16
	xor     bx, bx

mr_loop:

	mov     al, #mdio_dir
	out     dx, al
	call    delay_2
	mov     al, #(mdio_dir | mdio_clock)
	out     dx, al
	call    delay_2
	in      al, dx
	call    delay_2
	shr     al, #2
	and     al, #1
	shl     bx, #1
	or      bl, al
	loop    mr_loop

	; is this really needed ?

	call    mdio_tx_rx

	clc
	jmp     mr_exit

mr_exit:

	mov     ax, bx
	pop     dx
	pop     cx
	pop     bx
	ret


;-------------------------------------------------------------------------------
; Get PHY register
;-------------------------------------------------------------------------------

; arg1 : register number (0...31)
; arg2 : pointer to register value

; returns:

; AX : error code (0 = success)


_ne2k_phy_get:

	push    bp
	mov     bp, sp

	push    bx
	push    cx
	push    dx

	; internal PHY (address = 10h)

	mov     cx, #$10
	mov     dx, [bp + $4]
	call    mdio_read
	jc      pg_err

	; save value in *arg2

	mov     bx, [bp + $6]
	mov     [bx], ax

	; success

	xor     ax,ax
	jmp     pg_exit

pg_err:

	mov     ax,1

pg_exit:

	pop     dx
	pop     cx
	pop     bx

	pop     bp
	ret

;-------------------------------------------------------------------------------
; PHY - Get speed and duplex mode
;-------------------------------------------------------------------------------

	; get internal PHY status
	; from linked GPIO pins

	;mov     dx, #io_eth_gpio
	;in      al, dx
	;and     al, #7


;-------------------------------------------------------------------------------
; PHY - Set speed and duplex mode
;-------------------------------------------------------------------------------

; AX : speed (0 = low,  1 = high)
; DX : duplex (0 = half,  1 = full)

phy_mode:

	push    ax
	push    cx
	push    dx

	; align bits to register

	and     ax, #1
	shl     ax, 13
	and     dx, #1
	shl     dx, 8
	or      ax, dx

	; set control register
	; of internal PHY (address = 10h)

	mov     cx, #$10
	mov     dx, #0
	call    mdio_write

	pop     dx
	pop     cx
	pop     ax
	ret


;-------------------------------------------------------------------------------

	EXPORT  _ne2k_phy_get

	END

;-------------------------------------------------------------------------------
