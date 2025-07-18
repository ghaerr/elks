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
#include <arch/irq.h>
#include <arch/segment.h>

#ifdef CONFIG_FS_XMS

/*
 * Set the below =1 to automatically disable using XMS INT 15 w/HMA instead of
 * hanging the system during boot, when hma=kernel and INT 15/1F disables A20
 * in Compaq and most other BIOSes (QEMU and DosBox-X do not).
 * Otherwise, when =0, hma=kernel must be commented out in /bootopts
 * to boot when configured for xms=int15 on those same systems.
 */
#define AUTODISABLE     1       /* =1 to disable XMS w/HMA if BIOS INT 15 disables A20 */

/* these used when running XMS_INT15 or XMS_LOADALL */
struct gdt_table;
void bios_block_movew(struct gdt_table *gdtp, size_t words);	    /* INT 15/1F */
#define COPY            0       /* block move */
#define CLEAR           1       /* block clear */
int loadall_block_op(struct gdt_table *gdtp, size_t bytes, int op); /* LOADALL */
static void int15_fmemcpy(void *dst_off, addr_t dst_seg, void *src_off, addr_t src_seg,
        size_t bytes, int op);

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
	if (xms_bootopts == XMS_DISABLED) {
		printk("off. ");
		return XMS_DISABLED;
	}
	/* check forced INT 15/1F or not 80286/80386 */
	if (xms_bootopts == XMS_INT15 || arch_cpu < CPU_80286) {
#if AUTODISABLE
		if (kernel_cs == 0xffff) {
			/* BIOS INT 15/1F block_move disables A20 on most systems! */
			printk("disabled w/kernel HMA and int 15/1F\n");
			return XMS_DISABLED;
		}
#endif
		printk("int 15/1F, ");
		enabled = XMS_INT15;
	} else {
		if (arch_cpu == CPU_80286) {/* 80286 only */
			printk("LOADALL, ");
			enabled = XMS_LOADALL;
		} else {                    /* 80386 only */
			enable_unreal_mode();
			printk("unreal mode, ");
			enabled = XMS_UNREAL;
		}
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

		if (need_xms_src && need_xms_dst)
			debug("xms to xms fmemcpy %08lx -> %08lx %u\n",
				src_seg + (unsigned)src_off, dst_seg + (unsigned)dst_off, count);

		if (xms_enabled == XMS_UNREAL)
			linear32_fmemcpyw(dst_off, dst_seg, src_off, src_seg, count);
		else
			int15_fmemcpy(dst_off, dst_seg, src_off, src_seg, count << 1, COPY);
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
		if ((count & 1) && xms_enabled == XMS_INT15) {
			static char buf[2];
			size_t wc = count >> 1;

			if (wc)
				int15_fmemcpy(dst_off, dst_seg, src_off, src_seg, wc << 1, COPY);
#pragma GCC diagnostic ignored "-Wpointer-arith"
			dst_off += count-1;
			src_off += count-1;

			if (need_xms_src) {
				/* move from XMS to kernel data segment */
				int15_fmemcpy(buf, (addr_t)kernel_ds<<4, src_off, src_seg, 2, COPY);
				pokeb((word_t)dst_off, (seg_t)(dst_seg>>4), buf[0]);
			} else {
				/* move from kernel data segment to XMS, very infrequent for odd count */
				addr_t kernel_ds_32 = (addr_t)kernel_ds << 4;
				int15_fmemcpy(buf, kernel_ds_32, dst_off, dst_seg, 2, COPY);
				buf[0] = peekb((word_t)src_off, (seg_t)(src_seg>>4));
				int15_fmemcpy(dst_off, dst_seg, buf, kernel_ds_32, 2, COPY);
			}
			return;
		}
		/* count can be odd bytes for XMS_LOADALL */
		int15_fmemcpy(dst_off, dst_seg, src_off, src_seg, count, COPY);
	  }
	  return;
	}
	fmemcpyb(dst_off, (seg_t)dst_seg, src_off, (seg_t)src_seg, count);
}

/* clear XMS or far memory, INT 15/1F not supported */
void xms_fmemset(void *dst_off, ramdesc_t dst_seg, size_t count)
{
	int	need_xms_dst = dst_seg >> 16;

	if (need_xms_dst) {
		if (xms_enabled == XMS_INT15) panic("xms_fmemset");
		if (xms_enabled == XMS_UNREAL)
			linear32_fmemset(dst_off, dst_seg, 0, count);
		else int15_fmemcpy(dst_off, dst_seg, 0, 0, count, CLEAR);
		return;
	}
	fmemsetb(dst_off, (seg_t)dst_seg, 0, count);
}

/* the following struct and code is used for XMS INT 15/1F and LOADALL block moves only */
struct gdt_table {
	word_t	limit_15_0;
	word_t	base_15_0;
	byte_t	base_23_16;
	byte_t	access_byte;
	byte_t	flags_limit_19_16;
	byte_t	base_31_24;
};

static struct gdt_table gdt_table[8];   /* static table requires mutex below */

/* move/clear data between XMS and main memory using either BIOS INT 15/1F or LOADALL */
/* NOTE: BIOS INT 15/1F can't handle odd bytes or memory clear! */
static void int15_fmemcpy(void *dst_off, addr_t dst_seg, void *src_off, addr_t src_seg,
	size_t bytes, int op)
{
	struct gdt_table *gp;
	//if (xms_enabled == XMS_INT15 && ((bytes & 1) || op != COPY)) panic("int15_fmemcpy");

	src_seg += (word_t)src_off;
	dst_seg += (word_t)dst_off;

	clr_irq();			/* xms_fmemcpyw callable at interrupt time! */
	if (xms_enabled == XMS_INT15)	/* LOADALL only uses two entries */
		memset(gdt_table, 0, sizeof(gdt_table));
	gp = &gdt_table[2];		/* source descriptor*/
	gp->limit_15_0 = 0xffff;	/* must be FFFF for LOADALL */
	gp->base_15_0 = (word_t)src_seg;
	gp->base_23_16 = src_seg >> 16;
	gp->access_byte = 0x92;		/* present, data, expand-up, writable */
	gp->flags_limit_19_16 = 0;	/* byte-granular, 16-bit, limit=64K */
	gp->base_31_24 = src_seg >> 24;

	gp = &gdt_table[3];		/* dest descriptor*/
	gp->limit_15_0 = 0xffff;	/* must be FFFF for LOADALL */
	gp->base_15_0 = (word_t)dst_seg;
	gp->base_23_16 = dst_seg >> 16;
	gp->access_byte = 0x92;		/* present, data, expand-up, writable */
	gp->flags_limit_19_16 = 0;	/* byte-granular, 16-bit, limit=64K */
	gp->base_31_24 = dst_seg >> 24;
	/* interrupts re-enabled in block_move or loadall_block_op routine */
	if (xms_enabled == XMS_LOADALL)
		loadall_block_op(gdt_table, bytes, op); /* block move/clear bytes */
	else 
		bios_block_movew(gdt_table, bytes >> 1);
}

#endif /* CONFIG_FS_XMS */
