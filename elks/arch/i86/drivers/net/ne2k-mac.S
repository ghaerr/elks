//-----------------------------------------------------------------------------
// NE2K driver - low part - MAC routines
//
// Updated by Helge Skrivervik July 2020:
//	. pick up MAC address from prom
//	. fixed read ring buffer wrap around errors
//	. added ring buffer overflow handling
//-----------------------------------------------------------------------------

#include "arch/ports.h"

	.code16

// TODO: move definitions to ne2k-defs.s
// adjust only this line for card base address
base               = NE2K_PORT     // I/O base address

// register array
io_ne2k_command    = base + 0x00  // command
io_ne2k_rx_first   = base+0x01  // page 0
io_ne2k_rx_last    = base+0x02  // page 0
io_ne2k_rx_get     = base+0x03  // page 0

// This is not a true NE2K register
//io_ne2k_rx_put1  = base+0x06  // page 0 - read

io_ne2k_tx_start   = base+0x04  // page 0 - write
io_ne2k_tx_len1    = base+0x05  // page 0 - write
io_ne2k_tx_len2    = base+0x06  // page 0 - write

io_ne2k_int_stat   = base+0x07  // page 0

io_ne2k_dma_addr1  = base+0x08  // page 0
io_ne2k_dma_addr2  = base+0x09  // page 0
io_ne2k_dma_len1   = base+0x0A  // page 0 - write
io_ne2k_dma_len2   = base+0x0B  // page 0 - write

io_ne2k_rx_stat    = base+0x0C  // page 0 - read

io_ne2k_rx_conf    = base+0x0C  // page 0 - write
io_ne2k_tx_conf    = base+0x0D  // page 0 - write
io_ne2k_data_conf  = base+0x0E  // page 0 - write
io_ne2k_int_mask   = base+0x0F  // page 0 - write

io_ne2k_unicast    = base+0x01  // page 1 - 6 bytes
io_ne2k_rx_put     = base+0x07  // page 1
io_ne2k_multicast  = base+0x08  // page 1 - 8 bytes

io_ne2k_data_io    = base+0x10  // 2 bytes

io_ne2k_reset      = base+0x1F


// Ring segmentation

tx_first           = 0x40
rx_first           = 0x46
rx_last            = 0x80

	.text
// --- not the right thing to do ...
_ne2k_next_pk:
	.word 0	// being used as byte ...

	.global _ne2k_skip_cnt
_ne2k_skip_cnt:
	.word 0	// # of packets to skip if buffer overrun, default is all (0)

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

	mov     $io_ne2k_command,%dx
	mov	$0x42,%al	// page 1
	out	%al,%dx

	// load MAC address

	mov     $io_ne2k_unicast,%dx
	mov     $6,%cx
	cld

ems_loop:

	lodsb
	out     %al,%dx
	inc     %dx
	loop    ems_loop

	mov     $io_ne2k_command,%dx
	mov	$0x02,%al	// back to pg 0
	out	%al,%dx

	pop     %si
	pop     %bp
	ret

//-----------------------------------------------------------------------------
// DMA initialization - Prepare for internal NIC DMA transfer
//-----------------------------------------------------------------------------

// BX : chip memory address (4000h...8000h)
// CX : byte count

dma_init:

	push    %ax
	push    %dx

	// assume page 0

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
//-------------------------------------
// TODO: It would make sense to have the first read operation get a full page (256 bytes) 
// instead of just the first 4 bytes. That way a single DMA read operation will cover
// most incoming packets in interactive sessions.
//

dma_write:

	push    %ax
	push    %cx
	push    %dx
	push    %si

	cli		//Experimental
	inc     %cx     // make byte count even
	and     $0xfffe,%cx
	call    dma_init
	shr     %cx     // half -> word size transf


	// start DMA write

	mov     $io_ne2k_command,%dx
	//mov	$0x0a,%al	// per app note (P 1-17)
	//out     %al,%dx
	mov	$0x12,%al	// per app note (P 1-17)
	out     %al,%dx

	// I/O write loop

	mov     $io_ne2k_data_io,%dx
	cld

emw_loop:

	lodsw
	out     %ax,%dx
	loop    emw_loop

	// wait for DMA completed

	mov     $io_ne2k_int_stat,%dx
check_dma_w:
	in      %dx,%al
	test    $0x40,%al       // dma done?
	jz      check_dma_w     // loop if not

	mov     $0x40,%al       //clear DMA intr bit in ISR
	out     %al,%dx
	clc


	sti		// Experimental
	pop     %si
	pop     %dx
	pop     %cx
	pop     %ax
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

	mov	$io_ne2k_tx_len2,%dx
	mov	$0x0f,%al	// prep for using the 'send packet' cmd
	out	%al,%dx

	mov     $io_ne2k_command,%dx
	mov	$0x18,%al	// send packet
	out	%al,%dx
	// now the dma does the rest, and an RDC interrupt is fielded when complete
	// we can loop here while waiting, or return and handle completion separately.
	in	%dx,%al
	test	$0x40,%al
	jz	rlp
rlp1:	
	mov	$io_ne2k_int_stat,%dx
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
//
// FIXME: The first read operation should get a full page (256 bytes)
// instead of just the first 4 bytes. That way a single DMA read operation will cover
// most incoming packets in interactive sessions.
//-----------------------------------------------------------------------------

// BX    : chip memory to read from
// CX    : byte count
// ES:DI : host memory to write to

dma_read:

	push    %ax
	push    %cx
	push    %dx
	push    %di

	cli		//Experimental
	inc     %cx     // make byte count even
	and     $0xfffe,%cx
	call    dma_init
	shr     %cx     // half -> word size transf

	// start DMA read

	mov     $io_ne2k_command,%dx
	//in	%dx,%al
	//and	$0x07,%al	// ensure pg 0
	//or	$0x08,%al	// 0x8 = read
	mov	$0x0a,%al	// 0ah per application note
	out     %al,%dx

	// I/O read loop

	push    %es  // compiler scratch
	mov     %ds,%ax
	mov     %ax,%es


	mov     $io_ne2k_data_io,%dx
	cld
	//cli	// FIXME disable interrupts - for testing

emr_loop:

	in      %dx,%ax
	stosw
	loop    emr_loop

	//sti	// re-enable int
	pop	%es

	// wait for DMA to complete

	mov     $io_ne2k_int_stat,%dx

check_dma_r:
	in      %dx,%al
	test    $0x40,%al       // dma done?
	jz      check_dma_r     // loop if not

	mov     $0x40,%al       // reset ISR (RDC bit only)
	out     %al,%dx

	sti		//Experimental

	pop     %di
	pop     %dx
	pop     %cx
	pop     %ax

	ret

//
//-----------------------------------------------------------------------
// ne2k_getpage -- return current page numbers in BOUNDARY / CURRENT registers
// AH = CURRENT, AL = BOUNDARY
// for debugging only
//
	.global ne2k_getpage

ne2k_getpage:
	mov	$0x42,%al	// page 1
	mov	$io_ne2k_command,%dx
	out	%al,%dx

	mov     $io_ne2k_rx_put,%dx     // CURRENT
	in      %dx,%al
	mov     %al,%cl

	mov	$0x02,%al	// page 0
	mov	$io_ne2k_command,%dx
	out	%al,%dx

	mov     $io_ne2k_rx_get,%dx     // BOUNDARY
	in      %dx,%al
	mov     %cl,%ah

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

	mov	$0x42,%al	// page 1
	mov	$io_ne2k_command,%dx
	out	%al,%dx

	mov     $io_ne2k_rx_put,%dx
	in      %dx,%al
	mov     %al,%cl

	mov	$0x02,%al	// back to page 0
	mov	$io_ne2k_command,%dx
	out	%al,%dx

	// get RX get pointer

	mov	_ne2k_next_pk,%al

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
	push	%bx

	// get RX put pointer

	mov	_ne2k_next_pk,%bh
	xor	%bl,%bl		//	Next pkt to read in BX

	// get packet header	FIXME - read entire page instead, may happen to
	// 			contain the entire packet

	mov     4(%bp),%di	// Buffer address to receive data - duplicate above ...
	mov     $4,%cx	
	call    dma_read

	mov     0(%di),%ax  // AH : next record, AL : status
	mov     2(%di),%cx  // packet size (without CRC)

	// check packet size - not really required since the NIC will not
	// accept such packets per our initialization

	or      %cx,%cx
	jz      npg_err2

	cmp     $1528,%cx  // max - head - crc
	jnc     npg_err

	add     $4,%bx		// source memory address 'inside ' the chip
	add     $4,%di		// Destination memory address (+4 to keep the header)

	push    %ax
	push    %cx

	// get packet body

	call    dma_read
	pop     %cx

	// update RX get pointer

	pop     %ax
	xchg    %al,%ah			// get next pointer to %al
	mov	%al,_ne2k_next_pk	// save 'real' next ptr
	dec	%al
	cmp	$rx_first,%al
	jnb	npg_next		// if the decrement sent us outside the ring..
	mov	$rx_last-1,%al		// make it right ...

npg_next:

	mov     $io_ne2k_rx_get,%dx	// update read_ptr reg (BOUNDARY)
	out     %al,%dx

	xor     %ax,%ax
	jmp     npg_exit

npg_err:

	mov     $-1,%ax
	jmp	npg_exit

npg_err2:
	mov	$-2,%ax

npg_exit:
#if 0
	mov	%ax,%bx	// save return value
	pop	%ax	// check if we have buffer overflow pending
	test	$0x10,%al
	jz	npg_finis
	call	ne2k_clr_oflow	// do the rest of the reset processing
npg_finis:
	mov	%bx,%ax	// restore return value
#endif	

// clear RX bit in ISR
	mov     %ax,%bx // save
	// assume page 0
	mov     $io_ne2k_int_stat,%dx   // reset the interrupt bit
	mov     $1,%al       // NE2K_STAT_RX 
	out     %al,%dx
	mov     %bx,%ax // unsave

	pop	%bx
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

	//cli		// Experimental
	push    %bp
	mov     %sp,%bp
	push    %si  // used by compiler

	// assume register page 0

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
	//in	%dx,%al		// NOTE: THIS DOES NOT WORK
	//and	$0x38,%al	// preserve Remote DMA command bits
	//or	$0x6,%al	// set the TXP bit (and STA, not really required)
				// this works:
	mov	$6,%al		// Set TX bit, starts transfer...
	out	%al,%dx

	mov	$io_ne2k_int_stat,%dx	// reset tx intr bit
	mov	$2,%al		// Test, should not make any difference
	out	%al,%dx
				// Not waiting for completion
	xor     %ax, %ax	// always zero return

	pop     %si
	pop     %bp
	//sti		//Experimental
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

	// assume page 0

	// get interrupt status

	xor     %ah,%ah

	mov     $io_ne2k_int_stat,%dx
	in      %dx,%al
	test    $0x13,%al	// ring buffer overflow, tx, rx
				// Dont reset RDC intr here, it will break 
				// the dma_read/write routines
	jz      nis_next

	// clear TX interrupt only
	push	%ax	
	//mov	$2,%al		// removing this for test, reset all ints see what happens ..
	out     %al,%dx
	pop	%ax

nis_next:

	ret

//-----------------------------------------------------------------------------
// NE2K initialization
//-----------------------------------------------------------------------------

	.global ne2k_init

ne2k_init:

	// Stop DMA and MAC

	mov     $io_ne2k_command,%dx
	mov	$0x21,%al	// page 0 + Abort DMA; STOP
	out     %al,%dx

	// data I/O in words for PC/AT and higher

	mov     $io_ne2k_data_conf,%dx
	mov     $0x49,%al
	out     %al,%dx

	// clear DMA length - Important!

	xor     %al,%al
	mov     $io_ne2k_dma_len1,%dx
	out     %al,%dx
	inc     %dx  // = io_ne2k_dma_len2
	out     %al,%dx

	// accept packet without error
	// unicast & broadcast & promiscuous

	mov     $io_ne2k_rx_conf,%dx
	//mov     $0x54,%al	// 54 is nonsensical
	mov     $0x04,%al	// ++ Broadcast OK, no multicast
	out     %al,%dx

	// half-duplex and internal loopback
	// to insulate the MAC while stopped

	mov     $io_ne2k_tx_conf,%dx
	mov     $2,%al  // 2 for loopback
	out     %al,%dx

	// set RX ring limits
	// all 16KB on-chip memory
	// except one TX frame at beginning (6 x 256B)

	mov     $io_ne2k_rx_first,%dx
	mov     $rx_first,%al
	out     %al,%dx

	// set RX get pointer -- BOUNDARY

	mov     $io_ne2k_rx_get,%dx
	out     %al,%dx

	mov     $io_ne2k_rx_last,%dx
	mov     $rx_last,%al
	out     %al,%dx

	mov     $io_ne2k_tx_start,%dx
	mov     $tx_first,%al
	out     %al,%dx

	// clear all interrupt flags

	mov     $io_ne2k_int_stat,%dx
	mov     $0x7F,%al
	out     %al,%dx

	// set interrupt mask
	// TX & RX without error and overflow

	mov     $io_ne2k_int_mask,%dx
	mov     $0x13,%al	// 53 = Overflow, RX, TX + RDC (debug)
	out     %al,%dx

	mov	$0x42,%al	// page 1
	mov	$io_ne2k_command,%dx
	out	%al,%dx

	// set RX put pointer  = RX get

	mov     $io_ne2k_rx_put,%dx
	mov     $rx_first,%al
	inc     %al	// set CURRENT = always one ahead
	out     %al,%dx
	mov	%al,_ne2k_next_pk

	mov	$0x02,%al	// page 0
	mov	$io_ne2k_command,%dx
	out	%al,%dx

	// now enable transmitter
	mov     $io_ne2k_tx_conf,%dx
	mov     $0,%al  // 2 for loopback
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
	//in      %dx,%al
	//and     $0xC0,%al
	//or      $0x21,%al
	mov	$0x21,%al	// page 0 + stop
	out     %al,%dx

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

	// assume page 0

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
// Get  MAC address from NIC's prom
// WARNING: This function will reset the controller. Use before the init()!
//-----------------------------------------------------------------------------

// arg1 : pointer to 6 bytes buffer

	.global ne2k_get_hw_addr

ne2k_get_hw_addr:

	push    %bp
	mov     %sp,%bp
	push    %di  // used by compiler

	mov     4(%bp),%di

	// Effectively a partly reset of the NIC, required in order to get access to the
	// PROM - 32 bytes of which only the first 6 bytes are of interest
	// This routine leaves the NIC initialized but not activated (TX is in loopback).

	//mov   $io_ne2k_reset,%dx
	//in    %dx,%al
	//out   %al,%dx         //reset NIC
	//mov   $io_ne2k_int_stat,%dx

w_reset:
	//in    %dx,%al // wait for reset to complete
	//test  $0x80,%al
	//jz    w_reset

	mov   $io_ne2k_command,%dx
	mov   $0x21,%al       // pg 0, stop, reset DMA
	out   %al,%dx
	mov     $io_ne2k_data_conf,%dx
	mov     $0x49,%al       //word access
	out     %al,%dx
	mov	$io_ne2k_dma_len1, %dx
	xor	%al,%al         // clear count regs
	out	%al,%dx         // 
	inc	%dx
	out	%al,%dx
	mov   $io_ne2k_int_mask,%dx
	out   %al,%dx         // mask completion irq
	mov   $io_ne2k_int_stat,%dx
	mov   $0xff,%al
	out   %al,%dx // clear interrupt status reg, required

	mov   $io_ne2k_rx_conf,%dx
	mov   $0x20,%al
	out   %al,%dx         // set to monitor mode
	inc   %dx             // $io_ne2k_tx_conf
	mov   $2,%al          // %al = 2
	out   %al,%dx         // Loopback mode

// use dma read instead
	mov	$32,%cx
	xor	%bx,%bx	// read from 0:0
	call	dma_read

	mov	$io_ne2k_tx_conf,%dx	// set tx back to normal
	xor	%al,%al
	out	%al,%dx

	pop	%di
	pop     %bp
	ret

//-----------------------------------------------------------------------------
// NE2K clear overflow --- respond to an input ring buffer overflow interrupt
// The recovery reads the last compete pcket into the provided (arg1) buffer.
//-----------------------------------------------------------------------------
//      Parameters:
//      Input: # of packets to discard 4(%sp)
//             4 byte buffer for packet header read
//      Returns: AL = new BOUNDARY ptr, AH = CURRENT ptr
//               Byte Buffer contains the header of the last packet read
//               or 00 if everything was discarded.

	.global ne2k_clr_oflow

ne2k_clr_oflow:

        mov	$io_ne2k_command,%dx
        mov	$0x21,%al       // pg 0, stop, reset DMA
        out	%al,%dx
	
	mov	$io_ne2k_dma_len1,%dx
	xor	%al,%al
	out	%al,%dx
	inc	%dx
	out	%al,%dx	// clear dma counter

	push	%bp
	mov	%sp,%bp
	push	%di
	push	%cx

	mov	_ne2k_skip_cnt,%bx	// get # of packets to discard
	mov	4(%bp),%di	// where to put the header while discarding...

	mov	$io_ne2k_int_stat,%dx

of_reset:	// May need a timeout counter here to avoid being stuck if something goes wrong ...
	in	%dx,%al		// wait for reset to complete
	test	$0x80,%al
	jz	of_reset

	mov	$io_ne2k_tx_conf,%dx	// must set tx to loopback
	mov	$2,%al
	out	%al,%dx
	mov	$io_ne2k_command,%dx	// restart NIC
	mov	$0x22,%al
	out	%al,%dx
	
	// NIC has stopped, now clear out the ring buffer
	// -- either the given # of frames or the whole shebang -
        // by manipulating the BOUNDARY pointer to discard packets.

	call	ne2k_getpage    // get BOUNDARY (AL) and CURRENT (AH) pointers

	cmp	$0,%bx		// zero - just empty the ring buffer
	jnz	of_drop_packets	// not zero - dicard %bx packets

	mov	%ah,%al         // CURRENT = BOUNDARY
	cmp	$rx_first,%ah   // if CURRENT is at the beginning of the ring (!)
	jnz	of_clr_pk
	mov	$rx_last+1,%al  // do the 'one behind' trickery

of_clr_pk:
	mov	%ah,_ne2k_next_pk // save real boundary ptr
	dec	%al
	mov	$io_ne2k_rx_get,%dx     // fake BOUNDARY ptr
	out	%al,%dx
	jmp	of_drop_ok

of_drop_packets:
	// loop through the number of packets given by %bx
	// terminate if we reach the head of the buffer before the # of packets
	mov	%ax,%cx		// save BOUNDARY & CURRENT
	mov	_ne2k_next_pk,%ah
of_drop_loop1:
	push	%cx
	push	%bx
	xor	%bl,%bl
	mov	%ah,%bh         // Start of next pkt

	// get header
	mov	$4,%cx		// 4 bytes only
	call	dma_read

	mov	0(%di),%ax	// AH : next record, AL : status
	//mov	2(%di),%cx	// packet size (without CRC)
	pop	%bx		// packet counter
	pop	%cx		// need CURRENT (front of queue)
	cmp	%ch,%ah		// has the tail cought up with the head yet?
	jz	of_wraparound
	dec	%bx	
	jnz	of_drop_loop1
	// discard completed, get %ax in order for return

of_wraparound:
	mov	%ah,%al
	dec	%al	// don't care about wraparound, this is debug info

of_drop_ok:
	push	%ax	// save for return
	mov	$io_ne2k_tx_conf,%dx	// set tx back to normal

	xor	%al,%al
	out	%al,%dx
	mov	$io_ne2k_int_stat,%dx	// clear interrupt
	in	%dx,%al			// (all active bits, this is effectively a reset)
	out	%al,%dx

	pop	%ax	// return value as if we'd callet get_page() (for debugging)
	pop	%cx
	pop	%di
	pop	%bp
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
	mov     $io_ne2k_int_stat,%dx   // reset the interrupt bit
	mov     $0x40,%al
	out     %al,%dx

	mov     $1,%ax
#endif
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
