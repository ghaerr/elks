//-----------------------------------------------------------------------------
// NE2K driver - low part - MAC routines
//
// Updated by Santiago Hormazabal on Dec 2021:
//  . 8 bit access if CONFIG_ETH_BYTE_ACCESS is set, using a ne1k patch from
//    NCommander.
// Updated by Helge Skrivervik July 2020:
//	. pick up MAC address from prom
//	. fixed read ring buffer wrap around errors
//	. added ring buffer overflow handling
// oct-2021: Pick up I/O port # from init/bootopts (HS)
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
rx_last            = 0x80

//-----------------------------------------------------------------------------
	.data
	.extern current
	.extern	net_port	// io-port base

_ne2k_next_pk:
	.word 0	// being used as byte ...

	.global _ne2k_skip_cnt
_ne2k_skip_cnt:
	.word 0	// # of packets to skip if buffer overrun, default is all (0)

	.global _ne2k_has_data
_ne2k_has_data:
	.word 0 

	.text

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
	shr     %cx     // half -> word size transf


	// start DMA write

	mov	net_port,%dx	// command register
	mov	$0x12,%al	
	out     %al,%dx

	// I/O write loop

	mov	net_port,%dx	// Do this before changing the data segment
	push	%dx		// Command register
	add	$io_ne2k_data_io,%dx
	mov	current,%bx		// setup for far memory xfer
	mov	TASK_USER_DS(%bx),%ds
	cld

emw_loop:

#ifdef CONFIG_ETH_BYTE_ACCESS
    lodsb
    outb     %al,%dx
    lodsb
    outb     %al,%dx
#else
	lodsw
	out     %ax,%dx
#endif

	loop    emw_loop

	// wait for DMA completed

	pop	%dx	// instead of: mov	net_port,%dx
	add	$io_ne2k_int_stat,%dx
check_dma_w:
	in      %dx,%al
	test    $0x40,%al       // dma done?
	jz      check_dma_w     // loop if not

	mov     $0x40,%al       //clear DMA intr bit in ISR
	out     %al,%dx


	sti		// Experimental
	pop     %si
	pop	%ds
	pop	%bx
	pop     %cx
	ret

#if 0
//-------------------------------------------------------------------------
// This is an (untested) skeleton routine for DMA-assiste paket transfer
// from the NIC to host memory.
// TODO: Add DMA channel setup and teardown. Makes sens to do that outiside of this
// routine.
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
// AL:	 : 0: buffer is local, <>0: buffer is far

dma_read:

	push    %di
	push    %es
	push	%bx
	push	%ax

	cli		// Experimental - disable INTR
	inc     %cx     // make byte count even
	and     $0xfffe,%cx
	call    dma_init
	shr     %cx     // half -> word size transf

	mov     %ds,%bx
	mov     %bx,%es
	pop	%ax
	cmp	$0,%al		// Use local buffer if zero
	jz	buf_local
	mov	current,%bx	// Normal: read directly into the (far) buffer
	mov	TASK_USER_DS(%bx),%es

buf_local:
	pop	%bx
	push	%bx

	// start DMA read

	mov	net_port,%dx	// command register
	mov	$0x0a,%al	// RD0 & STA
	out     %al,%dx

	// I/O read loop

	mov	net_port,%dx
	add	$io_ne2k_data_io,%dx
	cld
emr_loop:
#ifdef CONFIG_ETH_BYTE_ACCESS
    inb      %dx,%al
    stosb
    inb      %dx,%al
    stosb
#else
	in      %dx,%ax
	stosw
#endif
	loop    emr_loop

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
// Used internally, exposed externally for debugging purposes.
//
	.global ne2k_getpage

ne2k_getpage:
	mov	$0x42,%al		// page 1
	mov	net_port,%dx	// command register
	out	%al,%dx

	mov	net_port,%dx
	add	$io_ne2k_rx_put,%dx	// CURRENT
	in      %dx,%al
	mov     %al,%ah

	mov	$0x02,%al		// page 0
	mov	net_port,%dx	// command register
	out	%al,%dx

	mov	net_port,%dx
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
	movw	_ne2k_has_data,%ax	// TEST !!!!!
#endif
	ret

//-----------------------------------------------------------------------------
// Get received packet
//-----------------------------------------------------------------------------
// arg1 : packet buffer to receive the data. The buffer must be 4 bytes to hold 
//	the NIC header plus the mac Ethernet frame size.
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
	mov	8(%bp),%di	// get the hdr into arg3

	// get RX_put pointer

	mov	_ne2k_next_pk,%bh
	xor	%bl,%bl		// Next pkt to read in BX

	mov	$4,%cx
	//mov     %ds,%ax
	//mov     %ax,%es		// local address space
	xor	%al,%al		// local address space
	call	dma_read

	mov	0(%di),%ax	// AH : next record, AL : status
	mov	2(%di),%cx	// packet size (without CRC)

	//add	$4,%sp
	mov	4(%bp),%di	// Buffer address to receive data.
	mov	6(%bp),%dx	// read len
	cmp	%cx,%dx		// choose the shorter
	jnb	npg_cont0
	mov	%dx,%cx		

#if 0
	// -------------------------------------------------------------
	// Check packet size - not required since the NIC will not
	// accept such packets per our initialization. Note, in order to handle
	// erroneous packets, rx_get (BOUNDARY) pointer must be updated.
	// -------------------------------------------------------------
	or      %cx,%cx		// zero length
	jz      npg_err2

	cmp     $1528,%cx	// max - head - crc
	jnc     npg_err
#endif
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
	add	$4,%bx

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
	mov	$rx_last-1,%al	// make it right ...

npg_next:

	mov	net_port,%dx
	add	$io_ne2k_rx_get,%dx	// update RX_get (BOUNDARY)
	out     %al,%dx

#if 0	/* error processing - not used */
	xor     %ax,%ax
	jmp     npg_exit
npg_err:
	mov     $-1,%ax		// Packet too big
	jmp	npg_exit

npg_err2:
	mov	$-2,%ax		// zero length packet
#endif  /* ---------------------------- */

npg_exit:
#if 0	/* Handle ring buffer overflow - not used */
        /* OFLOW handled by its interrupt handler */

	mov	net_port,%dx  
	add	$io_ne2k_int_stat,%dx  
	in	%dx,%al			// get the status bits
	test	$0x10,%al
	jz	npg_no_oflw
	push	%bx
	call	ne2k_clr_oflow
	pop	%bx

npg_no_oflw:
#endif
	// The is effectively the replacement for the rx_stat routine,
	// clear the has_data flag if ring buffer is empty.
	cli	// Interrupts off (experimental)
	//mov	_ne2k_has_data,%ax
	//dec	%ax	// TESTING
	//cmp	$0,%ax
	//jz	npg_zero	// don't set to zero unless sure (TESTING)
	//mov	%ax,_ne2k_has_data
npg_zero:
	call	ne2k_getpage
	cmp	%ah,%bl		// ring buffer empty?
	jnz	npg_more_data
	movw	$0,_ne2k_has_data
npg_more_data:

	sti		// Enable interrupts
	pop	%ax	// return byte count (from %cx)
	//pop	%es
	pop     %di
	mov	%bp,%sp	// restore stack pointer
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

// arg1 : packet buffer to read from
// arg2 : size in bytes

// returns:

// AX : error code

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
	add	$io_ne2k_tx_start,%dx	// FIXME: This may not be required, done
					// at initialization time, never changes.
	mov     $tx_first,%al
	out     %al,%dx

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
	test	$0x4,%al	// Check that previous transmit competed.
	jnz	tx_rdy_wait
	mov	$6,%al		// Set TX bit, starts transfer...
	out	%al,%dx

	//mov	net_port,%dx
	//add	$io_ne2k_int_stat,%dx	// reset tx intr bit
	//mov	$2,%al		// Test, should not make any difference
	//out	%al,%dx

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

// returns:

// AX : status
//   01h = packet received
//   02h = packet sent
//   10h = RX ring overflow
//   40h = Remote DMA complete

	.global ne2k_int_stat

ne2k_int_stat:

	// get interrupt status

	mov	net_port,%dx
	add	$io_ne2k_int_stat,%dx
	in      %dx,%al
	test    $0x13,%al	// ring buffer overflow, tx, rx
	jz      nis_next

	mov	%al,%bl		// save return value
	and	$3,%al		// Reset TX & RX bits here, otherwise traffic 
				// will stop. DO NOT reset the RDC & OFLOW 
				// interrupts, it will disturb dma rd/wr.
	out     %al,%dx
	xor	%ax,%ax
	mov	%bl,%al		// unsave

nis_next:

	ret

//--------------------------------------------------------------
// Initialization operations common to several internal routines
// Uses: AX, DX
//--------------------------------------------------------------
ne2k_base_init:

	mov	net_port,%dx	// command register

	mov	$0x21,%al	// page 0 + Abort DMA; STOP
	out     %al,%dx

	// data I/O in words for PC/AT and higher

	mov	net_port,%dx
	add	$io_ne2k_data_conf,%dx
#ifdef CONFIG_ETH_BYTE_ACCESS
	mov     $0x48,%al	// set byte access
#else
	mov     $0x49,%al	// set word access
#endif
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

	// set RX ring limits
	// all 16KB on-chip memory
	// except one TX frame at beginning (6 x 256B)

	mov	net_port,%dx
	add	$io_ne2k_rx_first,%dx
	mov     $rx_first,%al
	out     %al,%dx

	// set RX_get pointer [BOUNDARY]

	mov	net_port,%dx
	add	$io_ne2k_rx_get,%dx
	out     %al,%dx

	mov	net_port,%dx
	add	$io_ne2k_rx_last,%dx
	mov     $rx_last,%al
	out     %al,%dx

	mov	net_port,%dx
	add	$io_ne2k_tx_start,%dx
	mov     $tx_first,%al
	out     %al,%dx

	// clear all interrupt flags

	mov	net_port,%dx
	add	$io_ne2k_int_stat,%dx
	mov     $0x7F,%al
	out     %al,%dx

	// set interrupt mask
	// TX & RX & OFLW, no error interrupts

	mov	net_port,%dx
	add	$io_ne2k_int_mask,%dx
	mov     $0x13,%al	// 0x53 = RDC, Overflow, RX, TX 
				// 0x13 = Overflow, RX, TX
	out     %al,%dx

	mov	$0x42,%al	// page 1
	mov	net_port,%dx	// command register
	out	%al,%dx

	// set RX put pointer  = RX get

	mov	net_port,%dx
	add	$io_ne2k_rx_put,%dx
	mov     $rx_first,%al
	inc     %al		// CURRENT = always one ahead
	out     %al,%dx
	mov	%al,_ne2k_next_pk

	mov	$0x02,%al	// page 0
	mov	net_port,%dx	// command register
	out	%al,%dx

	// now enable transmitter
	mov	net_port,%dx
	add	$io_ne2k_tx_conf,%dx
	mov     $0,%al		// 2 for loopback
	out     %al,%dx

	ret

//-----------------------------------------------------------------------------
// NE2K startup
//-----------------------------------------------------------------------------

	.global ne2k_start

ne2k_start:

	// start the transceiver

	mov	net_port,%dx	// command register
	mov	$0x02,%al
	out	%al,%dx

	// move out of internal loopback

	mov	net_port,%dx
	add	$io_ne2k_tx_conf,%dx
	xor	%al,%al
	out	%al,%dx

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

	// TODO: wait for the chip to get stable

	ret

//-----------------------------------------------------------------------------
// NE2K termination
//-----------------------------------------------------------------------------

// call ne2k_stop() before

	.global ne2k_term

ne2k_term:

	// mask all interrrupts

	mov	net_port,%dx
	add	$io_ne2k_int_mask,%dx
	xor     %al,%al
	out     %al,%dx

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
	// Address PROM - 32 bytes of which only the first 6 bytes are of interest.
	// NOTE: Since we're reading the entire PROM, we also have the opportunity to
	// detect the type of card. The caller can do this since the entire dataset 
	// is being returned.

w_reset:
	call	ne2k_base_init	// basic initialization

	xor	%al,%al
	mov	net_port,%dx
	add	$io_ne2k_int_mask,%dx
	out	%al,%dx         // mask completion irq
	mov	net_port,%dx
	add	$io_ne2k_int_stat,%dx
	mov	$0x7f,%al
	out	%al,%dx		// clear interrupt status reg, required

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

	mov	net_port,%dx	// set tx back to normal
	add	$io_ne2k_tx_conf,%dx	// set tx back to normal
	xor	%al,%al
	out	%al,%dx

	pop	%di
	pop     %bp
	ret

//-----------------------------------------------------------------------------
// NE2K clear overflow --- respond to an input ring buffer overflow interrupt
// The recovery reads the last compete pcket into the provided (arg1) buffer.
//-----------------------------------------------------------------------------
//      
//      Returns: AL = new BOUNDARY ptr, AH = CURRENT ptr
//

	.global ne2k_clr_oflow

ne2k_clr_oflow:

	push	%di
	push	%bp
	mov	%sp,%bp

	sub	$4,%sp		// get temp space on the stack
	mov	%sp,%di

	mov	net_port,%dx	// command register
of_chk_tx:	// Ensure no transmit is in progress
	in	%dx,%al
	test	4,%al
	//jnz	of_chk_tx

	call	ne2k_base_init	// stop & soft reset

	mov	net_port,%dx
	add	$io_ne2k_int_stat,%dx

of_reset:
	in	%dx,%al		// wait for reset to complete
	test	$0x80,%al
	jz	of_reset

	mov	net_port,%dx
	add	$io_ne2k_tx_conf,%dx	// must set tx to loopback
	mov	$2,%al
	out	%al,%dx
	mov	net_port,%dx	// Command register
	mov	$0x22,%al	// Restart NIC
	out	%al,%dx
	
	// NIC has stopped, now clear out the ring buffer

	call	ne2k_getpage    // get BOUNDARY (AL) and CURRENT (AH) pointers

of_drop_packets:
	// loop through the number of packets given by %bx
	// terminate if we reach the head of the buffer before the # of packets
	// if cnt = 0, just keep the oldest packet in the ring.
	mov	%ax,%cx		// save BOUNDARY & CURRENT
	mov	_ne2k_next_pk,%ah
	mov	_ne2k_skip_cnt,%bx	// get # of packets to discard
of_drop_loop1:
	push	%cx
	push	%bx
	xor	%bl,%bl
	mov	%ah,%bh         // Start of next pkt

	// get header
	mov	$4,%cx		// 4 bytes only
	mov	$0,%al		// local xfer
	call	dma_read

	mov	0(%di),%ax	// AH : next record, AL : status
	pop	%bx		// packet counter
	pop	%cx		// need CURRENT (front of queue)
	cmp	$0,%bx		// Zero = keep the oldest packet, skip the rest.
	jnz	of_drop_loop2

	mov	%cl,%al		// save BOUNDARY for return
	push	%ax
	mov	$0x42,%al	// page 1
	mov	net_port,%dx	// Command register
	out	%al,%dx

	mov	net_port,%dx
	add	$io_ne2k_rx_put,%dx
	mov	%ah,%al		// set CURRENT to the beginning of the next pkt,
	out	%al,%dx		// effectively clearing everything but the 
				// first pkt in the buffer.
	mov	net_port,%dx	// Command register
	mov	$2,%al
	out	%al,%dx		// page 0
	pop	%ax
	jmp	of_drop_ok
of_drop_loop2:
	cmp	%ch,%ah		// Has the tail caught up with the head yet?
	jz	of_wraparound
	dec	%bx	
	jnz	of_drop_loop1
	// discard completed, get %ax in order for return

of_wraparound:
	mov	%ah,%al
	dec	%al	// don't care about wraparound, this is debug info

of_drop_ok:
	push	%ax	// save for return
	mov	net_port,%dx	// set tx back to normal
	add	$io_ne2k_tx_conf,%dx

	xor	%al,%al
	out	%al,%dx
	mov	net_port,%dx	// clear all interrupt bits
	add	$io_ne2k_int_stat,%dx
	in	%dx,%al
	out	%al,%dx

	pop	%ax	// return value as if we'd called 
			// getpage() (for debugging)
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
// and accumulate the numbers in order to be of value.
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
	mov	$0x08,%al	// Clear TXE bit in ISR
	out	%al,%dx

	mov	net_port,%dx
	add	$io_ne2k_tx_stat,%dx
	in	%dx,%al
	xor	%ah,%ah
	ret

//-----------------------------------------------------------------------------
