//-----------------------------------------------------------------------------
// NE2K driver - low part - MAC routines
//
// Updated by Helge Skrivervik (HS) July 2020:
//	. pick up MAC address from prom
//	. fixed read ring buffer wrap around errors
//	. added ring buffer overflow handling
//	. use word I/O
// oct-2021: Pick up I/O port # from init/bootopts (HS)
// Updated by Santiago Hormazabal on Dec 2021:
//  . 8 bit access if CONFIG_ETH_BYTE_ACCESS is set, using a ne1k patch from
//    NCommander.
// apr-2022 (HS) : Support auto detection of 8bit mode, set 4k buffer size when in 8bit mode
//	     assuming RTL8019 buffer restrictions.
//	     Rewrote overflow handler, added direct access to many registers from C code
//	     Cleaned up initialization code, optimized 8bit I/O
//
//-----------------------------------------------------------------------------
// Terminology
// The ring buffer pointer terminology used in the DP8390C/NS3249C document is confusing.
// 'CURRENT' points to the next block to be filled by an incoming packet
// 'BOUNDARY' points to the next block to be read from the NIC
// The two must not be equal. If the ring buffer is empty, BOUNDARY = CURRENT -1
// So the next block to read is not BOUNDARY but BOUNDARY + 1. Easy - except at
// the wrap-around point: BOUNDARY may be 80 or 50, while the next block to read is 46.
// For simplicity (to avoid the ring wraparound logic on every read)
// the driver uses a variable to hold the next block to read - _ne2k_next_pk.
// For debugging, this variable is also available to the C part of the driver.
//
//-----------------------------------------------------------------------------

#include <linuxmt/config.h>
#include "arch/ports.h"
#include <arch/asm-offsets.h>

	.code16

// register array - offset from base I/O address

io_ne2k_command    = 0x00  // command register at base address
io_ne2k_rx_first   = 0x01  // page 0
io_ne2k_rx_last    = 0x02  // page 0
io_ne2k_rx_get     = 0x03  // page 0

io_ne2k_tx_start   = 0x04  // page 0 - write
io_ne2k_tx_len1    = 0x05  // page 0 - write
io_ne2k_tx_len2    = 0x06  // page 0 - write

io_ne2k_int_stat   = 0x07  // page 0

io_ne2k_dma_addr1  = 0x08  // page 0
io_ne2k_dma_addr2  = 0x09  // page 0
io_ne2k_dma_len1   = 0x0A  // page 0 - write
io_ne2k_dma_len2   = 0x0B  // page 0 - write

io_ne2k_rx_stat    = 0x0C  // page 0 - read
io_ne2k_tx_stat    = 0x04  // page 0 - read

io_ne2k_rx_conf    = 0x0C  // page 0 - write
io_ne2k_tx_conf    = 0x0D  // page 0 - write
io_ne2k_data_conf  = 0x0E  // page 0 - write
io_ne2k_int_mask   = 0x0F  // page 0 - write

io_ne2k_frame_errs = 0x0D	// page 0 read - Frame Alignment Error counter
io_ne2k_crc_errs   = 0x0E	// page 0 read - CRC error counter
io_ne2k_lost_pkts  = 0x0F	// page 0 read - Lost packet counter

io_ne2k_unicast    = 0x01  // page 1 - 6 bytes
io_ne2k_rx_put     = 0x07  // page 1
io_ne2k_multicast  = 0x08  // page 1 - 8 bytes

io_ne2k_data_io    = 0x10  // 2 bytes

io_ne2k_reset      = 0x1F	// Really a port, not a register, force HW reset of the chip


// Ring segmentation

tx_first           = 0x40
rx_first           = 0x46
rx_last_16	   = 0x80
rx_last_8	   = 0x50	// Use only 4k in 8 bit mode 

//-----------------------------------------------------------------------------
	.data
	.extern current
	.extern	net_port	// io-port base

	.global _ne2k_next_pk
_ne2k_next_pk:
	.word 0	// being used as byte ...

	.global _ne2k_has_data
_ne2k_has_data:
	.word 0 

	.global _ne2k_is_8bit	// set if this is an 8bit card
_ne2k_is_8bit:
	.word 0

_ne2k_rx_last:
	.byte rx_last_16	// default to the 16 bit value


	.text

//-----------------------------------------------------------------------------
// Set unicast address (aka MAC address)
//-----------------------------------------------------------------------------
// arg1 : pointer to unicast address (6 bytes)

	.global ne2k_addr_set

ne2k_addr_set:

	push    %bp
	mov     %sp,%bp
	push    %si  // used by compiler (???)

	mov     4(%bp),%si

	mov	net_port,%dx	// command-register
	mov	$0x42,%al	// page 1
	out	%al,%dx

	// load MAC address

	mov	net_port,%dx
	add	$io_ne2k_unicast,%dx
	mov     $6,%cx
	cld

ems_loop:

	lodsb
	out     %al,%dx
	inc     %dx
	loop    ems_loop

	mov	net_port,%dx	// command register
	mov	$0x02,%al	// back to pg 0
	out	%al,%dx

	pop     %si
	pop     %bp
	ret

//-----------------------------------------------------------------------------
// DMA initialization - Prepare for internal NIC DMA transfer
//-----------------------------------------------------------------------------
// Uses: DX, AX
// BX : chip memory address (4000h...8000h)
// CX : byte count

dma_init:

	// set DMA start address

	mov     net_port,%dx
	add	$io_ne2k_dma_addr1,%dx
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

	ret

//-----------------------------------------------------------------------------
// Write block to chip with internal DMA
//-----------------------------------------------------------------------------
//
// BX    : chip memory address (to write to)
// CX    : byte count
// DS:SI : host memory address (to read from)
//-------------------------------------

dma_write:

	push    %cx
	push	%bx	// TODO check if this is required (2)
	push	%ds
	push    %si

	cli		// Experimental
	inc     %cx     // make byte count even
	and     $0xfffe,%cx
	call    dma_init

	// start DMA write

	mov	net_port,%dx	// command register
	mov	$0x12,%al	
	out     %al,%dx

	// I/O write loop

	//mov	net_port,%dx	// Do this before changing the data segment
	add	$io_ne2k_data_io,%dx
	mov	_ne2k_is_8bit,%ax
	mov	current,%bx		// setup for far memory xfer
	mov	TASK_USER_DS(%bx),%ds
	cld
	test	%ax,%ax		// checking _ne2k_is_8bit
	jz	1f

	// Byte loop
4:	lodsb
	outb     %al,%dx
	loop    4b
	jmp	2f

	// word loop
1:	shr	%cx
3:	lodsw
	out     %ax,%dx
	loop	3b
2:
	// wait for DMA completed

	mov	net_port,%dx
	add	$io_ne2k_int_stat,%dx

check_dma_w:
	in      %dx,%al
	test    $0x40,%al       // dma done?
	jnz	end_dma_w

end_dma_w:
	mov     $0x40,%al       // clear DMA intr bit in ISR
	out     %al,%dx
	sti			// Experimental


	mov	%cl,%al		// Error (debug) return
	xor	%ah,%ah		// if AL is 0xff, we had a timeout, the dma never completed
	pop     %si
	pop	%ds
	pop	%bx
	pop     %cx
	ret

#if 0
//-------------------------------------------------------------------------
// This is an (untested) skeleton routine for DMA-assisted packet transfer
// from the NIC to host memory.
// TODO: Add DMA channel setup and teardown. 
//
dma_r:	// Use the send data command to read exactly one backet, 
	// the nic does everything on its own, needs only ES:DI
	
	push	%ax
	push	%di
	push	%dx
	push    %es  // compiler scratch

	mov     %ds,%ax
	mov     %ax,%es	// only required if we're setting up the dma locally

	mov	net_port,%dx
	add	$io_ne2k_tx_len2,%dx
	mov	$0x0f,%al	// prep for using the 'send packet' cmd
	out	%al,%dx

	mov	net_port,%dx	// command register
	mov	$0x18,%al	// send packet
	out	%al,%dx
	// now the dma does the rest, and an RDC interrupt is fielded when complete
	// we can loop here while waiting, or return and handle completion separately.
	in	%dx,%al
	test	$0x40,%al
	jz	rlp
rlp1:	
	mov	net_port,%dx
	add	$io_ne2k_int_stat,%dx
	mov     $0x40,%al       // reset (only this bit in) ISR
	out     %al,%dx         // Clear RDC

rlp_ret:	
	pop     %es
	pop	%dx
	pop	%di
	pop	%ax

	ret
#endif
	
//-----------------------------------------------------------------------------
// Read block from chip with internal DMA
//-----------------------------------------------------------------------------
//
// BX    : chip memory to read from
// CX    : byte count
// ES:DI : host memory to write to
// AL:	 : 0: buffer is local (kernel), <>0: buffer is far (process)

dma_read:

	push    %di
	push    %es
	push	%bx
	push	%ax

	inc     %cx     // make byte count even
	and     $0xfffe,%cx

	cli		// Experimental - disable INTR
	call    dma_init

	mov     %ds,%bx
	mov     %bx,%es
	pop	%ax
	cmp	$0,%al		// Use local buffer if zero
	jz	buf_local
	mov	current,%bx	// Normal: read directly into the (far) buffer
	mov	TASK_USER_DS(%bx),%es

	pop	%bx
	push	%bx

buf_local:
	mov	net_port,%dx	// command register
	mov	$0x0a,%al	// set RD0 & STA
	out     %al,%dx		// start DMA read

	mov	net_port,%dx
	add	$io_ne2k_data_io,%dx
	cld			// clear direction flag
	cmpw	$0,_ne2k_is_8bit
	jz	1f

// Byte transfer
byte_loop:
	inb      %dx,%al
	stosb
	loop    byte_loop
	jmp	3f

// Word transfer
1:
	shr     %cx     // half -> word size transf
word_loop:
	in      %dx,%ax
	stosw
	loop	word_loop
3:
	// wait for DMA to complete

	mov	net_port,%dx
	add	$io_ne2k_int_stat,%dx
check_dma_r:
	in      %dx,%al
	test    $0x40,%al       // dma done?
	jz      check_dma_r     // loop if not

	mov     $0x40,%al       // reset ISR (RDC bit only)
	out     %al,%dx
	sti		//Experimental - Enable INTR

	pop	%bx
	pop	%es
	pop     %di

	ret

//
//-----------------------------------------------------------------------
// ne2k_getpage -- return current ring buffer page numbers in AX:
// AH = CURRENT - where the next received packet will be stored,
// AL = BOUNDARY - where the next read from the buffer will start
//-----------------------------------------------------------------------
// NOTE: BOUNDARY is always one behind where the next read will start, the real 
// 	read point is in the variable _NE2K_NEXT_PK. This trick is necessary
//	because the internal logic in the NIC will trigger an overrun interrupt
//	if the BOUNDARY pointer matches or exceeds the CURRENT pointer.
//---------------
// Used internally, exposed externally for debugging purposes.
//
	.global ne2k_getpage

ne2k_getpage:
	mov	$0x42,%al	// page 1
	mov	net_port,%dx	// command register
	out	%al,%dx

	mov	net_port,%dx
	add	$io_ne2k_rx_put,%dx	// CURRENT
	in      %dx,%al
	mov     %al,%ah

	mov	$0x02,%al	// page 0
	mov	net_port,%dx	// command register
	out	%al,%dx

	//mov	net_port,%dx
	add	$io_ne2k_rx_get,%dx     // BOUNDARY
	in      %dx,%al

	ret


//-----------------------------------------------------------------------------
// Get RX status
//-----------------------------------------------------------------------------
// Returns:
// AX: status
//   01h = Data available in NIC ring buffer

	.global ne2k_rx_stat

ne2k_rx_stat:

	// get RX put pointer
#if 0
	mov	$0x42,%al	// page 1
	mov	net_port,%dx	// command register
	out	%al,%dx

	mov	net_port,%dx
	add	$io_ne2k_rx_put,%dx
	in      %dx,%al
	mov     %al,%ah

	mov	$0x02,%al	// back to page 0
	mov	net_port,%dx	// command register
	out	%al,%dx

	// get RX get pointer

	mov	_ne2k_next_pk,%al 
	cmp     %al,%ah		// The ring is empty if they are equal.
	jz      nrs_empty
	cmp	$0,%ax
	jz	nrs_empty
	mov     $1,%ax		// Yes, we have data
	jmp     nrs_exit

nrs_empty:
	xor     %ax,%ax

nrs_exit:

#else
	// sep2020: keep ring buffer status in a variable
	// instead of accessing the NIC registers continuously.
	call	ne2k_clr_int_reg // EXPERIMENTAL, should clear just the read.
	movw	_ne2k_has_data,%ax
#endif
	ret

//-----------------------------------------------------------------------------
// Get received packet
//-----------------------------------------------------------------------------
// arg1: buffer to receive the data
// arg2: int - requested read size (max buffer)
// arg3: int array [2] (return) containing the NIC packet header.
//
// returns:
// AX : < 0 if error, >0 is length read

	.global ne2k_pack_get

ne2k_pack_get:

	push    %bp
	mov     %sp,%bp
	push    %di
	//push	%es

	//sub 	$4,%sp		// temp space
	//mov	%sp,%di

	// get the 4 byte header first -> arg3
	mov	8(%bp),%di	
	mov	_ne2k_next_pk,%bh
	xor	%bl,%bl		// Next pkt to read in BX

	mov	$4,%cx		// Bytes to read
	//mov     %ds,%ax
	//mov     %ax,%es		// local address space
	xor	%al,%al		// indicate local address space
	call	dma_read

	mov	0(%di),%ax	// AH : next record, AL : status
	mov	2(%di),%cx	// packet size (without CRC)

	// get the actual data

	//add	$4,%sp
	mov	4(%bp),%di	// Buffer address to receive data.
	mov	6(%bp),%dx	// read len
	cmp	%cx,%dx		// choose the shorter
	jnb	npg_cont0
	mov	%dx,%cx		

#if 0
	// -------------------------------------------------------------
	// Packet size check not required since the NIC will not
	// accept such packets per our initialization. Note, in order to handle
	// erroneous packets, rx_get (BOUNDARY) pointer must be updated
	// to point to the next packet.
	// If oversized packets still occur, it's a driver problem (most likely
	// reading the wrong buffer page).
	// -------------------------------------------------------------
	or      %cx,%cx		// zero length
	jz      npg_err2

	cmp     $1528,%cx	// max - head - crc
	jnc     npg_err
#endif
	// -------------------------------------------------------------
	// This section did the smart thing when reading the NIC packet header:
	// Got the entire block (256b) instead of the 4 first bytes. Which was great 
	// for small packets (telnet, command packets etc.): One read instead of 2.
	//
	// Removed when changing to read directly into the process was introduced,
	// there is no longer anywhere to put the first 4 bytes.
	// It may be an idea to reintroduce a 256b buffer to take advantage 
	// of this 'optimization' and at the same time help strangled 8bit 
	// interfaces.

	//sub	$252,%cx	// Got entire packet?
	//jle	npg_cont
				// If not, get rest.
	//inc	%bh		// Point to next page
	//cmp	$rx_last,%bh	// check wraparound
	//jnz	npg_cont0
	//mov	$rx_first,%bh
npg_cont0:
	//add	$256,%di	// Update destination memory address 
				// (keep the 4 byte NIC header)
	push	%cx		// save length
	push	%ax
	add	$4,%bx		// Skip the 4 bytes already read

	mov	$1,%al		// use far transfer
	call    dma_read
	pop     %ax

	// update RX_get pointer (BOUNDARY, end of ring)

npg_cont:
	xchg    %al,%ah		// get pointer to %al
	mov	%al,_ne2k_next_pk  // save 'real' next ptr
	mov	%al,%bl		// save for later
	dec	%al
	cmp	$rx_first,%al
	jnb	npg_next	// if the decrement sent us outside the ring..
	mov	_ne2k_rx_last,%al
	dec	%al

npg_next:

	mov	net_port,%dx
	add	$io_ne2k_rx_get,%dx	// update RX_get (BOUNDARY)
	out     %al,%dx

npg_exit:
	// This is effectively the replacement for the rx_stat routine,
	// clear the has_data flag if ring buffer is empty.
	cli			// Ensure we don't get a rece condition when
				// updating _ne2k_has_data
	call	ne2k_getpage
	cmp	%ah,%bl		// ring buffer empty?
	jnz	npg_exit_ok
	movw	$0,_ne2k_has_data

npg_exit_ok:
	sti		// Enable interrupts
	pop	%ax	// return byte count (from %cx)
	//pop	%es
	pop     %di
	//mov	%bp,%sp	// restore stack pointer
	pop     %bp
	ret

//-----------------------------------------------------------------------------
// Get TX status: Read the TXP bit in the command reg, if reset, there is no 
// transmit in progress.
//-----------------------------------------------------------------------------

// returns:
// AX:
//   02h = ready to send

	.global ne2k_tx_stat

ne2k_tx_stat:

	mov	net_port,%dx	// command register
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
// Send packet: First transfer packet data to NIC memory, then kick off
// the actual transmit and return.
//-----------------------------------------------------------------------------
// arg1 : packet buffer to transfer
// arg2 : size in bytes
// returns:
//	AX : error code

	.global ne2k_pack_put

ne2k_pack_put:

	push    %bp
	mov     %sp,%bp
	push    %si

	// write packet to chip memory

	mov     6(%bp),%cx	// arg2 - count
	xor     %bl,%bl
	mov     $tx_first,%bh
	mov     4(%bp),%si	// arg1 - buffer
	call    dma_write	// copy the data

	// set TX pointer and length

	mov	net_port,%dx
	add	$io_ne2k_tx_start,%dx	// FIXME: This is probably superfluous, done
					// at initialization time, never changes.
	//mov     $tx_first,%al
	//out     %al,%dx

	//add	$io_ne2k_tx_len1,%dx	// use inc instead
	inc     %dx		// io_ne2k_tx_len1
	mov     %cl,%al
	out     %al,%dx
	inc     %dx		// = io_ne2k_tx_len2
	mov     %ch,%al
	out     %al,%dx

	// start TX

tx_rdy_wait:
	mov	net_port,%dx	// command register
	in	%dx,%al
	test	$0x4,%al	// Check that previous transmit completed.
	jnz	tx_rdy_wait
	and	$0x18,%al
	or	$6,%al		// set TX + STA; keep the others
	out	%al,%dx

1:	mov	net_port,%dx	// command register
	in      %dx,%al
	test    $4,%al		// Wait for completion
	jnz	1b

	xor     %ax, %ax	// Always zero return

	pop     %si
	pop     %bp
	ret

//-----------------------------------------------------------------------------
// Get NE2K interrupt status
//-----------------------------------------------------------------------------

// returns interrupt status reg unmodified

// AX : status
//   01h = packet received
//   02h = packet sent
//   10h = RX ring overflow
//   40h = Remote DMA complete

	.global ne2k_int_stat

ne2k_int_stat:

	mov	net_port,%dx
	add	$io_ne2k_int_stat,%dx
	in      %dx,%al
#if 0
	mov	%al,%ah
	and	$3,%al
	jz	1f
	out	%al,%dx
1:	mov	%ah,%al
#endif
	xor	%ah,%ah
	ret

//--------------------------------------------------------------
// Initialization operations common to several internal routines
// Uses: AX, DX
//--------------------------------------------------------------
ne2k_base_init:

	mov	net_port,%dx	// command register
	mov	$0x21,%al	// page 0 + Abort DMA; STOP
	out     %al,%dx

	// Set data size, 16 or 8 bits
	// Some machines are 8 bit only,
	// some interfaces are 8 bits only

	mov	net_port,%dx
	add	$io_ne2k_data_conf,%dx
	mov     $0x49,%al	// set word access
	cmpw	$0,_ne2k_is_8bit
	jz	1f
	dec     %al	// if in 8bit mode, 
			// set byte access, data_conf reg = 0x48
1:	
	out     %al,%dx

	// clear DMA length 

	xor     %al,%al
	mov	net_port,%dx
	add	$io_ne2k_dma_len1,%dx
	out     %al,%dx
	inc     %dx  		// = io_ne2k_dma_len2
	out     %al,%dx

	ret

//-----------------------------------------------------------------------------
// NE2K initialization
// Called from device open
//-----------------------------------------------------------------------------

	.global ne2k_init

ne2k_init:
	call	ne2k_base_init	// basic initialization

	// Accept only packets without errors.
	// Unicast & broadcast, no promiscuous, no multicast

	mov	net_port,%dx
	add	$io_ne2k_rx_conf,%dx
	mov     $0x04,%al
	out     %al,%dx

	// half-duplex and internal loopback
	// to insulate the MAC while stopped.

	mov	net_port,%dx
	add	$io_ne2k_tx_conf,%dx
	mov     $2,%al  // 2 for loopback
	out     %al,%dx

	// set RX ring limits - all 16KB on-chip memory
	// except one TX frame at beginning (6 x 256B)
	// (unless it's an 8 bit card, then only 4KB for send&receive)

	mov	net_port,%dx
	add	$io_ne2k_rx_first,%dx
	mov     $rx_first,%al	// start of ring, usually 0x46
	out     %al,%dx

	// set page at which the ring buffer ends,
	// defaults to the 16 bit value (0x80) 
	movb	$rx_last_16,%al
	testw	$1,_ne2k_is_8bit
	jz	1f
	testw	$2,_ne2k_is_8bit
	jnz	1f	// 16k override for 8 bit interface
	movb	$rx_last_8,%al
1:	movb	%al,_ne2k_rx_last

	mov	net_port,%dx
	add	$io_ne2k_rx_last,%dx
	out     %al,%dx

	call	ne2k_rx_init	// initialize receive buffer
#if 0	
	// this code is moved to ne2k_rx_init - TO BE DELETED
	// set RX_get pointer [BOUNDARY] accordingly

	mov     $rx_first,%al
	mov	%al,%ah		// save copy
	mov	net_port,%dx
	add	$io_ne2k_rx_get,%dx
	out     %al,%dx

	mov	$0x42,%al	// page 1
	mov	net_port,%dx	// command register
	out	%al,%dx

	// set RX_put pointer  [CURRENT] = RX_get [BOUNDARY]

	mov	net_port,%dx
	add	$io_ne2k_rx_put,%dx
	mov	%ah,%al		// restore
	//inc	%al
	out     %al,%dx
	mov	%al,_ne2k_next_pk // at initialization, CURRENT and BOUNDARY are equal

	mov	$0x22,%al	// back to page 0, 
	mov	net_port,%dx	// command register
	out	%al,%dx
#endif
	// initialize start of TX buffer
	mov	net_port,%dx
	add	$io_ne2k_tx_start,%dx
	mov     $tx_first,%al
	out     %al,%dx


	// FIXME _ wait till open (start) before enabling intr
	// set interrupt mask
	mov	net_port,%dx
	add	$io_ne2k_int_mask,%dx
	mov     $0x17,%al	// 0x53 = RDC, Overflow, RX, TX 
				// 0x13 = Overflow, RX, TX
				// 0x17 = Overflow, RXE, RX, TX
	out     %al,%dx

	// NOTE: Transmitter not yet enabled, done in the _start routine
	// FIXME: Should move the int mask and status reg clearing to 
	// the _start routine too to avoid interrupts from a closed device.

	ret

//------------------------------------------------------------------------
// rx_init
// reset the ring buffer front and end pointers to initial values
// Remember to clear _ne2k_has_data !!
//------------------------------------------------------------------------

	.global ne2k_rx_init

ne2k_rx_init:

	// set RX_get pointer [BOUNDARY] 

	mov     $rx_first,%al
	mov	%al,%ah		// save copy
	mov	net_port,%dx
	add	$io_ne2k_rx_get,%dx
	out     %al,%dx

	mov	$0x40,%al	// page 1
	mov	net_port,%dx	// command register
	out	%al,%dx

	// set RX_put pointer  [CURRENT] = RX_get [BOUNDARY]

	//mov	net_port,%dx
	add	$io_ne2k_rx_put,%dx
	mov	%ah,%al		// restore
	inc	%al		// works w/o this one,
				// but this is safer
	out     %al,%dx
	mov	%al,_ne2k_next_pk // at initialization, CURRENT and BOUNDARY are equal

	mov	$0x0,%al	// back to page 0, don't touch the other bits
	mov	net_port,%dx	// command register
	out	%al,%dx
	ret

//-----------------------------------------------------------------------------
// NE2K startup
//-----------------------------------------------------------------------------

	.global ne2k_start

ne2k_start:

	// start the transceiver

	mov	net_port,%dx	// command register
	mov	$0x22,%al	// ensure page 0
	out	%al,%dx

	// move out of internal loopback

	mov	net_port,%dx
	add	$io_ne2k_tx_conf,%dx
	xor	%al,%al
	out	%al,%dx

	// FIXME: Move setting the int mask here (from init)

	ret

//-----------------------------------------------------------------------------
// NE2K stop
//-----------------------------------------------------------------------------

	.global ne2k_stop

ne2k_stop:

	// Stop the DMA and the MAC

	mov	net_port,%dx	// command register
	mov	$0x21,%al	// page 0 + stop
	out     %al,%dx

	// mask all interrrupts

	add	$io_ne2k_int_mask,%dx
	xor     %al,%al
	out     %al,%dx

	// half-duplex and internal loopback
	// to insulate the MAC while stopped
	// and ensure TX finally ends

	mov	net_port,%dx
	add	$io_ne2k_tx_conf,%dx
	mov     $2,%al
	out     %al,%dx

	// clear DMA length

	xor     %al,%al
	mov	net_port,%dx
	add	$io_ne2k_dma_len1,%dx
	out     %al,%dx
	inc     %dx  // = io_ne2k_dma_len2
	out     %al,%dx

	// TODO: wait for the chip to get stable????

	ret

//-----------------------------------------------------------------------------
// NE2K probe
//-----------------------------------------------------------------------------
//
// Access the command register, check that the changes stick

// returns:
// AX: 0=found 1=not found

	.global ne2k_probe

ne2k_probe:

	// Poke then peek at the base address of the interface.
	// If something is there, return 0.
	// No attempt is made to get details about the i/f.

	mov	net_port,%dx	// command register
	mov	$0x20,%al	// set page 0
	out	%al,%dx
	in	%dx,%al
	cmp	$0xff,%al	// cannot be FF
	jz	np_err
	cmp	$0,%al		// cannot be 0
	jz	np_err
	
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

	mov	net_port,%dx
	add	$io_ne2k_reset,%dx
	in      %dx,%al
	out     %al,%dx

	mov	net_port,%dx
	add	$io_ne2k_int_stat,%dx

nr_loop:
	// wait for reset
	// without too much CPU

	hlt

	in      %dx,%al
	test    $0x80,%al	// Wait for RST bit to set
	jz      nr_loop

	// Leave the NIC in a known (stopped) state

	mov	net_port,%dx	// command register
	mov     $0x21,%al
	out     %al,%dx

	ret

//-----------------------------------------------------------------------------
// Get  MAC address from NIC's prom
// WARNING: This function will reset the controller. Use before the init()!
//-----------------------------------------------------------------------------

// arg1 : pointer to 32 bytes buffer

	.global ne2k_get_hw_addr

ne2k_get_hw_addr:

	push    %bp
	mov     %sp,%bp
	push    %di  // used by compiler

	mov     4(%bp),%di

	// Effectively a soft reset of the NIC, required in order to get access to the
	// address PROM. The PROM is 16 bytes, we get 32 back if reading in word mode,
	// the upper byte of each word is garbage. The MAC address is in the first 6 bytes.
	// The remaining 10 bytes sometimes identify the card type. The PROM content from 
	// an 8 bit Weird Electronics (RTL8019AS) card looks like this:
	// 001f1102602d49534138455448204242, the last 10 bytes being 'ISA8ETH BB'.
	// Many 16 bit cards have 0x57 in the last 2 bytes, supposedly indicating
	// 'true ne2k clones'.

w_reset:
	call	ne2k_base_init	// basic initialization

	xor	%al,%al
	mov	net_port,%dx
	add	$io_ne2k_int_mask,%dx
	out	%al,%dx         // mask all interrupts
	call	ne2k_clr_int_reg// required

	mov	net_port,%dx
	add	$io_ne2k_rx_conf,%dx
	mov	$0x20,%al
	out	%al,%dx		// set to monitor mode
	inc	%dx		// $io_ne2k_tx_conf
	mov	$2,%al
	out	%al,%dx         // Loopback mode

	// Now read the PROM
	mov	$32,%cx		// bytes to read
	xor	%bx,%bx		// read from 0:0
	xor	%al,%al		// AL = 0 : local xfer
	call	dma_read

	mov	net_port,%dx
	add	$io_ne2k_tx_conf,%dx	// set tx back to normal
	xor	%al,%al
	out	%al,%dx

	pop	%di
	pop     %bp
	ret

//-----------------------------------------------------------------------------
// NE2K clear overflow --- respond to an input ring buffer overflow interrupt
//-----------------------------------------------------------------------------
//      input: arg1 = buffer recovery strategy
//		0 -> clear input buffer	and reset the NIC
//		1 -> keep the oldest (next-to-read) packet, always safe
//		2 and higher: delete this # of packets from the end (BOUNDARY)
//		towards the head. In 8bit/4k mode, >1 doesn't make much sense.
//	[May want to automatically set reasonable values, such as 1 for 8bit,
//	  4 for 16bit.]
//
//      Returns: AL = BOUNDARY ptr, AH = CURRENT ptr for debugging
//

	.global ne2k_clr_oflow

ne2k_clr_oflow:

	push	%di
	push	%bp
	mov	%sp,%bp

of_cont_1:
	sub	$4,%sp		// get temp space on the stack
	mov	%sp,%di		//   for the dma_read call
	mov     6(%bp),%bx	// arg1, # of packets to kill

	// We have not cleared the OFLW INT bit yet, so NIC interrupts are not enabled 
	//cli
	mov	net_port,%dx	// command register
1:	in	%dx,%al
	test	$0x4,%al	// must wait for transmit to complete
	jnz	1b

	mov	$0x21,%al	// page 0 + Abort DMA; STOP
	out     %al,%dx

	// clear dma counters, required
	add	io_ne2k_dma_len1,%dx  // io_ne2k_dma_len1
	xor	%al,%al
	out     %al,%dx
	inc     %dx		// io_ne2k_dma_len2
	out     %al,%dx

	mov	net_port,%dx
	add	$io_ne2k_int_stat,%dx

of_reset_wait:
	in	%dx,%al		// wait for reset to complete
	test	$0x80,%al
	jz	of_reset_wait

	mov	net_port,%dx
	add	$io_ne2k_tx_conf,%dx	// must set tx to loopback
	mov	$2,%al
	out	%al,%dx

	mov	net_port,%dx	// Command register
	mov	$0x22,%al	// Restart NIC
	out	%al,%dx
	
	// NIC is running but offline, start deleting
1:
	//call	ne2k_getpage    // get BOUNDARY (AL) and CURRENT (AH) pointers

of_drop_packets:

	// initial housekeeping
	mov	_ne2k_next_pk,%ah	// The 'real' BOUNDARY ptr
	and	$0x7,%bx		// limit the packet counter value and check for ZERO
	jnz	of_drop_loop1
	call	ne2k_rx_init	// purge everything
	call	ne2k_getpage	// update return values
	push	%ax
	jmp	of_exit0

of_drop_loop1:
	push	%bx
	xor	%bl,%bl
	mov	%ah,%bh         // Start of next pkt

	// get header of next packet from ring buffer
	mov	$4,%cx		// 4 bytes only
	xor	%al,%al		// use local memory
	call	dma_read	// remember: interrupts are disabled in dma_read!!

	mov	0(%di),%ax	// AH : next record, AL : status

	pop	%bx		// packet counter
	dec	%bx		// packet counter
	jnz	of_drop_loop1	// BX = 1-7

of_drop_1:
	/// Move the front of the ring (CURRENT) to the block # in AH
	///   effectively deleting the rest of the ring.
	mov	net_port,%dx	// Command register
	mov	$0x42,%al	// set page 1
	out	%al,%dx

	//mov	net_port,%dx
	add	$io_ne2k_rx_put,%dx
	mov	%ah,%al		// set CURRENT to the beginning of the next pkt,
	out	%al,%dx		// effectively clearing everything but the 
				// first pkt in the buffer.
	mov	net_port,%dx	// Command register
	mov	$22,%al
	out	%al,%dx		// set page 0
	jmp	of_drop_ok

of_drop_2:
#if 0
	// ALT 2 (BX = 2), delete this packet, keep the rest -
	// by moving the BOUNDARY pointer to the beginning of the next packet
	// May not be safe for 8bit interfaces, the 'next' (last) packet
	// may be garbage. Experimental	- KEPT FOR REFERENCE
	// Verdict: Does not work well for any setting.

	//mov	%cl,%al		// save BOUNDARY for return
	//push	%ax
	mov	net_port,%dx
	add	$io_ne2k_rx_get,%dx
	mov	%ah,%al		// set BOUNDARY to the beginning of the next pkt,
				// don't touch CURRENT
	mov	%al,_ne2k_next_pk
	// do the wrap-around excercise
	dec	%al
	cmp	$rx_first,%al
	jnb	1f
	mov	_ne2k_rx_last,%al
	dec	%al
1:
	out     %al,%dx
	mov	%ch,%ah		// return value
	jmp	of_drop_ok
#endif

of_drop_ok:
	// check if the ring buffer is empty
	// may seem moot, but the 8bit interface cannot hold more than 1 full size packet
	// in the buffer, cleaning the only packet will leave the buffer effectively empty
	call	ne2k_getpage		// REMOVE _ JUST TEST
	push	%ax			// save for return
	cmp	_ne2k_next_pk,%ah
	jz	1f
	movw	$1,_ne2k_has_data
	jmp	of_exit0
1:	movw	$0,_ne2k_has_data

of_exit0:
	mov	net_port,%dx		// set tx back to normal
	add	$io_ne2k_tx_conf,%dx
	xor	%al,%al
	out	%al,%dx

	call	ne2k_clr_int_reg	// clear all interrupt bits
	//sti

	pop	%ax	// return value from getpage()
			// (for debugging)
of_exit:
	mov	%bp,%sp
	pop	%bp
	pop	%di
	ret


//-----------------------------------------------------------------------------
// NE2K Remote DMA complete - for now just a placeholder -
// and the right place to reset the intr status bit.
//-----------------------------------------------------------------------------

	.global ne2k_rdc

ne2k_rdc:

	// FIXME enabling read DMA transfers
#if 0
	// don't do this unless we have real dma,
	// it will screw up the transfers between NIC and system.
	mov	net_port,%dx
	add     $io_ne2k_int_stat,%dx   // reset the interrupt bit
	mov     $0x40,%al
	out     %al,%dx

	mov     $1,%ax
#endif
	ret


//-----------------------------------------------------------------------------
// NE2K get error statistics
// returns 3 bytes in the byte_t array[3] pointed to by arg1.
// Max value in each counter is 192. After reading, the regs are reset.
//-----------------------------------------------------------------------------

	.global ne2k_get_errstat

ne2k_get_errstat:

// Currently useful only 4 debugging: Needs a regime to regularly collect 
// and accumulate the numbers in order to be of statistical value.
#if 0
	push	%bp
	mov	%si,%bp
	push	%di

	mov	4(%bp),%di	

	// assume pg 0
	mov	net_port,%dx
	add	$io_ne2k_frame_errs,%dx
	in	%dx,%al
	stosb

	inc	%dx	//	$io_ne2k_crc_errs
	in	%dx,%al
	stosb

	inc	%dx	//	$io_ne2k_lost_pkts
	in	%dx,%al
	stosb
	
	pop	%di
	pop	%bp
#endif
	xor	%ax,%ax
	ret

//---------------------------------------------------------------------------
// Ne2k - get TX error status
// return the content of the TX status register in AX
//---------------------------------------------------------------------------

	.global ne2k_get_tx_stat

ne2k_get_tx_stat:
	mov	net_port,%dx
	add	$io_ne2k_int_stat,%dx
	mov	$0x0a,%al	// Clear PTX & TXE bits in ISR
	out	%al,%dx

	mov	net_port,%dx
	add	$io_ne2k_tx_stat,%dx
	in	%dx,%al
	xor	%ah,%ah
	ret

//---------------------------------------------------------------------------
// Ne2k - get RX error status
// return the content of the RX status register in AX
//---------------------------------------------------------------------------

	.global ne2k_get_rx_stat

ne2k_get_rx_stat:
	// called from interrupt RX RDY processing, must reset the 
	// 
	mov	net_port,%dx
	add	$io_ne2k_int_stat,%dx
	mov	$1,%al		// Clear PRX bit in ISR
	out	%al,%dx

	mov	net_port,%dx
	add	$io_ne2k_rx_stat,%dx
	in	%dx,%al
	xor	%ah,%ah
	ret

//--------------------------------------------------------------------------
// Ne2k - clear interrupt status reg
//--------------------------------------------------------------------------

	.global ne2k_clr_int_reg

ne2k_clr_int_reg:
	mov	net_port,%dx
	add	$io_ne2k_int_stat,%dx
	in	%dx,%al
	out	%al,%dx
	ret

//--------------------------------------------------------------------------
// Ne2k - clear tally counters
//	  Just read the registers to clear them
//--------------------------------------------------------------------------

	.global ne2k_clr_err_cnt

ne2k_clr_err_cnt:
	mov	net_port,%dx
	add	$io_ne2k_int_stat,%dx
	mov	$0x20,%al
	out	%al,%dx

	mov	net_port,%dx
	add	$io_ne2k_frame_errs,%dx
	in	%dx,%al
	inc	%dx	// CRC errors
	in	%dx,%al
	inc	%dx	// Missed packets
	in	%dx,%al
	ret

//-------------------------------------------------------------------------
// Ne2k - clear the RXE bit from the interrupt status reg
//-------------------------------------------------------------------------
	.global ne2k_clr_rxe

ne2k_clr_rxe:
	mov	net_port,%dx
	add	$io_ne2k_int_stat,%dx
	mov	$0x4,%al
	out	%al,%dx
	ret

//-----------------------------------------------------------------------------
