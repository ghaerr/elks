#include <arch/bitops.h>
#include <arch/irq.h>
#include <linuxmt/kernel.h>

unsigned long bit_masks[] = 
	{1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 
  	32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 
	8388608, 16777216, 33554432, 67108864, 134217728, 268435456,  
	536870912, 1073741824, 2147483648};

/*
 *	Messy as we lack atomic bit operations on an 8086.
 */
 
int clear_bit(bit,addr)
int bit;
unsigned char *addr;
{
	unsigned int flags;
	int offset = (bit / 8);
	unsigned int r;
	bit%=8;
	save_flags(flags);
	icli();
	r=(addr[offset]&&(1<<bit));
	addr[offset] ^= r;	/* xor bit with itself is 0 */
	restore_flags(flags);
/*	if(r)
		return 1;
	return r; */
	return (r ? 1:0);
}

int set_bit(nr,addr)
int nr;
register unsigned int *addr;
{
	unsigned int mask, retval, offset;
	int i;

	icli();
	retval = test_bit(nr, addr);
	isti();
	if (retval) 
		return 1;
	else 
	{		
		offset = nr / 16;	
		mask = bit_masks[nr % 16];
		addr[offset] += mask;
		if (!test_bit(nr, addr)) panic("set_bit failed! %d\n", mask);
		return 0;
	}
}

#if 1
int test_bit(bit,addr)
int bit;
unsigned int *addr;
{
	unsigned int mask;
	int offset;
	int i;
	offset = (bit / 16);
	mask = bit_masks[bit % 16];
	return ((mask & addr[offset] ) != 0);
}
#else
#asm
	.globl _test_bit
	.text
	.even
_test_bit:
	push bp
	mov bp,sp

	mov cx,[bp+4]
	mov bx,cx
	and cl,#7
	shr bx,#1
	shr bx,#1
	shr bx,#1
	add bx,[bp+6]

	mov al,[bx]
	shr ax,cl
	and ax,#1

	pop bp
	ret
#endasm
#endif

/* Ack... nobody even seemed to try to write to a file before 0.0.49a was
 * released, or otherwise they might have tracked it down to this being
 * non-existant :) 
 * - Chad
 */
#if 1

#if 0
int find_first_zero_bit_test(addr, len)
unsigned long *addr;
int len;
{
	int res;

/*	printk("ffzb: new = %x, vnew = %x\n",
		res = find_first_zero_bit_new(addr,len),
		find_first_zero_bit_vnew(addr,len)); */ /* Just to make it compile*/
	return res;
}
#endif

int find_first_non_zero_bit(addr, len) /* Use the old faithful version */
unsigned long *addr;
int len;
{
	unsigned int i;

	for (i = 0; i < len; i++) {
		if (test_bit(i, addr)) {
			return i;
		}
	}	
	return len;
}

int find_first_zero_bit(addr, len) /* Use the old faithful version */
unsigned long *addr;
int len;
{
	unsigned int i;

	for (i = 0; i < len; i++) {
		if (!test_bit(i, addr)) {
			return i;
		}
	}	
	return len;
}
#if 0
int find_first_zero_bit_new(addr, len)
unsigned long *addr;
int len;
{
	unsigned int *ip = (unsigned int*)addr;
	unsigned int iw;
	unsigned int ib;
	unsigned int im;

	for(iw = 0; iw <= (len / 16); iw++, ip++) {
		if(*ip != UINT_MAX) {
			for(ib=0, im=1; ib<16; ib++, im <<= 1) {
				if (!(*ip & im)) {
					iw = (iw * 16) + ib;
					return (iw > len) ? len : iw;
				}
			}
		}
	}

	return len;
}
#endif
#else
#asm
	.globl _find_first_zero_bit
	.text
	.even
_find_first_zero_bit:
	push bp
	mov bp,sp
	push di

	mov cx,[bp+6]			! cx = len
	mov di,[bp+4]			! di = addr
	mov bx,di			! bx = saved addr

	mov ax,#$ffff
	shr cx,#1
	shr cx,#1
	shr cx,#1
	shr cx,#1			! search a word at a time....

! di -> start
! cx -> length in bits...
	cld
	repe
		scasw

	test cx,cx
	jnz __ff_n1
	mov ax,[bp+6]			! return len;
	jmp __ff_ret

__ff_n1:
! now we've got an empty bit in [di-2]
	sub bx,di
	add bx,#2
	neg bx
	shl bx,#1
	shl bx,#1
	shl bx,#1		! bx = offset & ~0xf
	mov ax,[di-2]

__ff_nx:			! don't need a counter, as ax != -1
	test ax,#1
	jz __ff_done
	inc bx
	shr ax,#1
	jmp __ff_nx

__ff_done:
	mov ax,bx

__ff_ret:
	pop di
	pop bp
	ret
#endasm
#endif

