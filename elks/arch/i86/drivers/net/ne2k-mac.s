;-------------------------------------------------------------------------------
; NE2K driver - low part - MAC routines
;-------------------------------------------------------------------------------


; Mem base

mem_rx_count      EQU 0


; I/O base @ 300h

io_eth_command    EQU $300

io_eth_rx_first   EQU $301  ; page 0
io_eth_rx_last    EQU $302  ; page 0
io_eth_rx_get     EQU $303  ; page 0

io_eth_tx_start   EQU $304  ; page 0 - write
io_eth_tx_len1    EQU $305  ; page 0 - write
io_eth_tx_len2    EQU $306  ; page 0 - write

io_eth_int_stat   EQU $307  ; page 0

io_eth_dma_addr1  EQU $308  ; page 0
io_eth_dma_addr2  EQU $309  ; page 0
io_eth_dma_len1   EQU $30A  ; page 0 - write
io_eth_dma_len2   EQU $30B  ; page 0 - write

io_eth_rx_stat    EQU $30C  ; page 0 - read
io_eth_rx_conf    EQU $30C  ; page 0 - write
io_eth_tx_conf    EQU $30D  ; page 0 - write
io_eth_data_conf  EQU $30E  ; page 0 - write
io_eth_int_mask   EQU $30F  ; page 0 - write

io_eth_unicast    EQU $301  ; page 1 - 6 bytes
io_eth_rx_put_2   EQU $307  ; page 1
io_eth_multicast  EQU $308  ; page 1 - 8 bytes

io_eth_data_io    EQU $310  ; 2 bytes


	.TEXT

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
seg_bios:0696		      mov     dx, io_eth_command
seg_bios:0699		      mov     al, 21h ;	'!'
seg_bios:069B		      out     dx, al
seg_bios:069C		      mov     dx, io_eth_tx_conf
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

	mov     ah, al
	and     ah, #$FE
	shl     ah, #6

	mov     dx, #io_eth_command
	in      al, dx
	and     al, #$3F
	or      al, ah
        out     dx, al

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

	mov     al, #1
        call    eth_page_sel

	; load MAC address

	mov     dx, #io_eth_unicast
	mov     cx, #6
	cld

ems_loop:

	lodsb
	out     dx, al
	inc     dx
	loop    ems_loop

	pop     si
	pop     dx
	pop     cx
	pop     ax
	ret


;-------------------------------------------------------------------------------
; Stop MAC and abort DMA
;-------------------------------------------------------------------------------

eth_stop:

	push    ax
	push    dx

	mov     dx, #io_eth_command
	in      al, dx
	and     al, #$C0
	or      al, #$21
	out     dx, al

	pop     dx
	pop     ax
	ret


;-------------------------------------------------------------------------------
; Configure MAC
;-------------------------------------------------------------------------------

eth_init:

	push    ax
	push    dx

	call    eth_stop

	xor     al,al
	call    eth_page_sel

	; data I/O in single bytes for 80188

	mov     dx, #io_eth_data_conf
	mov     al, #$48
	out     dx, al

	; accept all packet without error
	; broadcast, multicast and promiscuous

	mov     dx, #io_eth_rx_conf
	mov     al, #$5C
	out     dx, al

	; half-duplex and internal loopback

	mov     dx, #io_eth_tx_conf
	mov     al, #2
	out     dx, al

	; set RX ring limits
	; all 16KB on-chip memory
	; except one TX frame at beginning (6 x 256B)

	mov     dx, #io_eth_rx_first
	mov     al, #$46
	out     dx, al

	mov     dx, #io_eth_rx_last
	mov     al, #$80
	out     dx, al

	mov     dx, #io_eth_tx_start
	mov     al, #$40
	out     dx, al

	; set RX get pointer to start

	mov     dx, #io_eth_rx_get
	mov     al, #$46
	out     dx, al

	; clear all interrupt flags

	mov     dx, #io_eth_int_stat
	mov     al, #$FF
	out     dx, al

	; set interrupt mask

	mov     dx, #io_eth_int_mask
	mov     al, #$1F
	out     dx, al

	; select page 1

	mov     al, #1
	call    eth_page_sel

	; clear unicast address

	mov     cx, #6
	mov     dx, #io_eth_unicast
	xor     al, al

ei_loop_u:

	out     dx, al
	inc     dx
	loop    ei_loop_u

	; clear multicast bitmap

	mov     cx, #8
	mov     dx, #io_eth_multicast
	xor     al, al

ei_loop_m:

	out     dx, al
	inc     dx
	loop    ei_loop_m


	; set RX put pointer to start + 1

	mov     dx, #io_eth_rx_put_2
	mov     al, #$47
	out     dx, al

	pop     dx
	pop     ax
	ret


;-------------------------------------------------------------------------------
; DMA init
;-------------------------------------------------------------------------------

CX = byte count
BX = chip memory address

eth_dma_init:

	push    ax
	push    bx
	push    cx
	push    dx

	; select page 0

	xor     al, al
	call    eth_page_sel

	; set DMA start address

	mov     dx, #io_eth_dma_addr1
	mov     al, bl
	out     dx, al
	inc     dx
	mov     al, bh
	out     dx, al

	; set DMA byte count

	mov     dx, #io_eth_dma_len1
	mov     al, cl
	out     dx, al
	inc     dx
	mov     al, ch
	out     dx, al

	pop     dx
	pop     cx
	pop     bx
	pop     ax
	ret


;-------------------------------------------------------------------------------
; DMA write
;-------------------------------------------------------------------------------

DS:SI = host memory to read from
CX = byte count
BX = chip memory to write to

eth_dma_write:

	push    ax

	call    eth_dma_init

	; start DMA write

	???TBC???

	; I/O write loop

	mov     dx, #io_eth_data_io

emw_loop:

	lodsb
	out     dx, al
	loop    emw_loop

	; maybe check DMA stopped ?

	pop     si
	pop     dx
	pop     cx
	pop     bx
	pop     ax
	ret


;-------------------------------------------------------------------------------
; DMA read
;-------------------------------------------------------------------------------

ES:DI = host memory to write to
CX = byte count
BX = chip memory to read from

eth_dma_read:

	push    ax
	push    cx
	push    dx
	push    si

	call    eth_dma_init

	; start DMA read

	???TBC???

	; I/O read loop

	mov     dx, #io_eth_data_io

emr_loop:

	in      al, dx
	stosb
	loop    emr_loop

	; maybe check DMA stopped ?

	pop     si
	pop     dx
	pop     cx
	pop     ax
	ret


;-------------------------------------------------------------------------------

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
seg_bios:085D		      mov     dx, io_eth_int_stat
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
seg_bios:08D2		      mov     dx, io_eth_tx_len1
seg_bios:08D5		      mov     al, 64h
seg_bios:08D7		      out     dx, al
seg_bios:08D8		      mov     dx, io_eth_tx_len2
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
seg_bios:09A6		      mov     dx, io_eth_command
seg_bios:09A9		      mov     al, 22h ;	'"'
seg_bios:09AB		      out     dx, al

seg_bios:09AC		      mov     al, 0FFh
seg_bios:09AE		      mov     dx, #io_eth_int_stat
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


;-------------------------------------------------------------------------------
; Interrupt handler
;-------------------------------------------------------------------------------

; On Advantech SNMP-1000 SBC, the ethernet interrupt is INT0

int_eth:

	push    ax
	push    bx
	push    dx
	push    ds

	; get static data segment

	mov     ax, #$40
	mov     ds, ax

	; increment received frames count

	mov     bx, #mem_rx_count
	mov     ax, [bx]
	inc     ax
	mov     [bx], ax

	; end of interrupt handling

	mov     ax, #$0C
	mov     dx, #io_int_end
	out     dx, ax
	
	pop     ds
	pop     dx
	pop     bx
	pop     ax
	iret

;-------------------------------------------------------------------------------

_entry:

	retf

	ENTRY  _entry

	END

;-------------------------------------------------------------------------------
