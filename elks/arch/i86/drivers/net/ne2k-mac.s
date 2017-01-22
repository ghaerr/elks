;-------------------------------------------------------------------------------
; NE2K driver - low part
;-------------------------------------------------------------------------------

; I/O base @ 300h

io_eth_command  equ $300
io_eth_unicast  equ $301  ; page 1
io_eth_mdio     equ $314


; MDIO register

mdio_out    equ $08
mdio_in     equ $04
mdio_dir    equ $02  ; 0 = output / 1 = input
mdio_clock  equ $01


;-------------------------------------------------------------------------------
; Delay routines
;-------------------------------------------------------------------------------

delay_2:

	push    cx
	mov     cx,#2

d2_loop:

	loop    d2_loop
	pop     cx
	ret


;-------------------------------------------------------------------------------
; MDIO - Send word
;-------------------------------------------------------------------------------

; AX = word to send (big endian)
; CX = size of word
; DX = MDIO register

mdio_tx_byte:

	push    ax
	push    bx
	push    cx
	push    dx

	mov     bx,ax

mtb_loop:

	mov     ax,bx
	shr     ax,12                   ; align to data out bit
	and     al,#8
	out     dx,al                   ; clock low
	call    delay_2
	or	al,#mdio_clock
	out     dx,al                   ; clock high
	call    delay_2
	shl     bx,1
	loop    mtb_loop

	pop     dx
	pop     cx
	pop     bx
	pop     ax
	ret


;-------------------------------------------------------------------------------
; MDIO - Send word
;-------------------------------------------------------------------------------

mdio_tx_frame:
    proc near		      ;	CODE XREF: mdio_write+28p
seg_bios:0D38					      ;	mdio_write+34p	...
seg_bios:0D38		      push    bx
seg_bios:0D39		      push    dx
seg_bios:0D3A		      push    ax
seg_bios:0D3B		      push    cx
seg_bios:0D3C		      push    si
seg_bios:0D3D		      mov     bx, dx
seg_bios:0D3F		      mov     dx, io_eth_mdio
seg_bios:0D42		      push    ax
seg_bios:0D43		      mov     ax, bx
seg_bios:0D45		      mov     si, 0
seg_bios:0D48		      cmp     cx, 10h
seg_bios:0D4B		      jbe     short loc_F0D55
seg_bios:0D4D		      sub     cx, 10h
seg_bios:0D50		      mov     si, cx
seg_bios:0D52		      mov     cx, 10h
seg_bios:0D55
seg_bios:0D55 loc_F0D55:			      ;	CODE XREF: mdio_tx_frame+13j
seg_bios:0D55		      call    mdio_tx_byte
seg_bios:0D58		      pop     ax
seg_bios:0D59		      mov     cx, si
seg_bios:0D5B		      or      cx, cx
seg_bios:0D5D		      jz      short loc_F0D62
seg_bios:0D5F		      call    mdio_tx_byte
seg_bios:0D62
seg_bios:0D62 loc_F0D62:			      ;	CODE XREF: mdio_tx_frame+25j
seg_bios:0D62		      pop     si
seg_bios:0D63		      pop     cx
seg_bios:0D64		      pop     ax
seg_bios:0D65		      pop     dx
seg_bios:0D66		      pop     bx
seg_bios:0D67		      retn
seg_bios:0D67 mdio_tx_frame    endp
seg_bios:0D67
seg_bios:0D68

;-------------------------------------------------------------------------------
; MDIO - Turnaround from TX to RX
;-------------------------------------------------------------------------------

mdio_tx_rx:

	push    ax
	push    dx

	mov     dx,#io_eth_mdio
	mov     al,#mdio_dir
	out     dx,al                   ; clock low
	call    delay_2

	mov     al,#(mdio_dir | mdio_clock)
	out     dx,al
	call    delay_2

	pop     dx
	pop     ax
	ret


;-------------------------------------------------------------------------------
; MDIO - Write frame (32 bits)
;-------------------------------------------------------------------------------

; AX = register value
; CX = PHY identifier (10h for internal)
; DX = register address

mdio_write:

	push    ax
	push    bx
	push    cx
	push    dx

	; compute first word

	shl     cx,7                    ; align PHY identifier
	shr     dx,1                    ; align register addresse
	or      dx,#$5002               ; head (start & write) and tail

	; send preamble
	; first 16 low bits are really needed ?

	push    dx
	push    ax

	xor     dx,dx
	xor     ax,ax
	mov     cx,16                   ; 16 bits low
	call    mdio_tx_frame

	mov     dx,#$FFFF
	mov     ax,#$FFFF
	mov     cx,32                   ; 32 bits high
	call    mdio_tx_frame

	; send frame

	pop     ax
	pop     dx
	mov     cx,32                   ; 32 bits - control & data words
	call    mdio_tx_frame

	; is this really needed ?

	call    mdio_tx_rx

	pop     dx
	pop     cx
	pop     bx
	pop     ax
	ret


;-------------------------------------------------------------------------------
; MDIO - Read frame
;-------------------------------------------------------------------------------

seg_bios:0DC6
seg_bios:0DC6 mdio_read	      proc near		      ;	CODE XREF: mdio_reg_get+6p
seg_bios:0DC6		      push    cx
seg_bios:0DC7		      push    bx
seg_bios:0DC8		      push    dx
seg_bios:0DC9		      push    ax
seg_bios:0DCA		      shr     dx, 1
seg_bios:0DCC		      shl     cx, 7
seg_bios:0DCF		      shl     dx, 2
seg_bios:0DD2		      or      dx, cx
seg_bios:0DD4		      or      dx, 6000h
seg_bios:0DD8		      push    dx
seg_bios:0DD9		      push    ax
seg_bios:0DDA		      xor     dx, dx
seg_bios:0DDC		      xor     ax, ax
seg_bios:0DDE		      mov     cx, 10h
seg_bios:0DE1		      call    mdio_tx_frame
seg_bios:0DE4		      mov     dx, 0FFFFh
seg_bios:0DE7		      mov     ax, 0FFFFh
seg_bios:0DEA		      mov     cx, 20h ;	' '
seg_bios:0DED		      call    mdio_tx_frame
seg_bios:0DF0		      pop     ax
seg_bios:0DF1		      pop     dx
seg_bios:0DF2		      mov     cx, 0Eh
seg_bios:0DF5		      call    mdio_tx_frame
seg_bios:0DF8		      mov     cx, 2
seg_bios:0DFB
seg_bios:0DFB loc_F0DFB:			      ;	CODE XREF: mdio_read+40j
seg_bios:0DFB		      call    mdio_tx_rx
seg_bios:0DFE		      mov     dx, io_eth_mdio
seg_bios:0E01		      in      al, dx
seg_bios:0E02		      test    al, 4
seg_bios:0E04		      jz      short loc_F0E0A
seg_bios:0E06		      loop    loc_F0DFB
seg_bios:0E08		      jmp     short loc_F0E35
seg_bios:0E0A ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0E0A
seg_bios:0E0A loc_F0E0A:			      ;	CODE XREF: mdio_read+3Ej
seg_bios:0E0A		      mov     cx, 10h
seg_bios:0E0D		      xor     bx, bx
seg_bios:0E0F
seg_bios:0E0F loc_F0E0F:			      ;	CODE XREF: mdio_read+62j
seg_bios:0E0F		      mov     al, 2
seg_bios:0E11		      out     dx, al
seg_bios:0E12		      call    delay_2
seg_bios:0E15		      mov     al, 3
seg_bios:0E17		      out     dx, al
seg_bios:0E18		      call    delay_2
seg_bios:0E1B		      in      al, dx
seg_bios:0E1C		      call    delay_2
seg_bios:0E1F		      shr     al, 2
seg_bios:0E22		      and     al, 1
seg_bios:0E24		      shl     bx, 1
seg_bios:0E26		      or      bl, al
seg_bios:0E28		      loop    loc_F0E0F
seg_bios:0E2A		      call    mdio_tx_rx
seg_bios:0E2D		      clc
seg_bios:0E2E
seg_bios:0E2E loc_F0E2E:			      ;	CODE XREF: mdio_read+70j
seg_bios:0E2E		      pop     ax
seg_bios:0E2F		      mov     ax, bx
seg_bios:0E31		      pop     dx
seg_bios:0E32		      pop     bx
seg_bios:0E33		      pop     cx
seg_bios:0E34		      retn
seg_bios:0E35 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0E35
seg_bios:0E35 loc_F0E35:			      ;	CODE XREF: mdio_read+42j
seg_bios:0E35		      stc
seg_bios:0E36		      jmp     short loc_F0E2E
seg_bios:0E36 mdio_read	      endp
seg_bios:0E36
seg_bios:0E38
seg_bios:0E38 ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:0E38
seg_bios:0E38
seg_bios:0E38 mdio_reg_get    proc near		      ;	CODE XREF: phy_xxx:loc_F0E4Bp
seg_bios:0E38		      push    dx
seg_bios:0E39		      shl     dx, 1
seg_bios:0E3B		      mov     cx, 10h
seg_bios:0E3E		      call    mdio_read
seg_bios:0E41		      pop     dx
seg_bios:0E42		      retn
seg_bios:0E42 mdio_reg_get    endp
seg_bios:0E42
seg_bios:0E43
seg_bios:0E43 ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:0E43
seg_bios:0E43
seg_bios:0E43 phy_xxx	      proc near
seg_bios:0E43		      pusha
seg_bios:0E44		      mov     cx, 5
seg_bios:0E47
seg_bios:0E47 loc_F0E47:			      ;	CODE XREF: phy_xxx+16j
seg_bios:0E47		      push    cx
seg_bios:0E48		      mov     dx, 0
seg_bios:0E4B
seg_bios:0E4B loc_F0E4B:			      ;	CODE XREF: phy_xxx+13j
seg_bios:0E4B		      call    mdio_reg_get
seg_bios:0E4E		      jb      short loc_F0E5D
seg_bios:0E50		      inc     dx
seg_bios:0E51		      cmp     dx, 20h ;	' '
seg_bios:0E54		      jnb     short loc_F0E58
seg_bios:0E56		      jmp     short loc_F0E4B
seg_bios:0E58 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0E58
seg_bios:0E58 loc_F0E58:			      ;	CODE XREF: phy_xxx+11j
seg_bios:0E58		      pop     cx
seg_bios:0E59		      loop    loc_F0E47
seg_bios:0E5B		      jmp     short loc_F0E5E
seg_bios:0E5D ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0E5D
seg_bios:0E5D loc_F0E5D:			      ;	CODE XREF: phy_xxx+Bj
seg_bios:0E5D		      pop     cx
seg_bios:0E5E
seg_bios:0E5E loc_F0E5E:			      ;	CODE XREF: phy_xxx+18j
seg_bios:0E5E		      popa
seg_bios:0E5F		      retn
seg_bios:0E5F phy_xxx	      endp
seg_bios:0E5F
seg_bios:09DD ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:09DD
seg_bios:09DD
seg_bios:09DD phy_stat_wait   proc near		      ;	CODE XREF: eth_loop_int+17p
seg_bios:09DD					      ;	eth_loop_int+3Ep ...
seg_bios:09DD		      push    dx
seg_bios:09DE		      push    ax
seg_bios:09DF		      push    cx
seg_bios:09E0		      xor     cx, cx
seg_bios:09E5
seg_bios:09E5 loc_F09E5:			      ;	CODE XREF: phy_stat_wait+12j
seg_bios:09E5		      mov     dx, 317h
seg_bios:09E8		      in      al, dx
seg_bios:09E9		      and     al, 7
seg_bios:09EB		      cmp     al, bl
seg_bios:09ED		      jz      short loc_F09F1
seg_bios:09EF		      loop    loc_F09E5
seg_bios:09F1
seg_bios:09F1 loc_F09F1:			      ;	CODE XREF: phy_stat_wait+10j
seg_bios:09F4		      pop     cx
seg_bios:09F5		      pop     ax
seg_bios:09F6		      pop     dx
seg_bios:09F7		      retn
seg_bios:09F7 phy_stat_wait   endp
seg_bios:09F7
seg_bios:09F8

;-------------------------------------------------------------------------------
; PHY - Set speed and duplex mode
;-------------------------------------------------------------------------------

phy_100_fd:

	push    ax
	push    cx
	push    dx

	mov     cx,#$10                 ; internal PHY identifier
	mov     dx,#0                   ; PHY control register
	mov     ax,#$2100               ; high speed - full duplex
	call    mdio_write

	pop     dx
	pop     cx
	pop     ax
	ret


phy_100_hd:

	push    ax
	push    cx
	push    dx

	mov     cx,#$10                 ; internal PHY identifier
	mov     dx,#0                   ; PHY control register
	mov     ax,#$2000               ; high speed - half duplex
	call    mdio_write

	pop     dx
	pop     cx
	pop     ax
	ret


phy_10_fd:

	push    ax
	push    cx
	push    dx

	mov     cx,#$10                 ; internal PHY identifier
	mov     dx,#0                   ; PHY control register
	mov     ax,#$0100               ; low speed - full duplex
	call    mdio_write

	pop     dx
	pop     cx
	pop     ax
	ret


phy_10_hd:

	push    ax
	push    cx
	push    dx

	mov     cx,#$10                 ; internal PHY identifier
	mov     dx,#0                   ; PHY control register
	mov     ax,#0                   ; low speed - half duplex
	call    mdio_write

	pop     dx
	pop     cx
	pop     ax
	ret
	

;-------------------------------------------------------------------------------
; ???
;-------------------------------------------------------------------------------

seg_bios:0693 eth_mode	      db 0		      ;	DATA XREF: eth_tx_put+3Dr
seg_bios:0693					      ;	eth_rx_check+7Ar ...
seg_bios:0694
seg_bios:0694 ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:0694
seg_bios:0694
seg_bios:0694 eth_tx_conf_set proc near		      ;	CODE XREF: eth_loop_int+1Cp
seg_bios:0694					      ;	eth_loop_int+43p ...
seg_bios:0694		      push    dx
seg_bios:0695		      push    ax
seg_bios:0696		      mov     dx, 300h
seg_bios:0699		      mov     al, 21h ;	'!'
seg_bios:069B		      out     dx, al
seg_bios:069C		      mov     dx, 30Dh
seg_bios:069F		      mov     al, bl
seg_bios:06A1		      out     dx, al
seg_bios:06A2		      pop     ax
seg_bios:06A3		      pop     dx
seg_bios:06A4		      retn
seg_bios:06A4 eth_tx_conf_set endp
seg_bios:06A4
seg_bios:06A4 ;	───────────────────────────────────────────────────────────────────────────

;-------------------------------------------------------------------------------
; Select register page
;-------------------------------------------------------------------------------

; AL = page number (0 or 1)

eth_page_sel:

	push    ax
	push    dx

	mov     ah,al
	and     ah,#$FE
	shl     ah,6

	mov     dx,#io_eth_command
	in      al,dx
	and     al,#$3F
	or      al,ah
        out     dx,al

	pop     dx
	pop     ax
	ret

        
;-------------------------------------------------------------------------------
; Set unicast address
;-------------------------------------------------------------------------------

; DS:SI = unicast address (6 bytes)

eth_mac_set:

	push ax
	push cx
	push dx
	push si

; select page 1

	mov     al,1
        call    eth_page_sel

	mov     dx,#io_eth_unicast                ; unicast address register (6 bytes)
	mov     cx,#6
	cld

ems_loop:

	lodsb
	out     dx,al
	inc     dx
	loop    ems_loop

	pop     si
	pop     dx
	pop     cx
	pop     ax
	ret

;-------------------------------------------------------------------------------

seg_bios:06FD
seg_bios:06FD ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:06FD
seg_bios:06FD
seg_bios:06FD eth_init	      proc near		      ;	CODE XREF: eth_loop_int+Fp
seg_bios:06FD					      ;	eth_loop_int+36p ...
seg_bios:06FD		      mov     dx, 300h
seg_bios:0700		      mov     al, 21h ;	'!'
seg_bios:0702		      out     dx, al
seg_bios:0703		      mov     dx, 30Eh
seg_bios:0706		      mov     al, 48h ;	'H'
seg_bios:0708		      out     dx, al
seg_bios:0709		      mov     dx, 30Ch
seg_bios:070C		      mov     al, 5Ch ;	'\'
seg_bios:070E		      out     dx, al
seg_bios:070F		      mov     dx, 30Dh
seg_bios:0712		      mov     al, 2
seg_bios:0714		      out     dx, al
seg_bios:0715		      mov     dx, 301h
seg_bios:0718		      mov     al, 46h ;	'F'
seg_bios:071A		      out     dx, al
seg_bios:071B		      mov     dx, 303h
seg_bios:071E		      mov     al, 46h ;	'F'
seg_bios:0720		      out     dx, al
seg_bios:0721		      mov     dx, 302h
seg_bios:0724		      mov     al, 80h ;	'Ç'
seg_bios:0726		      out     dx, al
seg_bios:0727		      mov     dx, 304h
seg_bios:072A		      mov     al, 40h ;	'@'
seg_bios:072C		      out     dx, al
seg_bios:072D		      mov     dx, 307h
seg_bios:0730		      mov     al, 0FFh
seg_bios:0732		      out     dx, al
seg_bios:0733		      mov     dx, 30Fh
seg_bios:0736		      mov     al, 1Fh
seg_bios:0738		      out     dx, al
seg_bios:0739		      mov     dx, 300h
seg_bios:073C		      mov     al, 61h ;	'a'
seg_bios:073E		      out     dx, al
seg_bios:073F		      mov     cx, 6
seg_bios:0742		      mov     dx, 301h
seg_bios:0745		      xor     al, al
seg_bios:0747
seg_bios:0747 loc_F0747:			      ;	CODE XREF: eth_init+4Cj
seg_bios:0747		      out     dx, al
seg_bios:0748		      inc     dx
seg_bios:0749		      loop    loc_F0747
seg_bios:074B		      mov     cx, 8
seg_bios:074E		      mov     dx, 308h
seg_bios:0751		      xor     al, al
seg_bios:0753
seg_bios:0753 loc_F0753:			      ;	CODE XREF: eth_init+58j
seg_bios:0753		      out     dx, al
seg_bios:0754		      inc     dx
seg_bios:0755		      loop    loc_F0753
seg_bios:0757		      mov     dx, 307h
seg_bios:075A		      mov     al, 47h ;	'G'
seg_bios:075C		      out     dx, al
seg_bios:075D		      mov     cs:rx_page, al
seg_bios:0761		      mov     dx, 300h
seg_bios:0764		      mov     al, 21h ;	'!'
seg_bios:0766		      out     dx, al
seg_bios:0767		      mov     ax, 0Fh
seg_bios:076A		      call    outs_get
seg_bios:076D		      retn
seg_bios:076D eth_init	      endp
seg_bios:076D
seg_bios:076E
seg_bios:076E ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:076E
seg_bios:076E
seg_bios:076E eth_test_reg    proc near		      ;	CODE XREF: eth_test+17p
seg_bios:076E		      pusha
seg_bios:076F		      mov     dx, 300h
seg_bios:0772		      mov     al, 61h ;	'a'
seg_bios:0774		      out     dx, al
seg_bios:0775		      mov     dx, 301h
seg_bios:0778		      mov     al, 0FFh
seg_bios:077A		      out     dx, al
seg_bios:077B		      mov     dx, 302h
seg_bios:077E		      mov     al, 0
seg_bios:0780		      out     dx, al
seg_bios:0781		      mov     dx, 303h
seg_bios:0784		      mov     al, 5Ah ;	'Z'
seg_bios:0786		      out     dx, al
seg_bios:0787		      mov     dx, 304h
seg_bios:078A		      mov     al, 0A5h ; 'Ñ'
seg_bios:078C		      out     dx, al
seg_bios:078D		      mov     dx, 305h
seg_bios:0790		      mov     al, 5Ah ;	'Z'
seg_bios:0792		      out     dx, al
seg_bios:0793		      mov     dx, 306h
seg_bios:0796		      mov     al, 0A5h ; 'Ñ'
seg_bios:0798		      out     dx, al
seg_bios:0799		      mov     cx, 8
seg_bios:079C		      mov     dx, 308h
seg_bios:079F
seg_bios:079F loc_F079F:			      ;	CODE XREF: eth_test_reg+35j
seg_bios:079F		      mov     al, cl
seg_bios:07A1		      out     dx, al
seg_bios:07A2		      inc     dx
seg_bios:07A3		      loop    loc_F079F
seg_bios:07A5		      mov     dx, 301h
seg_bios:07A8		      in      al, dx
seg_bios:07A9		      cmp     al, 0FFh
seg_bios:07AB		      jnz     short loc_F07DC
seg_bios:07AD		      inc     dx
seg_bios:07AE		      in      al, dx
seg_bios:07AF		      cmp     al, 0
seg_bios:07B1		      jnz     short loc_F07DC
seg_bios:07B3		      inc     dx
seg_bios:07B4		      in      al, dx
seg_bios:07B5		      cmp     al, 5Ah ;	'Z'
seg_bios:07B7		      jnz     short loc_F07DC
seg_bios:07B9		      inc     dx
seg_bios:07BA		      in      al, dx
seg_bios:07BB		      cmp     al, 0A5h ; 'Ñ'
seg_bios:07BD		      jnz     short loc_F07DC
seg_bios:07BF		      inc     dx
seg_bios:07C0		      in      al, dx
seg_bios:07C1		      cmp     al, 5Ah ;	'Z'
seg_bios:07C3		      jnz     short loc_F07DC
seg_bios:07C5		      inc     dx
seg_bios:07C6		      in      al, dx
seg_bios:07C7		      cmp     al, 0A5h ; 'Ñ'
seg_bios:07C9		      jnz     short loc_F07DC
seg_bios:07CB		      mov     cx, 8
seg_bios:07CE		      mov     dx, 308h
seg_bios:07D1
seg_bios:07D1 loc_F07D1:			      ;	CODE XREF: eth_test_reg+69j
seg_bios:07D1		      in      al, dx
seg_bios:07D2		      cmp     al, cl
seg_bios:07D4		      jnz     short loc_F07DC
seg_bios:07D6		      inc     dx
seg_bios:07D7		      loop    loc_F07D1
seg_bios:07D9		      clc
seg_bios:07DA		      jmp     short loc_F07DD
seg_bios:07DC ;	───────────────────────────────────────────────────────────────────────────
seg_bios:07DC
seg_bios:07DC loc_F07DC:			      ;	CODE XREF: eth_test_reg+3Dj
seg_bios:07DC					      ;	eth_test_reg+43j ...
seg_bios:07DC		      stc
seg_bios:07DD
seg_bios:07DD loc_F07DD:			      ;	CODE XREF: eth_test_reg+6Cj
seg_bios:07DD		      pushf
seg_bios:07DE		      mov     dx, 300h
seg_bios:07E1		      mov     al, 21h ;	'!'
seg_bios:07E3		      out     dx, al
seg_bios:07E4		      popf
seg_bios:07E5		      popa
seg_bios:07E6		      retn
seg_bios:07E6 eth_test_reg    endp
seg_bios:07E6
seg_bios:07E7
seg_bios:07E7 ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:07E7
seg_bios:07E7
seg_bios:07E7 eth_test_buf    proc near		      ;	CODE XREF: eth_test+28p
seg_bios:07E7		      pusha
seg_bios:07E8		      mov     di, 0
seg_bios:07EB		      mov     dx, 300h
seg_bios:07EE		      mov     ax, 22h ;	'"'
seg_bios:07F1		      out     dx, al
seg_bios:07F2		      mov     dx, 30Eh
seg_bios:07F5		      mov     ax, 48h ;	'H'
seg_bios:07F8		      out     dx, al
seg_bios:07F9		      mov     dx, 308h
seg_bios:07FC		      mov     ax, 4000h
seg_bios:07FF		      out     dx, al
seg_bios:0800		      inc     dx
seg_bios:0801		      mov     al, ah
seg_bios:0803		      out     dx, al
seg_bios:0804		      mov     dx, 30Ah
seg_bios:0807		      mov     ax, 4000h
seg_bios:080A		      mov     cx, ax
seg_bios:080C		      push    ax
seg_bios:080D		      out     dx, al
seg_bios:080E		      inc     dx
seg_bios:080F		      mov     al, ah
seg_bios:0811		      out     dx, al
seg_bios:0812		      mov     dx, 300h
seg_bios:0815		      mov     al, 12h
seg_bios:0817		      out     dx, al
seg_bios:0818		      mov     dx, 310h
seg_bios:081B		      mov     ax, di
seg_bios:081D
seg_bios:081D loc_F081D:			      ;	CODE XREF: eth_test_buf+38j
seg_bios:081D		      out     dx, al
seg_bios:081E		      inc     ax
seg_bios:081F		      loop    loc_F081D
seg_bios:0821		      mov     dx, 308h
seg_bios:0824		      mov     ax, 4000h
seg_bios:0827		      out     dx, al
seg_bios:0828		      inc     dx
seg_bios:0829		      mov     al, ah
seg_bios:082B		      out     dx, al
seg_bios:082C		      mov     dx, 30Ah
seg_bios:082F		      pop     ax
seg_bios:0830		      mov     cx, ax
seg_bios:0832		      out     dx, al
seg_bios:0833		      inc     dx
seg_bios:0834		      mov     al, ah
seg_bios:0836		      out     dx, al
seg_bios:0837		      mov     dx, 300h
seg_bios:083A		      mov     al, 0Ah
seg_bios:083C		      out     dx, al
seg_bios:083D		      mov     dx, 310h
seg_bios:0840		      mov     bx, di
seg_bios:0842		      mov     si, 0
seg_bios:0845
seg_bios:0845 loc_F0845:			      ;	CODE XREF: eth_test_buf+64j
seg_bios:0845		      in      al, dx
seg_bios:0846		      cmp     bl, al
seg_bios:0848		      jnz     short loc_F0850
seg_bios:084A		      inc     bx
seg_bios:084B		      loop    loc_F0845
seg_bios:084D		      clc
seg_bios:084E		      jmp     short loc_F0851
seg_bios:0850 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0850
seg_bios:0850 loc_F0850:			      ;	CODE XREF: eth_test_buf+61j
seg_bios:0850		      stc
seg_bios:0851
seg_bios:0851 loc_F0851:			      ;	CODE XREF: eth_test_buf+67j
seg_bios:0851		      popa
seg_bios:0852		      retn
seg_bios:0852 eth_test_buf    endp
seg_bios:0852
seg_bios:0853
seg_bios:0853 ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:0853
seg_bios:0853
seg_bios:0853 eth_rx_wait     proc near		      ;	CODE XREF: eth_loop+11p
seg_bios:0853		      push    cx
seg_bios:0854		      mov     cx, 50
seg_bios:0857
seg_bios:0857 loc_F0857:			      ;	CODE XREF: eth_rx_wait+15j
seg_bios:085A		      push    cx
seg_bios:085B		      xor     cx, cx
seg_bios:085D
seg_bios:085D loc_F085D:			      ;	CODE XREF: eth_rx_wait+12j
seg_bios:085D		      mov     dx, 307h
seg_bios:0860		      in      al, dx
seg_bios:0861		      test    al, 1
seg_bios:0863		      jnz     short loc_F086C
seg_bios:0865		      loop    loc_F085D
seg_bios:0867		      pop     cx
seg_bios:0868		      loop    loc_F0857
seg_bios:086A		      jmp     short loc_F086D
seg_bios:086C ;	───────────────────────────────────────────────────────────────────────────
seg_bios:086C
seg_bios:086C loc_F086C:			      ;	CODE XREF: eth_rx_wait+10j
seg_bios:086C		      pop     cx
seg_bios:086D
seg_bios:086D loc_F086D:			      ;	CODE XREF: eth_rx_wait+17j
seg_bios:086D		      clc
seg_bios:086E		      pop     cx
seg_bios:086F		      retn
seg_bios:086F eth_rx_wait     endp
seg_bios:086F
seg_bios:0870
seg_bios:0870 ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:0870
seg_bios:0870
seg_bios:0870 eth_tx_put      proc near		      ;	CODE XREF: eth_tx_start:loc_F08CFp
seg_bios:0870		      mov     dx, 300h
seg_bios:0873		      mov     al, 22h ;	'"'
seg_bios:0875		      out     dx, al
seg_bios:0876		      mov     dx, 308h
seg_bios:0879		      mov     al, 0
seg_bios:087B		      out     dx, al
seg_bios:087C		      mov     dx, 309h
seg_bios:087F		      mov     al, 40h
seg_bios:0881		      out     dx, al
seg_bios:0882		      mov     dx, 30Ah
seg_bios:0885		      mov     al, 64h
seg_bios:0887		      out     dx, al
seg_bios:0888		      mov     dx, 30Bh
seg_bios:088B		      mov     al, 0
seg_bios:088D		      out     dx, al
seg_bios:088E		      mov     dx, 300h
seg_bios:0891		      mov     al, 12h
seg_bios:0893		      out     dx, al
seg_bios:0894		      mov     cx, 6
seg_bios:0897		      mov     dx, 310h
seg_bios:089A		      xor     si, si
seg_bios:089C
seg_bios:089C loc_F089C:			      ;	CODE XREF: eth_tx_put+35j
seg_bios:089C		      mov     al, cs:mac_buf[si]
seg_bios:08A1		      out     dx, al
seg_bios:08A2		      jmp     short $+2
seg_bios:08A4		      inc     si
seg_bios:08A5		      loop    loc_F089C
seg_bios:08A7		      mov     cx, 5Eh
seg_bios:08AA		      mov     dx, 310h
seg_bios:08AD		      mov     al, cs:eth_mode
seg_bios:08B1
seg_bios:08B1 loc_F08B1:			      ;	CODE XREF: eth_tx_put+46j
seg_bios:08B1		      out     dx, al
seg_bios:08B2		      jmp     short $+2
seg_bios:08B4		      inc     al
seg_bios:08B6		      loop    loc_F08B1
seg_bios:08B8		      mov     dx, 300h
seg_bios:08BB		      mov     al, 22h ;	'"'
seg_bios:08BD		      out     dx, al
seg_bios:08BE		      retn
seg_bios:08BE eth_tx_put      endp
seg_bios:08BE
seg_bios:08BF
seg_bios:08BF ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:08BF
seg_bios:08BF
seg_bios:08BF eth_tx_start    proc near		      ;	CODE XREF: eth_loop+Cp
seg_bios:08BF		      push    cx
seg_bios:08C0		      mov     cx, 7FFFh
seg_bios:08C3
seg_bios:08C3 loc_F08C3:			      ;	CODE XREF: eth_tx_start+Cj
seg_bios:08C3		      mov     dx, io_eth_command
seg_bios:08C6		      in      al, dx
seg_bios:08C7		      test    al, 4
seg_bios:08C9		      jz      short loc_F08CF
seg_bios:08CB		      loop    loc_F08C3
seg_bios:08CD		      jmp     short loc_F08E7
seg_bios:08CF ;	───────────────────────────────────────────────────────────────────────────
seg_bios:08CF
seg_bios:08CF loc_F08CF:			      ;	CODE XREF: eth_tx_start+Aj
seg_bios:08CF		      call    eth_tx_put
seg_bios:08D2		      mov     dx, 305h
seg_bios:08D5		      mov     al, 64h
seg_bios:08D7		      out     dx, al
seg_bios:08D8		      mov     dx, 306h
seg_bios:08DB		      mov     al, 0
seg_bios:08DD		      out     dx, al
seg_bios:08DE		      mov     dx, io_eth_command
seg_bios:08E1		      mov     al, 26h ;	'&'
seg_bios:08E3		      out     dx, al
seg_bios:08E4		      clc
seg_bios:08E5		      jmp     short loc_F08E8
seg_bios:08E7 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:08E7
seg_bios:08E7 loc_F08E7:			      ;	CODE XREF: eth_tx_start+Ej
seg_bios:08E7		      stc
seg_bios:08E8
seg_bios:08E8 loc_F08E8:			      ;	CODE XREF: eth_tx_start+26j
seg_bios:08E8		      pop     cx
seg_bios:08E9		      retn
seg_bios:08E9 eth_tx_start    endp
seg_bios:08E9
seg_bios:08EA
seg_bios:08EA ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:08EA
seg_bios:08EA
seg_bios:08EA eth_rx_check    proc near		      ;	CODE XREF: eth_loop+16p
seg_bios:08EA		      push    cx
seg_bios:08EB		      mov     dx, io_eth_command
seg_bios:08EE		      mov     al, 22h ;	'"'
seg_bios:08F0		      out     dx, al
seg_bios:08F1		      mov     dx, 308h
seg_bios:08F4		      mov     al, 0
seg_bios:08F6		      out     dx, al
seg_bios:08F7		      mov     dx, 309h
seg_bios:08FA		      mov     al, cs:rx_page
seg_bios:08FE		      out     dx, al
seg_bios:08FF		      mov     dx, 30Ah
seg_bios:0902		      mov     al, 4
seg_bios:0904		      out     dx, al
seg_bios:0905		      mov     dx, 30Bh
seg_bios:0908		      mov     al, 0
seg_bios:090A		      out     dx, al
seg_bios:090B		      mov     dx, 300h
seg_bios:090E		      mov     al, 0Ah	      ;	remote DMA read	start
seg_bios:0910		      out     dx, al
seg_bios:0911		      mov     dx, io_eth_data ;	read status and	length words
seg_bios:0914		      in      al, dx
seg_bios:0915		      nop
seg_bios:0916		      in      al, dx
seg_bios:0917		      mov     bl, al
seg_bios:0919		      in      al, dx
seg_bios:091A		      mov     cl, al
seg_bios:091C		      in      al, dx
seg_bios:091D		      mov     ch, al
seg_bios:091F		      sub     cx, 4
seg_bios:0922		      mov     dx, io_eth_command
seg_bios:0925		      mov     al, 22h ;	'"'
seg_bios:0927		      out     dx, al
seg_bios:0928		      mov     dx, 308h
seg_bios:092B		      mov     al, 4
seg_bios:092D		      out     dx, al
seg_bios:092E		      mov     dx, 309h
seg_bios:0931		      mov     al, cs:rx_page
seg_bios:0935		      out     dx, al
seg_bios:0936		      mov     dx, 30Ah
seg_bios:0939		      mov     al, cl
seg_bios:093B		      out     dx, al
seg_bios:093C		      mov     dx, 30Bh
seg_bios:093F		      mov     al, ch
seg_bios:0941		      out     dx, al
seg_bios:0942		      mov     dx, io_eth_command
seg_bios:0945		      mov     al, 0Ah
seg_bios:0947		      out     dx, al
seg_bios:0948		      mov     di, cx
seg_bios:094A		      mov     cx, 6
seg_bios:094D		      mov     bh, 0
seg_bios:094F		      mov     dx, io_eth_data
seg_bios:0952		      xor     si, si
seg_bios:0954
seg_bios:0954 loc_F0954:			      ;	CODE XREF: eth_rx_check+73j
seg_bios:0954		      in      al, dx
seg_bios:0955		      cmp     al, cs:mac_buf[si]
seg_bios:095A		      jnz     short loc_F0978
seg_bios:095C		      inc     si
seg_bios:095D		      loop    loc_F0954
seg_bios:095F		      mov     cx, di
seg_bios:0961		      sub     cx, 6
seg_bios:0964		      mov     bh, cs:eth_mode
seg_bios:0969		      mov     dx, io_eth_data
seg_bios:096C
seg_bios:096C loc_F096C:			      ;	CODE XREF: eth_rx_check+89j
seg_bios:096C		      in      al, dx
seg_bios:096D		      cmp     al, bh
seg_bios:096F		      jnz     short loc_F0978
seg_bios:0971		      inc     bh
seg_bios:0973		      loop    loc_F096C
seg_bios:0975		      clc
seg_bios:0976		      jmp     short loc_F0979
seg_bios:0978 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0978
seg_bios:0978 loc_F0978:			      ;	CODE XREF: eth_rx_check+70j
seg_bios:0978					      ;	eth_rx_check+85j
seg_bios:0978		      stc
seg_bios:0979
seg_bios:0979 loc_F0979:			      ;	CODE XREF: eth_rx_check+8Cj
seg_bios:0979		      pushf
seg_bios:097A		      mov     cs:rx_page, bl
seg_bios:097F		      cmp     bl, 46h
seg_bios:0982		      jnz     short loc_F0988
seg_bios:0984		      mov     bl, 7Fh
seg_bios:0986		      jmp     short loc_F098A
seg_bios:0988 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0988
seg_bios:0988 loc_F0988:			      ;	CODE XREF: eth_rx_check+98j
seg_bios:0988		      dec     bl
seg_bios:098A
seg_bios:098A loc_F098A:			      ;	CODE XREF: eth_rx_check+9Cj
seg_bios:098A		      mov     dx, 303h
seg_bios:098D		      mov     al, bl
seg_bios:098F		      out     dx, al
seg_bios:0990
seg_bios:0990 loc_F0990:
seg_bios:0990		      popf
seg_bios:0991		      pop     cx
seg_bios:0992		      retn
seg_bios:0992 eth_rx_check    endp
seg_bios:0992
seg_bios:0993
seg_bios:0993 ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:0993
seg_bios:0993
seg_bios:0993 eth_delay_frame proc near		      ;	CODE XREF: eth_loop+1Bp
seg_bios:0993		      push    cx
seg_bios:0994		      mov     cx, 5
seg_bios:0997
seg_bios:0997 loc_F0997:			      ;	CODE XREF: eth_delay_frame+Dj
seg_bios:099A		      push    cx
seg_bios:099B		      xor     cx, cx
seg_bios:099D
seg_bios:099D loc_F099D:			      ;	CODE XREF: eth_delay_frame:loc_F099Dj
seg_bios:099D		      loop    loc_F099D
seg_bios:099F		      pop     cx
seg_bios:09A0		      loop    loc_F0997
seg_bios:09A2		      pop     cx
seg_bios:09A3		      retn
seg_bios:09A3 eth_delay_frame endp
seg_bios:09A3
seg_bios:09A4
seg_bios:09A4 ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:09A4
seg_bios:09A4
seg_bios:09A4 eth_int_clear   proc near		      ;	CODE XREF: eth_loop:loc_F09BEp
seg_bios:09A4		      push    dx
seg_bios:09A5		      push    ax
seg_bios:09A6		      mov     dx, 300h
seg_bios:09A9		      mov     al, 22h ;	'"'
seg_bios:09AB		      out     dx, al
seg_bios:09AC		      mov     al, 0FFh
seg_bios:09AE		      mov     dx, 307h
seg_bios:09B1		      out     dx, al
seg_bios:09B2		      pop     ax
seg_bios:09B3		      pop     dx
seg_bios:09B4		      retn
seg_bios:09B4 eth_int_clear   endp
seg_bios:09B4
seg_bios:09B5
seg_bios:09B5 ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:09B5
seg_bios:09B5
seg_bios:09B5 eth_loop	      proc near		      ;	CODE XREF: eth_loop_int+28p
seg_bios:09B5					      ;	eth_loop_int+4Fp ...
seg_bios:09B5		      mov     dx, io_eth_command
seg_bios:09B8		      mov     al, 22h ;	'"'
seg_bios:09BA		      out     dx, al
seg_bios:09BB		      mov     cx, 5
seg_bios:09BE
seg_bios:09BE loc_F09BE:			      ;	CODE XREF: eth_loop+1Ej
seg_bios:09BE		      call    eth_int_clear
seg_bios:09C1		      call    eth_tx_start
seg_bios:09C4		      jb      short loc_F09D8
seg_bios:09C6		      call    eth_rx_wait
seg_bios:09C9		      jb      short loc_F09D8
seg_bios:09CB		      call    eth_rx_check
seg_bios:09CE		      jb      short loc_F09D8
seg_bios:09D0		      call    eth_delay_frame
seg_bios:09D3		      loop    loc_F09BE
seg_bios:09D5		      clc
seg_bios:09D6		      jmp     short loc_F09D9
seg_bios:09D8 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:09D8
seg_bios:09D8 loc_F09D8:			      ;	CODE XREF: eth_loop+Fj
seg_bios:09D8					      ;	eth_loop+14j ...
seg_bios:09D8		      stc
seg_bios:09D9
seg_bios:09D9 loc_F09D9:			      ;	CODE XREF: eth_loop+21j
seg_bios:09D9		      call    print_result
seg_bios:09DC		      retn
seg_bios:09DC eth_loop	      endp
seg_bios:09DC
seg_bios:09DD
seg_bios:0A43
seg_bios:0A43 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0A44 a100mbFullDuplex___ db '    100Mb & Full Duplex ... '
seg_bios:0A44					      ;	DATA XREF: eth_loop_int+6o
seg_bios:0A44					      ;	eth_loop_ext+6o
seg_bios:0A60 a100mbHalfDuplex___ db '    100Mb & Half Duplex ... '
seg_bios:0A60					      ;	DATA XREF: eth_loop_int+2Do
seg_bios:0A7C a10mbFullDuplex___ db '    10Mb & Full Duplex ... '
seg_bios:0A7C					      ;	DATA XREF: eth_loop_int+54o
seg_bios:0A7C					      ;	eth_loop_ext+2Do
seg_bios:0A97 a10bHalfDuplex___	db '    10b & Half Duplex ... '
seg_bios:0A97					      ;	DATA XREF: eth_loop_int+7Bo
seg_bios:0AB1
seg_bios:0AB1 ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:0AB1
seg_bios:0AB1

;-------------------------------------------------------------------------------
; Internal loopback test
;-------------------------------------------------------------------------------

seg_bios:0AB1 eth_loop_int    proc near		      ;	CODE XREF: eth_test+39p
seg_bios:0AB1		      push    es
seg_bios:0AB2		      push    bp
seg_bios:0AB3		      push    dx
seg_bios:0AB4		      push    cx
seg_bios:0AB5		      push    ax
seg_bios:0AB6		      push    bx
seg_bios:0AB7		      mov     bp, offset a100mbFullDuplex___ ; "    100Mb & Full Duplex	... "
seg_bios:0ABA		      mov     cx, 1Ch
seg_bios:0ABD		      call    root_print
seg_bios:0AC0		      call    eth_init
seg_bios:0AC3		      call    phy_100_fd
seg_bios:0AC6		      mov     bl, 7
seg_bios:0AC8		      call    phy_stat_wait
seg_bios:0ACB		      mov     bl, 82h ;	'é'
seg_bios:0ACD		      call    eth_tx_conf_set
seg_bios:0AD0		      call    eth_mac_set
seg_bios:0AD3		      mov     cs:eth_mode, 1
seg_bios:0AD9		      call    eth_loop
seg_bios:0ADC		      jb      short loc_F0B51
seg_bios:0ADE		      mov     bp, offset a100mbHalfDuplex___ ; "    100Mb & Half Duplex	... "
seg_bios:0AE1		      mov     cx, 1Ch
seg_bios:0AE4		      call    root_print
seg_bios:0AE7		      call    eth_init
seg_bios:0AEA		      call    phy_100_hd
seg_bios:0AED		      mov     bl, 5
seg_bios:0AEF		      call    phy_stat_wait
seg_bios:0AF2		      mov     bl, 2
seg_bios:0AF4		      call    eth_tx_conf_set
seg_bios:0AF7		      call    eth_mac_set
seg_bios:0AFA		      mov     cs:eth_mode, 2
seg_bios:0B00		      call    eth_loop
seg_bios:0B03		      jb      short loc_F0B51
seg_bios:0B05		      mov     bp, offset a10mbFullDuplex___ ; "	   10Mb	& Full Duplex ... "
seg_bios:0B08		      mov     cx, 1Bh
seg_bios:0B0B		      call    root_print
seg_bios:0B0E		      call    eth_init
seg_bios:0B11		      call    phy_10_fd
seg_bios:0B14		      mov     bl, 3
seg_bios:0B16		      call    phy_stat_wait
seg_bios:0B19		      mov     bl, 82h ;	'é'
seg_bios:0B1B		      call    eth_tx_conf_set
seg_bios:0B1E		      call    eth_mac_set
seg_bios:0B21		      mov     cs:eth_mode, 3
seg_bios:0B27		      call    eth_loop
seg_bios:0B2A		      jb      short loc_F0B51
seg_bios:0B2C		      mov     bp, offset a10bHalfDuplex___ ; "	  10b &	Half Duplex ...	"
seg_bios:0B2F		      mov     cx, 1Ah
seg_bios:0B32		      call    root_print
seg_bios:0B35		      call    eth_init
seg_bios:0B38		      call    phy_10_hd
seg_bios:0B3B		      mov     bl, 1
seg_bios:0B3D		      call    phy_stat_wait
seg_bios:0B40
seg_bios:0B40 loc_F0B40:
seg_bios:0B40		      mov     bl, 2
seg_bios:0B42		      call    eth_tx_conf_set
seg_bios:0B45		      call    eth_mac_set
seg_bios:0B48		      mov     cs:eth_mode, 4
seg_bios:0B4E		      call    eth_loop
seg_bios:0B51
seg_bios:0B51 loc_F0B51:			      ;	CODE XREF: eth_loop_int+2Bj
seg_bios:0B51					      ;	eth_loop_int+52j ...
seg_bios:0B51		      pop     bx
seg_bios:0B52		      pop     ax
seg_bios:0B53		      pop     cx
seg_bios:0B54		      pop     dx
seg_bios:0B55		      pop     bp
seg_bios:0B56		      pop     es
seg_bios:0B57		      retn
seg_bios:0B57 eth_loop_int    endp
seg_bios:0B57

;-------------------------------------------------------------------------------
; External loopback test
;-------------------------------------------------------------------------------

seg_bios:0B58
seg_bios:0B58 eth_loop_ext    proc near		      ;	CODE XREF: eth_test+47p
seg_bios:0B58		      push    es
seg_bios:0B59		      push    bp
seg_bios:0B5A		      push    dx
seg_bios:0B5B		      push    cx
seg_bios:0B5C		      push    ax
seg_bios:0B5D		      push    bx
seg_bios:0B5E		      mov     bp, offset a100mbFullDuplex___ ; "    100Mb & Full Duplex	... "
seg_bios:0B61		      mov     cx, 1Ch
seg_bios:0B64		      call    root_print
seg_bios:0B67		      call    eth_init
seg_bios:0B6A		      call    phy_100_fd
seg_bios:0B6D		      mov     bl, 7
seg_bios:0B6F		      call    phy_stat_wait
seg_bios:0B72		      mov     bl, 80h ;	'Ç'
seg_bios:0B74		      call    eth_tx_conf_set
seg_bios:0B77		      call    eth_mac_set
seg_bios:0B7A		      mov     cs:eth_mode, 5
seg_bios:0B80		      call    eth_loop
seg_bios:0B83		      jb      short loc_F0BAA
seg_bios:0B85		      mov     bp, offset a10mbFullDuplex___ ; "	   10Mb	& Full Duplex ... "
seg_bios:0B88		      mov     cx, 1Bh
seg_bios:0B8B		      call    root_print
seg_bios:0B8E		      call    eth_init
seg_bios:0B91		      call    phy_10_fd
seg_bios:0B94		      mov     bl, 3
seg_bios:0B96		      call    phy_stat_wait
seg_bios:0B99		      mov     bl, 80h ;	'Ç'
seg_bios:0B9B		      call    eth_tx_conf_set
seg_bios:0B9E		      call    eth_mac_set
seg_bios:0BA1		      mov     cs:eth_mode, 6
seg_bios:0BA7		      call    eth_loop
seg_bios:0BAA
seg_bios:0BAA loc_F0BAA:			      ;	CODE XREF: eth_loop_ext+2Bj
seg_bios:0BAA		      pop     bx
seg_bios:0BAB		      pop     ax
seg_bios:0BAC		      pop     cx
seg_bios:0BAD		      pop     dx
seg_bios:0BAE		      pop     bp
seg_bios:0BAF		      pop     es
seg_bios:0BB0		      retn
seg_bios:0BB0 eth_loop_ext    endp
seg_bios:0BB0
seg_bios:0BB1




seg_bios:0E60
seg_bios:0E60 ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:0E60
seg_bios:0E60
seg_bios:0E60 int_60h	      proc far		      ;	DATA XREF: vect_setup+12Co
seg_bios:0E60		      push    ds
seg_bios:0E61		      push    es
seg_bios:0E62		      push    si
seg_bios:0E63		      push    di
seg_bios:0E64		      push    dx
seg_bios:0E65		      push    cx
seg_bios:0E66		      push    bx
seg_bios:0E67		      mov     bx, 6800h
seg_bios:0E6A		      mov     es, bx
seg_bios:0E6C		      assume es:nothing
seg_bios:0E6C		      cmp     ah, 21h ;	'!'
seg_bios:0E6F		      jz      short int_60h_21h
seg_bios:0E71		      cmp     ah, 40h
seg_bios:0E74		      jnz     short loc_F0E79
seg_bios:0E76		      jmp     int_60h_40h
seg_bios:0E79 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0E79
seg_bios:0E79 loc_F0E79:			      ;	CODE XREF: int_60h+14j
seg_bios:0E79		      cmp     ah, 41h
seg_bios:0E7C		      jnz     short loc_F0E81
seg_bios:0E7E		      jmp     int_60h_41h
seg_bios:0E81 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0E81
seg_bios:0E81 loc_F0E81:			      ;	CODE XREF: int_60h+1Cj
seg_bios:0E81		      cmp     ah, 42h
seg_bios:0E84		      jnz     short loc_F0E89
seg_bios:0E86		      jmp     int_60h_42h
seg_bios:0E89 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0E89
seg_bios:0E89 loc_F0E89:			      ;	CODE XREF: int_60h+24j
seg_bios:0E89		      cmp     ah, 80h
seg_bios:0E8C		      jnz     short loc_F0E91
seg_bios:0E8E		      jmp     int_60h_80h
seg_bios:0E91 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0E91
seg_bios:0E91 loc_F0E91:			      ;	CODE XREF: int_60h+2Cj
seg_bios:0E91		      cmp     ah, 81h
seg_bios:0E94		      jnz     short loc_F0E99
seg_bios:0E96		      jmp     int_60h_81h
seg_bios:0E99 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0E99
seg_bios:0E99 loc_F0E99:			      ;	CODE XREF: int_60h+34j
seg_bios:0E99		      cmp     ah, 82h
seg_bios:0E9C		      jnz     short loc_F0EA1
seg_bios:0E9E		      jmp     int_60h_82h
seg_bios:0EA1 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0EA1
seg_bios:0EA1 loc_F0EA1:			      ;	CODE XREF: int_60h+3Cj
seg_bios:0EA1		      cmp     ah, 0A0h
seg_bios:0EA4		      jnz     short loc_F0EA9
seg_bios:0EA6		      jmp     int_60h_A0h
seg_bios:0EA9 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0EA9
seg_bios:0EA9 loc_F0EA9:			      ;	CODE XREF: int_60h+44j
seg_bios:0EA9		      cmp     ah, 0A1h
seg_bios:0EAC		      jnz     short int_60h_exit
seg_bios:0EAE		      jmp     int_60h_A1h
seg_bios:0EB1 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0EB1
seg_bios:0EB1 int_60h_exit:			      ;	CODE XREF: int_60h+4Cj
seg_bios:0EB1					      ;	int_60h:loc_F0F06j ...
seg_bios:0EB1		      pop     bx
seg_bios:0EB2		      pop     cx
seg_bios:0EB3		      pop     dx
seg_bios:0EB4		      pop     di
seg_bios:0EB5		      pop     si
seg_bios:0EB6		      pop     es
seg_bios:0EB7		      assume es:nothing
seg_bios:0EB7		      pop     ds
seg_bios:0EB8		      iret
seg_bios:0EB8 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0EB9		      db 0EBh ;	Ù
seg_bios:0EBA		      db 0F6h ;	÷
seg_bios:0EBB ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0EBB
seg_bios:0EBB int_60h_21h:			      ;	CODE XREF: int_60h+Fj
seg_bios:0EBB		      sti
seg_bios:0EBC
seg_bios:0EBC loc_F0EBC:			      ;	CODE XREF: int_60h:loc_F0F04j
seg_bios:0EBC		      mov     ax, si
seg_bios:0EBE		      and     ax, 1Fh
seg_bios:0EC1		      jnz     short loc_F0EEF
seg_bios:0EC3		      mov     al, 0Dh
seg_bios:0EC5		      call    print_char
seg_bios:0EC8		      mov     al, 0Ah
seg_bios:0ECA		      call    print_char
seg_bios:0ECD		      mov     ax, ds
seg_bios:0ECF		      xchg    ah, al
seg_bios:0ED1		      call    print_hex_byte
seg_bios:0ED4		      xchg    al, ah
seg_bios:0ED6		      call    print_hex_byte
seg_bios:0ED9		      mov     al, 3Ah ;	':'
seg_bios:0EDB		      call    print_char
seg_bios:0EDE		      mov     ax, si
seg_bios:0EE0		      xchg    ah, al
seg_bios:0EE2		      call    print_hex_byte
seg_bios:0EE5		      xchg    al, ah
seg_bios:0EE7		      call    print_hex_byte
seg_bios:0EEA		      mov     al, 20h ;	' '
seg_bios:0EEC		      call    print_char
seg_bios:0EEF
seg_bios:0EEF loc_F0EEF:			      ;	CODE XREF: int_60h+61j
seg_bios:0EEF		      lodsb
seg_bios:0EF0		      call    print_hex_byte
seg_bios:0EF3		      dec     cx
seg_bios:0EF4		      jz      short loc_F0F06
seg_bios:0EF6		      mov     ah, 10h
seg_bios:0EF8		      int     16h	      ;	KEYBOARD - GET ENHANCED	KEYSTROKE (AT model 339,XT2,XT286,PS)
seg_bios:0EF8					      ;	Return:	AH = scan code,	AL = character
seg_bios:0EFA		      or      ah, ah
seg_bios:0EFC		      jnz     short loc_F0F04
seg_bios:0EFE		      cmp     al, 3
seg_bios:0F00		      jnz     short loc_F0F04
seg_bios:0F02		      jmp     short loc_F0F06
seg_bios:0F04 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0F04
seg_bios:0F04 loc_F0F04:			      ;	CODE XREF: int_60h+9Cj
seg_bios:0F04					      ;	int_60h+A0j
seg_bios:0F04		      jmp     short loc_F0EBC
seg_bios:0F06 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0F06
seg_bios:0F06 loc_F0F06:			      ;	CODE XREF: int_60h+94j
seg_bios:0F06					      ;	int_60h+A2j
seg_bios:0F06		      jmp     short int_60h_exit
seg_bios:0F08 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0F08
seg_bios:0F08 int_60h_40h:			      ;	CODE XREF: int_60h+16j
seg_bios:0F08		      xor     ah, ah
seg_bios:0F0A		      shl     al, 1
seg_bios:0F0C		      shl     al, 1
seg_bios:0F0E		      shl     al, 1
seg_bios:0F10		      add     ax, 420h
seg_bios:0F13		      mov     bx, ax
seg_bios:0F15		      mov     ax, es:[bx]
seg_bios:0F18		      jmp     short int_60h_exit
seg_bios:0F1A ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0F1A
seg_bios:0F1A int_60h_41h:			      ;	CODE XREF: int_60h+1Ej
seg_bios:0F1A		      call    eth_60h_41h
seg_bios:0F1D		      jmp     short int_60h_exit
seg_bios:0F1F ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0F1F
seg_bios:0F1F int_60h_42h:			      ;	CODE XREF: int_60h+26j
seg_bios:0F1F		      call    eth_60h_42h
seg_bios:0F22		      jmp     short int_60h_exit
seg_bios:0F24 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0F24
seg_bios:0F24 int_60h_80h:			      ;	CODE XREF: int_60h+2Ej
seg_bios:0F24		      call    eth_init
seg_bios:0F27		      jmp     short int_60h_exit
seg_bios:0F29 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0F29
seg_bios:0F29 int_60h_81h:			      ;	CODE XREF: int_60h+36j
seg_bios:0F29		      call    eth_60h_81h
seg_bios:0F2C		      jmp     short int_60h_exit
seg_bios:0F2E ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0F2E
seg_bios:0F2E int_60h_82h:			      ;	CODE XREF: int_60h+3Ej
seg_bios:0F2E		      call    eth_60h_82h
seg_bios:0F31		      jmp     int_60h_exit
seg_bios:0F34 ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0F34
seg_bios:0F34 int_60h_A0h:			      ;	CODE XREF: int_60h+46j
seg_bios:0F34		      mov     ax, 7E00h
seg_bios:0F37		      mov     es, ax
seg_bios:0F39		      assume es:nothing
seg_bios:0F39		      mov     di, 100h
seg_bios:0F3C		      jmp     int_60h_exit
seg_bios:0F3F ;	───────────────────────────────────────────────────────────────────────────
seg_bios:0F3F
seg_bios:0F3F int_60h_A1h:			      ;	CODE XREF: int_60h+4Ej
seg_bios:0F3F		      mov     ax, 7E00h
seg_bios:0F42		      mov     es, ax
seg_bios:0F44		      mov     di, 100h
seg_bios:0F47		      jmp     int_60h_exit
seg_bios:0F47 int_60h	      endp
seg_bios:0F47
seg_bios:0F4A
seg_bios:0F4A ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:0F4A
seg_bios:0F4A
seg_bios:0F4A int0_hand	      proc near		      ;	CODE XREF: int_int0+3p
seg_bios:0F4A		      push    es
seg_bios:0F4B		      push    dx
seg_bios:0F4C		      push    bx
seg_bios:0F4D		      mov     bx, 1FFFh
seg_bios:0F50		      mov     es, bx
seg_bios:0F52		      assume es:nothing
seg_bios:0F52		      xor     bx, bx
seg_bios:0F54		      mov     ax, es:[bx]
seg_bios:0F57		      inc     ax
seg_bios:0F58		      mov     es:[bx], ax
seg_bios:0F5B		      pop     bx
seg_bios:0F5C		      pop     dx
seg_bios:0F5D		      pop     es
seg_bios:0F5E		      assume es:nothing
seg_bios:0F5E		      mov     ax, 0Ch
seg_bios:0F61		      mov     dx, io_eoi
seg_bios:0F64		      out     dx, ax
seg_bios:0F65		      retn
seg_bios:0F65 int0_hand	      endp
seg_bios:0F65
seg_bios:0F66
seg_bios:0F66 ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:0F66
seg_bios:0F66
seg_bios:0F66 eth_60h_41h     proc near		      ;	CODE XREF: int_60h:int_60h_41hp
seg_bios:0F66		      mov     dx, 30Dh
seg_bios:0F69		      xor     al, al
seg_bios:0F6B		      out     dx, al
seg_bios:0F6C		      mov     dx, 300h
seg_bios:0F6F		      mov     al, 22h ;	'"'
seg_bios:0F71		      out     dx, al
seg_bios:0F72		      retn
seg_bios:0F72 eth_60h_41h     endp
seg_bios:0F72
seg_bios:0F73
seg_bios:0F73 ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:0F73
seg_bios:0F73
seg_bios:0F73 eth_60h_42h     proc near		      ;	CODE XREF: int_60h:int_60h_42hp
seg_bios:0F73		      mov     dx, 300h
seg_bios:0F76		      mov     al, 21h ;	'!'
seg_bios:0F78		      out     dx, al
seg_bios:0F79		      retn
seg_bios:0F79 eth_60h_42h     endp
seg_bios:0F79
seg_bios:0F7A
seg_bios:0F7A ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:0F7A
seg_bios:0F7A
seg_bios:0F7A eth_60h_81h     proc near		      ;	CODE XREF: int_60h:int_60h_81hp
seg_bios:0F7A		      push    ds
seg_bios:0F7B		      push    si
seg_bios:0F7C		      push    dx
seg_bios:0F7D		      push    cx
seg_bios:0F7E		      push    ax
seg_bios:0F7F		      mov     dx, 300h
seg_bios:0F82		      in      al, dx
seg_bios:0F83		      push    ax
seg_bios:0F84		      and     al, 3Fh
seg_bios:0F86		      or      al, 40h
seg_bios:0F88		      out     dx, al
seg_bios:0F89		      mov     dx, 301h
seg_bios:0F8C		      mov     cx, 6
seg_bios:0F8F
seg_bios:0F8F loc_F0F8F:			      ;	CODE XREF: eth_60h_81h+18j
seg_bios:0F8F		      lodsb
seg_bios:0F90		      out     dx, al
seg_bios:0F91		      inc     dx
seg_bios:0F92		      loop    loc_F0F8F
seg_bios:0F94		      mov     dx, 300h
seg_bios:0F97		      pop     ax
seg_bios:0F98		      out     dx, al
seg_bios:0F99		      pop     ax
seg_bios:0F9A		      pop     cx
seg_bios:0F9B		      pop     dx
seg_bios:0F9C		      pop     si
seg_bios:0F9D		      pop     ds
seg_bios:0F9E		      retn
seg_bios:0F9E eth_60h_81h     endp
seg_bios:0F9E
seg_bios:0F9F
seg_bios:0F9F ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:0F9F
seg_bios:0F9F
seg_bios:0F9F eth_60h_82h     proc near		      ;	CODE XREF: int_60h:int_60h_82hp
seg_bios:0F9F		      push    dx
seg_bios:0FA0		      push    ax
seg_bios:0FA1		      push    ax
seg_bios:0FA2		      mov     dx, 300h
seg_bios:0FA5		      in      al, dx
seg_bios:0FA6		      and     al, 3Fh
seg_bios:0FA8		      out     dx, al
seg_bios:0FA9		      mov     dx, 30Ch
seg_bios:0FAC		      pop     ax
seg_bios:0FAD		      out     dx, al
seg_bios:0FAE		      pop     ax
seg_bios:0FAF		      pop     dx
seg_bios:0FB0		      retn
seg_bios:0FB0 eth_60h_82h     endp
seg_bios:0FB0
seg_bios:0FB1
seg_bios:0FB1 ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:0FB1
seg_bios:0FB1
seg_bios:0FB1 ins_get	      proc near		      ;	CODE XREF: dio_test+1Bp
seg_bios:0FB1		      push    dx
seg_bios:0FB2		      xor     ax, ax
seg_bios:0FB4		      mov     dx, 318h
seg_bios:0FB7		      in      al, dx
seg_bios:0FB8		      pop     dx
seg_bios:0FB9		      retn
seg_bios:0FB9 ins_get	      endp
seg_bios:0FB9
seg_bios:0FBA
seg_bios:0FBA ;	███████████████	S U B R	O U T I	N E ███████████████████████████████████████
seg_bios:0FBA
seg_bios:0FBA
seg_bios:0FBA outs_get	      proc near		      ;	CODE XREF: eth_init+6Dp
seg_bios:0FBA					      ;	dio_test+4Bp ...
seg_bios:0FBA		      push    dx
seg_bios:0FBB		      push    bx
seg_bios:0FBC		      mov     bx, ax
seg_bios:0FBE		      mov     dx, 31Ah
seg_bios:0FC1		      in      al, dx
seg_bios:0FC2		      and     al, 0F0h
seg_bios:0FC4		      xor     bl, 0Bh
seg_bios:0FC7		      and     bl, 0Fh
seg_bios:0FCA		      or      al, bl
seg_bios:0FCC		      out     dx, al
seg_bios:0FCD		      pop     bx
seg_bios:0FCE		      pop     dx
seg_bios:0FCF		      retn
seg_bios:0FCF outs_get	      endp
seg_bios:0FCF
seg_bios:0FD0