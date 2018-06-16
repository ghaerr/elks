//-----------------------------------------------------------------------------
// NE2K driver - low part - MAC routines
//-----------------------------------------------------------------------------

	.code16

// TODO: move definitions to ne2k-defs.s

// I/O base @ 300h

io_ne2k_command    = 0x0300

io_ne2k_rx_first   = 0x0301  // page 0
io_ne2k_rx_last    = 0x0302  // page 0
io_ne2k_rx_get     = 0x0303  // page 0

// This is not a true NE2K register
//io_ne2k_rx_put1    = 0x0306  // page 0 - read

io_ne2k_tx_start   = 0x0304  // page 0 - write
io_ne2k_tx_len1    = 0x0305  // page 0 - write
io_ne2k_tx_len2    = 0x0306  // page 0 - write

io_ne2k_int_stat   = 0x0307  // page 0

io_ne2k_dma_addr1  = 0x0308  // page 0
io_ne2k_dma_addr2  = 0x0309  // page 0
io_ne2k_dma_len1   = 0x030A  // page 0 - write
io_ne2k_dma_len2   = 0x030B  // page 0 - write

io_ne2k_rx_stat    = 0x030C  // page 0 - read

io_ne2k_rx_conf    = 0x030C  // page 0 - write
io_ne2k_tx_conf    = 0x030D  // page 0 - write
io_ne2k_data_conf  = 0x030E  // page 0 - write
io_ne2k_int_mask   = 0x030F  // page 0 - write

io_ne2k_unicast    = 0x0301  // page 1 - 6 bytes
io_ne2k_rx_put     = 0x0307  // page 1
io_ne2k_multicast  = 0x0308  // page 1 - 8 bytes

io_ne2k_data_io    = 0x0310  // 2 bytes

io_ne2k_reset      = 0x031F


// Ring segmentation

tx_first           = 0x40
rx_first           = 0x46
rx_last            = 0x80

tx_first_word      = 0x4000
rx_first_word      = 0x4600
rx_last_word       = 0x8000

	.text

//-----------------------------------------------------------------------------
// Select register page
//-----------------------------------------------------------------------------

// AL : page number (0 or 1)

page_select:

	mov     %al,%ah
	and     $0x01,%ah
	shl     $6,%ah

	mov     $io_ne2k_command,%dx
	in      %dx,%al
	and     $0x3F,%al
	or      %ah,%al
	out     %al,%dx

	ret

//-----------------------------------------------------------------------------
// Set unicast address (aka MAC address)
//-----------------------------------------------------------------------------

// arg1 : pointer to unicast address (6 bytes)

	.global ne2k_addr_set

ne2k_addr_set:

	push    %bp
	mov     %sp,%bp
	push    %si  // used by compiler

	mov     4(%bp),%si

	// select page 1

	mov     $1,%al
	call    page_select

	// load MAC address

	mov     $io_ne2k_unicast,%dx
	mov     $6,%cx
	cld

ems_loop:

	lodsb
	out     %al,%dx
	inc     %dx
	loop    ems_loop

	pop     %si
	pop     %bp
	ret

//-----------------------------------------------------------------------------
// DMA initialization
//-----------------------------------------------------------------------------

// BX : chip memory address (4000h...8000h)
// CX : byte count

dma_init:

	push    %ax
	push    %dx

	// select page 0

	xor     %al,%al
	call    page_select

	// set DMA start address

	mov     $io_ne2k_dma_addr1,%dx
	mov     %bl,%al
	out     %al,%dx

	inc     %dx  // io_ne2k_dma_addr2
	mov     %bh,%al
	out     %al,%dx

	// set DMA byte count

	inc     %dx  // io_ne2k_dma_len1
	mov     %cl,%al
	out     %al,%dx

	inc     %dx  // io_ne2k_dma_len2
	mov     %ch,%al
	out     %al,%dx

	pop     %dx
	pop     %ax
	ret

//-----------------------------------------------------------------------------
// Write block to chip with internal DMA
//-----------------------------------------------------------------------------

// BX    : chip memory address (to write to)
// CX    : byte count
// DS:SI : host memory address (to read from)

dma_write:

	push    %ax
	push    %cx
	push    %dx
	push    %si

	call    dma_init

	// start DMA write

	mov     $io_ne2k_command,%dx
	in      %dx,%al
	and     $0xC7,%al
	or      $0x10,%al  // 010b : write
	out     %al,%dx

	// I/O write loop

	mov     $io_ne2k_data_io,%dx
	cld

emw_loop:

	lodsb
	out     %al,%dx
	loop    emw_loop

	// maybe check DMA completed ?

	pop     %si
	pop     %dx
	pop     %cx
	pop     %ax
	ret

//-----------------------------------------------------------------------------
// Read block from chip with internal DMA
//-----------------------------------------------------------------------------

// BX    : chip memory to read from
// CX    : byte count
// ES:DI : host memory to write to

dma_read:

	push    %ax
	push    %cx
	push    %dx
	push    %di

	call    dma_init

	// start DMA read

	mov     $io_ne2k_command,%dx
	in      %dx,%al
	and     $0xC7,%al
	or      $0x08,%al  // 001b : read
	out     %al,%dx

	// I/O read loop

	push    %es  // compiler scratch
	mov     %ds,%ax
	mov     %ax,%es

	mov     $io_ne2k_data_io,%dx
	cld

emr_loop:

	in      %dx,%al
	stosb
	loop    emr_loop

	pop     %es

	// maybe check DMA completed ?

	pop     %di
	pop     %dx
	pop     %cx
	pop     %ax
	ret

//-----------------------------------------------------------------------------
// Get RX status
//-----------------------------------------------------------------------------

// returns:

// AX: status
//   01h = packet received

	.global ne2k_rx_stat

ne2k_rx_stat:

	// get RX put pointer

	mov     $1,%al
	call    page_select

	mov     $io_ne2k_rx_put,%dx
	in      %dx,%al
	mov     %al,%cl

	// get RX get pointer

	xor     %al,%al
	call    page_select

	mov     $io_ne2k_rx_get,%dx
	in      %dx,%al
	inc     %al
	cmp     $rx_last,%al
	jnz     nrs_nowrap
	mov     $rx_first,%al

nrs_nowrap:

	// check ring is not empty

	cmp     %al,%cl
	jz      nrs_empty

	mov     $1,%ax
	jmp     nrs_exit

nrs_empty:

	xor     %ax,%ax

nrs_exit:

	ret

//-----------------------------------------------------------------------------
// Get received packet
//-----------------------------------------------------------------------------

// arg1 : packet buffer to write to

// returns:

// AX : error code

	.global ne2k_pack_get

ne2k_pack_get:

	push    %bp
	mov     %sp,%bp
	push    %di  // used by compiler

	// get RX put pointer

	mov     $1,%al
	call    page_select

	mov     $io_ne2k_rx_put,%dx
	in      %dx,%al
	mov     %al,%cl

	// get RX get pointer

	xor     %al,%al
	call    page_select

	mov     $io_ne2k_rx_get,%dx
	in      %dx,%al
	inc     %al
	cmp     $rx_last,%al
	jnz     npg_nowrap1
	mov     $rx_first,%al

npg_nowrap1:

	// check ring is not empty

	cmp     %al,%cl
	jz      npg_err

	xor     %bl,%bl
	mov     %al,%bh

	// get packet header

	mov     4(%bp),%di
	mov     $4,%cx
	call    dma_read

	mov     0(%di),%ax  // AH : next record, AL : status
	mov     2(%di),%cx  // packet size (without CRC)

	// check packet size

	or      %cx,%cx
	jz      npg_err

	cmp     $1528,%cx  // max - head - crc
	jnc     npg_err

	add     $4,%bx
	add     $4,%di

	push    %ax
	push    %cx

	mov     %bx,%ax
	add     %cx,%ax
	cmp     $rx_last_word,%ax
	jbe     npg_nowrap2

	mov     $rx_last_word,%ax
	sub     %bx,%ax
	mov     %ax,%cx

npg_nowrap2:

	// get packet body (first segment)

	call    dma_read

	add     %cx,%di

	mov     %cx,%ax
	pop     %cx
	sub     %ax,%cx
	jz      npg_nowrap3

	// get packet body (second segment)

	mov     $rx_first_word,%bx
	call    dma_read

npg_nowrap3:

	// update RX get pointer

	pop     %ax
	xchg    %al,%ah
	dec     %al
	cmp     $rx_first,%al
	jae     npg_next
	mov     $rx_last - 1,%al

npg_next:

	mov     $io_ne2k_rx_get,%dx
	out     %al,%dx

	xor     %ax,%ax
	jmp     npg_exit

npg_err:

	mov     $-1,%ax

npg_exit:

	pop     %di
	pop     %bp
	ret

//-----------------------------------------------------------------------------
// Get TX status
//-----------------------------------------------------------------------------

// returns:

// AX:
//   02h = ready to send

	.global ne2k_tx_stat

ne2k_tx_stat:

	mov     $io_ne2k_command,%dx
	in      %dx,%al
	and     $0x04,%al
	jz      nts_ready

	xor     %ax,%ax
	jmp     nts_exit

nts_ready:

	mov     $2,%ax

nts_exit:

	ret

//-----------------------------------------------------------------------------
// Put packet to send
//-----------------------------------------------------------------------------

// arg1 : packet buffer to read from
// arg2 : size in bytes

// returns:

// AX : error code

	.global ne2k_pack_put

ne2k_pack_put:

	push    %bp
	mov     %sp,%bp
	push    %si  // used by compiler

	xor     %al,%al
	call    page_select

	// write packet to chip memory

	mov     6(%bp),%cx
	xor     %bl,%bl
	mov     $tx_first,%bh
	mov     4(%bp),%si
	call    dma_write

	// set TX pointer and length

	mov     $io_ne2k_tx_start,%dx
	mov     $tx_first,%al
	out     %al,%dx

	inc     %dx  // io_ne2k_tx_len1
	mov     %cl,%al
	out     %al,%dx
	inc     %dx  // = io_ne2k_tx_len2
	mov     %ch,%al
	out     %al,%dx

	// start TX

	mov     $io_ne2k_command,%dx
	mov     $0x26,%al
	out     %al,%dx

	xor     %ax, %ax

	pop     %si
	pop     %bp
	ret

//-----------------------------------------------------------------------------
// Get NE2K interrupt status
//-----------------------------------------------------------------------------

// returns:

// AX : status
//   01h = packet received
//   02h = packet sent
//   10h = RX ring overflow

	.global ne2k_int_stat

ne2k_int_stat:

	// select page 0

	xor     %al,%al
	call    page_select

	// get interrupt status

	xor     %ah,%ah

	mov     $io_ne2k_int_stat,%dx
	in      %dx,%al
	test    $0x03,%al
	jz      nis_next

	// acknowledge interrupt

	out     %al,%dx

nis_next:

	ret

//-----------------------------------------------------------------------------
// NE2K initialization
//-----------------------------------------------------------------------------

	.global ne2k_init

ne2k_init:

	// select page 0

	xor     %al,%al
	call    page_select

	// Stop DMA and MAC
	// TODO: is this really needed after a reset ?

	mov     $io_ne2k_command,%dx
	in      %dx,%al
	and     $0xC0,%al
	or      $0x21,%al
	out     %al,%dx

	// data I/O in single bytes
	// and low endian for 80188
	// plus magical stuff (48h)

	mov     $io_ne2k_data_conf,%dx
	mov     $0x48,%al
	out     %al,%dx

	// accept packet without error
	// unicast & broadcast & promiscuous

	mov     $io_ne2k_rx_conf,%dx
	mov     $0x54,%al
	out     %al,%dx

	// half-duplex and internal loopback
	// to insulate the MAC while stopped
	// TODO: loopback cannot be turned off later !

	mov     $io_ne2k_tx_conf,%dx
	mov     $0,%al  // 2 for loopback
	out     %al,%dx

	// set RX ring limits
	// all 16KB on-chip memory
	// except one TX frame at beginning (6 x 256B)

	mov     $io_ne2k_rx_first,%dx
	mov     $rx_first,%al
	out     %al,%dx

	inc     %dx  // io_ne2k_rx_last
	mov     $rx_last,%al
	out     %al,%dx

	mov     $io_ne2k_tx_start,%dx
	mov     $tx_first,%al
	out     %al,%dx

	// set RX get pointer

	mov     $io_ne2k_rx_get,%dx
	mov     $rx_first,%al
	out     %al,%dx

	// clear DMA length
	// TODO: is this really needed after a reset ?

	xor     %al,%al
	mov     $io_ne2k_dma_len1,%dx
	out     %al,%dx
	inc     %dx  // = io_ne2k_dma_len2
	out     %al,%dx

	// clear all interrupt flags

	mov     $io_ne2k_int_stat,%dx
	mov     $0x7F,%al
	out     %al,%dx

	// set interrupt mask
	// TX & RX without error and overflow

	mov     $io_ne2k_int_mask,%dx
	mov     $0x03,%al
	out     %al,%dx

	// select page 1

	mov     $1,%al
	call    page_select

	// clear unicast address

	mov     $6,%cx
	mov     io_ne2k_unicast,%dx
	xor     %al,%al

ei_loop_u:

	out     %al,%dx
	inc     %dx
	loop    ei_loop_u

	// clear multicast bitmap

	mov     $8,%cx
	mov     $io_ne2k_multicast,%dx

ei_loop_m:

	out     %al,%dx
	inc     %dx
	loop    ei_loop_m

	// set RX put pointer to first bloc + 1

	mov     $io_ne2k_rx_put,%dx
	mov     $rx_first,%al
	inc     %al
	out     %al,%dx

	// return no error

	xor     %ax,%ax
	ret

//-----------------------------------------------------------------------------
// NE2K startup
//-----------------------------------------------------------------------------

	.global ne2k_start

ne2k_start:

	// start the transceiver

	mov     $io_ne2k_command,%dx
	in      %dx,%al
	and     $0xFC,%al
	or      $0x02,%al
	out     %al,%dx

	// move out of internal loopback
	// TODO: read PHY status to update the duplex mode ?

	mov     $io_ne2k_tx_conf,%dx
	xor     %al,%al
	out     %al,%dx

	xor     %ax,%ax
	ret

//-----------------------------------------------------------------------------
// NE2K stop
//-----------------------------------------------------------------------------

	.global ne2k_stop

ne2k_stop:

	// Stop the DMA and the MAC

	mov     $io_ne2k_command,%dx
	in      %dx,%al
	and     $0xC0,%al
	or      $0x21,%al
	out     %al,%dx

	// select page 0

	xor     %al,%al
	call    page_select

	// half-duplex and internal loopback
	// to insulate the MAC while stopped
	// and ensure TX finally ends

	mov     $io_ne2k_tx_conf,%dx
	mov     $2,%al
	out     %al,%dx

	// clear DMA length

	xor     %al,%al
	mov     $io_ne2k_dma_len1,%dx
	out     %al,%dx
	inc     %dx  // = io_ne2k_dma_len2
	out     %al,%dx

	// TODO: wait for the chip to get stable

	ret

//-----------------------------------------------------------------------------
// NE2K termination
//-----------------------------------------------------------------------------

// call ne2k_stop() before

	.global ne2k_term

ne2k_term:

	// select page 0

	xor     %al,%al
	call    page_select

	// mask all interrrupts

	mov     $io_ne2k_int_mask,%dx
	xor     %al,%al
	out     %al,%dx

	ret

//-----------------------------------------------------------------------------
// NE2K probe
//-----------------------------------------------------------------------------

// Read few registers at supposed I/O addresses
// and check their values for NE2K presence

// returns:

// AX: 0=found 1=not found

	.global ne2k_probe

ne2k_probe:

	// query command register
	// MAC & DMA should be stopped
	// and no TX in progress

	// register not initialized in QEMU
	// so do not rely on this one

	//mov     dx, #io_ne2k_command
	//in      al, dx
	//and     al, #$3F
	//cmp     al, #$21
	//jnz     np_err

	xor     %ax,%ax
	jmp     np_exit

np_err:

	mov     $1,%ax

np_exit:

	ret

//-----------------------------------------------------------------------------
// NE2K reset
//-----------------------------------------------------------------------------

	.global ne2k_reset

ne2k_reset:

	// reset device
	// with pulse on reset port

	mov     $io_ne2k_reset,%dx
	in      %dx,%al
	out     %al,%dx

	// stop all and select page 0

	mov     $io_ne2k_command,%dx
	mov     $0x21,%al
	out     %al,%dx

nr_loop:

	// wait for reset
	// without too much CPU

	hlt

	mov     $io_ne2k_int_stat,%dx
	in      %dx,%al
	test    $0x80,%al
	jz      nr_loop

	ret

//-----------------------------------------------------------------------------
// NE2K test
//-----------------------------------------------------------------------------

	.global ne2k_test

ne2k_test:

	// TODO: test sequence

	mov     $1,%ax
	ret

//-----------------------------------------------------------------------------
