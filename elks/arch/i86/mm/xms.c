/*
 * Extended (> 1M) memory management support for buffers and ramdisk.
 *
 * Nov 2021 Greg Haerr
 */

#include <linuxmt/config.h>
#include <linuxmt/memory.h>
#include <linuxmt/kernel.h>
#include <linuxmt/init.h>
#include <linuxmt/string.h>
#include <linuxmt/debug.h>
#include <arch/segment.h>

#ifdef CONFIG_FS_XMS

/*
 * Set the below =1 to automatically disable using XMS INT 15 w/HMA instead of
 * hanging the system during boot, when hma=kernel and INT 15/1F disables A20
 * in Compaq and most other BIOSes (QEMU and DosBox-X do not).
 * Otherwise, when =0, hma=kernel must be commented out in /bootopts
 * to boot when configured for xms=int15 on those same systems.
 */
#define AUTODISABLE		1		/* =1 to disable XMS w/HMA if BIOS INT 15 disables A20 */

/* these used only when running XMS_INT15 */
struct gdt_table;
int block_move(struct gdt_table *gdtp, size_t words);
void int15_fmemcpyw(void *dst_off, addr_t dst_seg, void *src_off, addr_t src_seg,
		size_t count);

/*
 * ramdesc_t: if CONFIG_FS_XMS not set, then it's a normal seg_t segment descriptor.
 * Otherwise, it's a physical RAM descriptor (32 bits), used for possible XMS access.
 * When <= 65535, low 16 bits are used as the (seg_t) physical segment.
 * When > 65535, all 32 bits are used as a linear address in unreal mode.
 *
 * addr_t: linear 32-bit RAM address, used directly by CPU using 32-bit
 *     address operand prefix and unreal mode for indexing off of DS:ESI or ES:EDI
 *     in linear32_fmemcypw.
 */

int xms_enabled;
unsigned int xms_alloc_ptr = KBYTES(XMS_START_ADDR);

/* try to enable XMS memory access using A20 gate and unreal mode or INT 15 block move */
int INITPROC xms_init(void)
{
	int enabled, size;
	static char xms_tried;

	/* don't repeat messages */
	if (xms_tried)
		return xms_enabled;
	xms_tried = 1;
	/* display initial A20 and A20 enable result */
	printk("xms: ");
	size = SETUP_XMS_KBYTES;
	printk("%uK, ", size);
	if (!size)                      /* 8086 systems won't have XMS */
		return XMS_DISABLED;
	enabled = enable_a20_gate();    /* returns verify_a20() */
	if (!enabled) {
		printk("disabled, A20 error. ");
		return XMS_DISABLED;
	}
	/* 80286 machines and Compaq BIOSes can't use unreal mode and must use INT 15/1F */
	if (xms_bootopts == XMS_INT15 || (arch_cpu <= 6 && xms_bootopts == XMS_UNREAL)) {
#if AUTODISABLE
		if(arch_cpu == 6){
			printk("LOADALL, ");
			enabled = XMS_INT15;
		}else if (kernel_cs == 0xffff) {
			/* BIOS INT 15/1F block_move disables A20 on most systems! */
			printk("disabled w/kernel HMA and int 15/1F\n");
			return XMS_DISABLED;
		}
#endif
		if(arch_cpu == 6)
			printk("LOADALL, ");
		else
			printk("int 15/1F, ");
		enabled = XMS_INT15;
	} else {
		if (xms_bootopts != XMS_UNREAL) {
			printk("off. ");
			return XMS_DISABLED;
		}
		enable_unreal_mode();
		printk("unreal mode, ");
		enabled = XMS_UNREAL;
	}
	if (kernel_cs == 0xffff)
		xms_alloc_ptr += 64;    /* 64K reserved for HMA kernel */
	xms_enabled = enabled;
	return xms_enabled;
}

/* allocate from XMS memory - very simple for now, no free */
ramdesc_t xms_alloc(unsigned int kbytes)
{
	unsigned int mem = xms_alloc_ptr;

	if (!xms_enabled)
		return 0;
	if (xms_alloc_ptr - KBYTES(XMS_START_ADDR) + kbytes > SETUP_XMS_KBYTES)
		return 0;
	xms_alloc_ptr += kbytes;
	//printk("xms_alloc %lx size %uK\n", (unsigned long)mem<<10, kbytes);
	return (ramdesc_t)mem << 10;
}

/* copy words between XMS and far memory */
void xms_fmemcpyw(void *dst_off, ramdesc_t dst_seg, void *src_off, ramdesc_t src_seg,
		size_t count)
{
	int	need_xms_src = src_seg >> 16;
	int	need_xms_dst = dst_seg >> 16;

	if (need_xms_src || need_xms_dst) {
		if (!xms_enabled) panic("xms_fmemcpyw");
		if (!need_xms_src) src_seg <<= 4;
		if (!need_xms_dst) dst_seg <<= 4;

	  if (xms_enabled == XMS_UNREAL)
		linear32_fmemcpyw(dst_off, dst_seg, src_off, src_seg, count);
	  else
		int15_fmemcpyw(dst_off, dst_seg, src_off, src_seg, count);
	  return;
	}
	fmemcpyw(dst_off, (seg_t)dst_seg, src_off, (seg_t)src_seg, count);
}

/* copy bytes between XMS and far memory */
void xms_fmemcpyb(void *dst_off, ramdesc_t dst_seg, void *src_off, ramdesc_t src_seg,
		size_t count)
{
	int	need_xms_src = src_seg >> 16;
	int	need_xms_dst = dst_seg >> 16;

	if (need_xms_src || need_xms_dst) {
		if (!xms_enabled) panic("xms_fmemcpyb");
		if (!need_xms_src) src_seg <<= 4;
		if (!need_xms_dst) dst_seg <<= 4;

	  if (xms_enabled == XMS_UNREAL)
		linear32_fmemcpyb(dst_off, dst_seg, src_off, src_seg, count);
	  else {
		/* lots of extra work on odd transfers because INT 15 block moves words only */
		size_t wc = count >> 1;
		if (count & 1) {
			static char buf[2];

			if (wc)
				int15_fmemcpyw(dst_off, dst_seg, src_off, src_seg, wc);
#pragma GCC diagnostic ignored "-Wpointer-arith"
			dst_off += count-1;
			src_off += count-1;

			if (need_xms_src) {
				/* move from XMS to kernel data segment */
				int15_fmemcpyw(buf, (addr_t)kernel_ds<<4, src_off, src_seg, 1);
				pokeb((word_t)dst_off, (seg_t)(dst_seg>>4), buf[0]);
			} else {
				/* move from kernel data segment to XMS, very infrequent for odd count */
				addr_t kernel_ds_32 = (addr_t)kernel_ds << 4;
				int15_fmemcpyw(buf, kernel_ds_32, dst_off, dst_seg, 1);
				buf[0] = peekb((word_t)src_off, (seg_t)(src_seg>>4));
				int15_fmemcpyw(dst_off, dst_seg, buf, kernel_ds_32, 1);
			}
			return;
		}
		int15_fmemcpyw(dst_off, dst_seg, src_off, src_seg, wc);
	  }
	  return;
	}
	fmemcpyb(dst_off, (seg_t)dst_seg, src_off, (seg_t)src_seg, count);
}

/* memset XMS or far memory, INT 15 not yet supported */
void xms_fmemset(void *dst_off, ramdesc_t dst_seg, byte_t val, size_t count)
{
	int	need_xms_dst = dst_seg >> 16;

	if (need_xms_dst) {
		if (xms_enabled != XMS_UNREAL) panic("xms_fmemset");
		linear32_fmemset(dst_off, dst_seg, val, count);
		return;
	}
	fmemsetb(dst_off, (seg_t)dst_seg, val, count);
}

/* the following struct and code is used for XMS INT 15 block moves only */
struct gdt_table {
	word_t	limit_15_0;
	word_t	base_15_0;
	byte_t	base_23_16;
	byte_t	access_byte;
	byte_t	flags_limit_19_16;
	byte_t	base_31_24;
};

static struct gdt_table gdt_table[8];

/* move words between XMS and main memory using BIOS INT 15h AH=87h block move */
void int15_fmemcpyw(void *dst_off, addr_t dst_seg, void *src_off, addr_t src_seg,
		size_t count)
{
	struct gdt_table *gp;
	memset(gdt_table, 0, sizeof(gdt_table));

	src_seg += (word_t)src_off;
	dst_seg += (word_t)dst_off;

	gp = &gdt_table[2];		/* source descriptor*/
	gp->limit_15_0 = 0xffff;
	gp->base_15_0 = (word_t)src_seg;
	gp->base_23_16 = src_seg >> 16;
	gp->access_byte = 0x93;	/* present, rignt 0, data, expand-up, writable, accessed */
	//gp->flags_limit_19_16 = 0;	/* byte-granular, 16-bit, limit=64K */
	//gp->flags_limit_19_16 = 0xCF;	/* page-granular, 32-bit, limit=4GB */
	gp->base_31_24 = src_seg >> 24;

	gp = &gdt_table[3];		/* dest descriptor*/
	gp->limit_15_0 = 0xffff;
	gp->base_15_0 = (word_t)dst_seg;
	gp->base_23_16 = dst_seg >> 16;
	gp->access_byte = 0x93;	/* present, rignt 0, data, expand-up, writable, accessed */
	//gp->flags_limit_19_16 = 0;	/* byte-granular, 16-bit, limit=64K */
	//gp->flags_limit_19_16 = 0xCF;	/* page-granular, 32-bit, limit=4GB */
	gp->base_31_24 = dst_seg >> 24;
	if(arch_cpu == 6)
	 loadall_block_move(gdt_table,count);
	else
	 block_move(gdt_table, count);
}

#endif /* CONFIG_FS_XMS */
