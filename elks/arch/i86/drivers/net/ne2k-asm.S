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
// apr-2022 (HS) : Support auto detection of 8bit mode, set 8k buffer size when in 8bit mode.
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
// the wrap-around point: BOUNDARY may be 80 (60 if 8bit), while the next block to read is 46.
// For simplicity (to avoid the ring wraparound logic on every read)
// the driver uses a variable to hold the next block to read - ne2k_next_pk.
// For debugging, this variable is also available to the C part of the driver.
//
//-----------------------------------------------------------------------------

//#include <linuxmt/config.h>
//#include "arch/ports.h"
#include <arch/asm-offsets.h>
#include <linuxmt/netstat.h>

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
rx_last_8	   = 0x60	// For 8k buffer in 8 bit mode - per spec. 

// Flags

BUF_4K	= ETHF_4K_BUF
BUF_8K	= ETHF_8K_BUF
BUF_16K	= ETHF_16K_BUF
BUF_FLAGS = (ETHF_4K_BUF|ETHF_8K_BUF|ETHF_16K_BUF)
ISA_8B	= ETHF_8BIT_BUS
ISA_16B	= ETHF_16BIT_BUS
USE_AUI = ETHF_USE_AUI

//-----------------------------------------------------------------------------
	.data
	.extern current
	.extern	net_port	// io-port base

	.global ne2k_next_pk
ne2k_next_pk:
	.word 0			// being used as byte ...

	.global ne2k_has_data
ne2k_has_data:
	.word 0 

	.global ne2k_flags	// flags 
ne2k_flags:
	.word 0

ne2k_rx_last:			// PSTOP - buffer upper bound
	.byte rx_last_16	// default to 16K

	.text

//-----------------------------------------------------------------------------
// Set unicast address (aka MAC address)
//-----------------------------------------------------------------------------
// arg1 : pointer to unicast address (6 bytes)

	.global ne2k_addr_set

ne2k_addr_set:

	push    %bp
	mov     %sp,%bp
	push    %si 

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
// Write data block to NIC with internal DMA
//-----------------------------------------------------------------------------
//
// BX    : NIC memory address (to write to)
// CX    : byte count
// DS:SI : host memory address (to read from)
//-------------------------------------

dma_write:

	push    %cx
	push	%bx	// TODO check if this is required (2)
	push	%ds

	inc     %cx     // make byte count even
	and     $0xfffe,%cx
	cli		// Mandatory
	call    dma_init

	// start DMA write

	mov	net_port,%dx	// command register
	mov	$0x12,%al	
	out     %al,%dx

	// I/O write loop

	//mov	net_port,%dx
	add	$io_ne2k_data_io,%dx
	mov	ne2k_flags,%ax		// Get this before changing the data segment
	mov	current,%bx		// setup for far memory xfer
	mov	TASK_USER_DS(%bx),%ds
	cld
	test	$ISA_8B,%ax		// checking ne2k_flags
	jz	wr_loop_w

	// Byte loop
wr_loop_b:
	lodsb
	outb	%al,%dx
	loop	wr_loop_b

	jmp	wr_loop_done

	// word loop
wr_loop_w:
	shr	%cx
3:	lodsw
	out     %ax,%dx
	loop	3b

wr_loop_done:
	
	pop	%ds	// get the data segment back
			// otherwise the net_port reference below won't work
	// wait for DMA completed
check_dma_w:
	mov	net_port,%dx
	add	$io_ne2k_int_stat,%dx
	in      %dx,%al
	and	$0x40,%al	// make sure we're done
	jz	check_dma_w

	//mov	$0x40,%al       // clear DMA intr bit in ISR
	out	%al,%dx

	sti			// Mandatory
	pop	%bx
	pop     %cx
	ret

//-----------------------------------------------------------------------------
// Read data block from chip with internal DMA
//-----------------------------------------------------------------------------
//
// BX    : NIC memory to read from
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

	pushfw		// save interrupt state
	cli		// Experimental - disable INTR
	call    dma_init

	mov     %ds,%bx
	mov     %bx,%es
	pop	%ax
	cmp	$0,%al		// Use local buffer if zero
	jz	buf_local
	mov	current,%bx	// Normal: read directly into the (far) buffer
	mov	TASK_USER_DS(%bx),%es

buf_local:
	mov	net_port,%dx	// command register
	mov	$0x0a,%al	// set RD0 & STA
	out     %al,%dx		// start DMA read

	//mov	net_port,%dx	// Already in DX
	add	$io_ne2k_data_io,%dx
	cld			// clear direction flag
	testw	$ISA_8B,ne2k_flags
	jz	word_loop0

byte_loop:
	inb      %dx,%al
	stosb
	loop    byte_loop
	jmp	3f

	// Word transfer
word_loop0:
	shr     %cx		// half -> word size transf
word_loop:
	in      %dx,%ax
	stosw
	loop	word_loop
3:
	mov	net_port,%dx
	add	$io_ne2k_int_stat,%dx
check_dma_r:
	in      %dx,%al
	test    $0x40,%al       // dma done?
	jz      check_dma_r
	

	mov     $0x40,%al       // clear ISR (RDC bit only)
	out     %al,%dx
	popfw			//Experimental - reenable interrupt state

	pop	%bx
	pop	%es
	pop     %di

	ret

//
//-----------------------------------------------------------------------
// ne2k_getpage -- return ring buffer page numbers in AX:
// AH = CURRENT - where the next received packet will be stored,
// AL = BOUNDARY - where the next read from the buffer will start
//-----------------------------------------------------------------------
// NOTE: BOUNDARY is always one behind where the next read will start, the real 
// 	read point is in ne2k_next_pk. This trick is necessary
//	because the internal logic in the NIC will trigger an overrun interrupt
//	if the BOUNDARY pointer matches or exceeds the CURRENT pointer.
//---------------
// Used internally, exposed externally for debugging purposes.
//
	.global ne2k_getpage

ne2k_getpage:
	mov	$0x42,%al		// page 1
	mov	net_port,%dx		// command register
	out	%al,%dx

	//mov	net_port,%dx		// already loaded
	add	$io_ne2k_rx_put,%dx	// CURRENT
	in      %dx,%al
	mov     %al,%ah

	mov	$0x02,%al		// page 0
	mov	net_port,%dx		// command register
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

	//mov	net_port,%dx
	add	$io_ne2k_rx_put,%dx
	in      %dx,%al
	mov     %al,%ah

	mov	$0x02,%al	// back to page 0
	mov	net_port,%dx	// command register
	out	%al,%dx

	// get RX get pointer

	mov	ne2k_next_pk,%al 
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

	movw	ne2k_has_data,%ax
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
	mov	ne2k_next_pk,%bh
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
	// Removed when read was changed to put data directly into process address space,
	// there is no longer anywhere to put the first 4 bytes.

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
	mov	%al,ne2k_next_pk  // save 'real' next ptr
	mov	%al,%bl		// save for later
	dec	%al
	cmp	$rx_first,%al
	jnb	npg_next	// if the decrement sent us outside the ring..
	mov	ne2k_rx_last,%al
	dec	%al

npg_next:

	mov	net_port,%dx
	add	$io_ne2k_rx_get,%dx	// update RX_get (BOUNDARY)
	out     %al,%dx

npg_exit:
	// This is effectively the replacement for the rx_stat routine,
	// clear the has_data flag if ring buffer is empty.
	cli			// Ensure we don't get a race condition when
				// updating ne2k_has_data
	call	ne2k_getpage
	cmp	%ah,%bl		// ring buffer empty?
	jnz	npg_exit_ok
	movw	$0,ne2k_has_data

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
	test	$0x04,%al
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
	add	$io_ne2k_tx_len1,%dx
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
				// If we skip the completion test below
				// (let the transmit run on its own for 
				// efficiency) this test makes sense. OTOH
				// we're doing the same test in write and select
				// so objectively it's superfluous.
	jnz	tx_rdy_wait
	//and	$0x18,%al	// Don't do this, it will set RD2 and
	//or	$6,%al		// cause an extra RDC abort interrupt
	mov	$6,%al		// set TX + STA
	out	%al,%dx		// start transfer
#if 0
	// EXPERIMENTAL - do we want to wait for completion here?
	// If we wait, we may as well check error status
	// and repeat the send if it failed (while we have the data at hand).
1:	mov	net_port,%dx	// command register
	in      %dx,%al
	test    $4,%al		// Wait for completion
	jnz	1b
#endif
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

	//mov	net_port,%dx
	add	$io_ne2k_data_conf,%dx
	mov     $0x49,%al	// set word access
	testw	$ISA_8B,ne2k_flags
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
// Uses AX, DX only
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
	mov     $2,%al		// 2 for loopback
	out     %al,%dx

	// set RX ring limits - 16KB on-chip memory
	// less one TX frame at the beginning (6 x 256B).
	// The defaults are 16k if 16 bit NIC, 8k if 8bit NIC.
	// Flags may force other sizes (up to 32k), no 
	//  sanity checking is done. See flags in netstat.h

	mov	net_port,%dx
	add	$io_ne2k_rx_first,%dx
	mov     $rx_first,%al	// start of ring, usually 0x46
	out     %al,%dx

	// set ending page for the ring buffer,
	// check for forced buffer size
	movw	ne2k_flags,%ax
	and	$BUF_FLAGS,%al	// Check for forced buffer size
	jz	2f
	movb	$0x10,%ah	// 4k minimum
	push	%cx		// FIXME: Is this required??
	and	$3,%al		// remove the BUF_4K bit (0x04) from flags
				// keep (BUF_8K|BUF_16K)
	mov	%al,%cl
	shl	%cl,%ah		// %AL is the block count to add to 
				// the buffer start address (usually 0x40),
				// the sum becoming the PSTOP value
	add	$tx_first,%ah	// NOTE: tx_first is where the buffer starts,
				// we don't want rx_first here.
	pop	%cx
	mov	%ah,%al
	jmp	1f

	// The normal case, use defaults
2:	movb	$rx_last_16,%al
	testw	$ISA_8B,ne2k_flags
	jz	1f
	movb	$rx_last_8,%al	// It's an 8 bit NIC, use 8K buffer
1:	movb	%al,ne2k_rx_last

	mov	net_port,%dx
	add	$io_ne2k_rx_last,%dx
	out     %al,%dx

	call	ne2k_rx_init	// initialize receive buffer

	// initialize start of TX buffer
	mov	net_port,%dx
	add	$io_ne2k_tx_start,%dx
	mov     $tx_first,%al
	out     %al,%dx

	// FIXME _ wait till open (start) before enabling intr
	// set interrupt mask
	mov	net_port,%dx
	add	$io_ne2k_int_mask,%dx

	// Create and set the effective interrupt mask.
	//
	// NOTE: Don't enable RXE intr if running QEMU.
	// Apparently there is a bug in QEMU which cause continuous interrupts
	// (and status reg = 00) if RXE is enabled.
	// OTOH RXE should be enabled if running an 8bit interface.
	mov     $0x1f,%al	// 0x53 = RDC, Overflow, RX, TX 
				// 0x13 = Overflow, RX, TX
				// 0x1F = Overflow, TXE, RXE, RX, TX
	testw	$ISA_8B,ne2k_flags
	jnz	1f
	and	$0xfb,%al	// RXE intr is useful (mostly) if 8bit interface,
				// clear if 16bit.
				// That way we get around the QEMU RXE bug too.
1:	out     %al,%dx

	// NOTE: The transmitter is not yet enabled, done in the _start routine.
	// FIXME: Should move the int mask and status reg clearing to 
	// the _start routine too to avoid interrupts from a closed device.

	ret

//------------------------------------------------------------------------
// rx_init
// reset the ring buffer front and end pointers to initial values
//------------------------------------------------------------------------

	.global ne2k_rx_init

ne2k_rx_init:

	// set RX_get pointer [BOUNDARY] 

	mov     $rx_first,%al
	mov	%al,%ah		// save copy
	mov	net_port,%dx
	add	$io_ne2k_rx_get,%dx
	out     %al,%dx		// Ring buffer starting point

	mov	$0x40,%al	// Switch to register page 1
	mov	net_port,%dx	// command register
	out	%al,%dx

	// set RX_put pointer  [CURRENT] = RX_get [BOUNDARY]

	//mov	net_port,%dx
	add	$io_ne2k_rx_put,%dx
	mov	%ah,%al		// restore $rx_first value
	inc	%al		// Keep CURRENT one ahead of BOUNDARY
	out     %al,%dx
	mov	%al,ne2k_next_pk 	// Initialize our local copy of
					// BOUNDARY + 1

	xor	%ax,%ax	// Switch back to page 0, don't touch the other bits
	mov	net_port,%dx	// command register
	out	%al,%dx
	movw	%ax,ne2k_has_data	// ZERO - Insurance, no data available
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

	//mov	net_port,%dx
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

#if 0   /* ne2k_probe in ne2k.c */
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
#endif

//-----------------------------------------------------------------------------
// NE2K reset
//-----------------------------------------------------------------------------

	.global ne2k_reset

ne2k_reset:

	// reset device with pulse on reset port

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
	push    %di

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
//	The defaults are set in the _intr routine in the .c part of the driver,
//	typically 3 if the buffer is 16k, 1 or 0 if lower.
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

	// We have not cleared the OFLW INT bit yet, so NIC 
	// interrupts are not enabled 

	mov	net_port,%dx	// command register
#if 0
	// Should not be required - the STOP command will wait out
	// whatever is in progess
1:	in	%dx,%al
	test	$0x4,%al	// wait if transmit in progress
	jnz	1b
#endif
	mov	$0x21,%al	// page 0 + Abort DMA; STOP
	out     %al,%dx

	// clear dma counters, required for the STOP command to complete
	// (and set the RST bit)
	add	$io_ne2k_dma_len1,%dx  // io_ne2k_dma_len1
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
	add	$io_ne2k_tx_conf,%dx	// set tx to loopback
	mov	$2,%al
	out	%al,%dx

	mov	net_port,%dx	// Command register
	mov	$0x22,%al	// Restart NIC
	out	%al,%dx
	
	// NIC is running but offline, start deleting

of_drop_packets:

	// initial housekeeping
	mov	ne2k_next_pk,%ah	// The 'real' BOUNDARY ptr
	and	$0x7,%bx		// limit the drop count and check for ZERO
	jnz	of_drop_loop1

	// Drop count is ZERO, purge the buffer
	call	ne2k_rx_init
	// EXPERIMENTAL - reset the NIC completely
	// Need more testing to see if this is useful
	//call	ne2k_reset		// THE HARD WAY
	//call	ne2k_init
	//call	ne2k_start		// enable transmitter
	jmp	of_drop_ok		// ... and exit

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
	dec	%bx
	jnz	of_drop_loop1	// BX = 1-7

of_drop_1:
	// Move the front of the ring (CURRENT) to the block # in AH
	//   effectively deleting the rest of the ring.
	mov	net_port,%dx	// Command register
	mov	$0x42,%al	// set page 1
	out	%al,%dx

	//mov	net_port,%dx
	add	$io_ne2k_rx_put,%dx
	mov	%ah,%al		// set CURRENT to the beginning of the next pkt,
	out	%al,%dx		// effectively clearing the rest of the ring.
	mov	net_port,%dx	// Command register
	mov	$02,%al		// Back to page 0
	out	%al,%dx

#if NOT_SO_SMART
	jmp	of_drop_ok

of_drop_2:
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
	mov	%al,ne2k_next_pk
	// do the wrap-around exercise
	dec	%al
	cmp	$rx_first,%al
	jnb	1f
	mov	ne2k_rx_last,%al
	dec	%al
1:
	out     %al,%dx
	mov	%ch,%ah		// return value
	jmp	of_drop_ok
#endif

of_drop_ok:
	// insurance: check if the ring buffer is empty.
	// we may have removed all packets in the buffer (if 4k buffer)
	call	ne2k_getpage
	push	%ax			// save for return
	cmp	ne2k_next_pk,%ah	// next_pk == CURRENT?
	jz	of_exit0
	movw	$1,ne2k_has_data
	jmp	of_exit1
of_exit0:
	movw	$0,ne2k_has_data

of_exit1:
	mov	net_port,%dx		// set tx back to normal
	add	$io_ne2k_tx_conf,%dx
	xor	%al,%al
	out	%al,%dx

	call	ne2k_clr_int_reg	// clear all interrupt bits

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

	mov	net_port,%dx
	add     $io_ne2k_int_stat,%dx   // reset the interrupt bit
	mov     $0x40,%al
	out     %al,%dx

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
#if LATER
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

ne2k_clr_int_reg:
	mov	net_port,%dx
	add	$io_ne2k_int_stat,%dx
	//in	%dx,%al
	mov	$0x7f,%al
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

//-----------------------------------------------------------------------------
