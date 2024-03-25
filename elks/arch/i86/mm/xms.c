/*
 * Extended (> 1M) memory management support for buffers.
 *
 * Nov 2021 Greg Haerr
 */

#include <linuxmt/config.h>
#include <linuxmt/memory.h>
#include <linuxmt/kernel.h>
#include <linuxmt/init.h>

#include <linuxmt/string.h>
#include <arch/segment.h>

/* linear address to start XMS buffer allocations from */
#define XMS_START_ADDR    0x00100000L	/* 1M */
//#define XMS_START_ADDR  0x00FA0000L	/* 15.6M (Compaq with only 1M ram) */

#ifdef CONFIG_FS_XMS_BUFFER

/* these used in CONFIG_FS_XMS_INT15 only */
struct gdt_table;
extern int block_move(struct gdt_table *gdtp, size_t words);
extern void int15_fmemcpyw(void *dst_off, addr_t dst_seg, void *src_off, addr_t src_seg,
		size_t count);

/*
 * ramdesc_t: if CONFIG_FS_XMS_BUFFER not set, then it's a normal seg_t segment descriptor.
 * Otherwise, it's a physical RAM descriptor (32 bits), used for possible XMS access.
 * When <= 65535, low 16 bits are used as the (seg_t) physical segment.
 * When > 65535, all 32 bits are used as a linear address in unreal mode.
 *
 * addr_t: linear 32-bit RAM address, used directly by CPU using 32-bit
 *     address operand prefix and unreal mode for indexing off of DS:ESI or ES:EDI
 *     in linear32_fmemcypw.
 */

static int xms_enabled;
static long_t xms_alloc_ptr = XMS_START_ADDR;

/* try to enable unreal mode and A20 gate. Return 1 if successful */
int xms_init(void)
{
	int enabled;

	/* display initial A20 and A20 enable result */
	printk("xms: ");
#ifndef CONFIG_FS_XMS_INT15
	if (check_unreal_mode() <= 0) {
		printk("disabled, requires 386, ");
		return 0;
	}
#endif
	printk("A20 was %s", verify_a20()? "on" : "off");
	enable_a20_gate();
	enabled = verify_a20();
	printk(" now %s, ", enabled? "on" : "off");
	if (!enabled) {
		printk("disabled, A20 error, ");
		return 0;
	}
#ifdef CONFIG_FS_XMS_INT15
	printk("using int 15/1F, ");
#else
	enable_unreal_mode();
	printk("using unreal mode, ");
#endif
	xms_enabled = 1;	/* enables xms_fmemcpyw()*/
	return xms_enabled;
}

/* allocate from XMS memory - very simple for now, no free */
ramdesc_t xms_alloc(long_t size)
{
	long_t mem = xms_alloc_ptr;

	xms_alloc_ptr += size;
	//printk("xms_alloc %lx size %lu\n", mem, size);
	return mem;
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

#ifndef CONFIG_FS_XMS_INT15
		linear32_fmemcpyw(dst_off, dst_seg, src_off, src_seg, count);
#else
		int15_fmemcpyw(dst_off, dst_seg, src_off, src_seg, count);
#endif
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

#ifndef CONFIG_FS_XMS_INT15
		linear32_fmemcpyb(dst_off, dst_seg, src_off, src_seg, count);
#else
		/* lots of extra work on odd transfers because INT 15 block moves words only */
		size_t wc = count >> 1;
		if (count & 1) {
			static char buf[2];

			if (wc)
				int15_fmemcpyw(dst_off, dst_seg, src_off, src_seg, wc);
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
#endif
		return;
	}
	fmemcpyb(dst_off, (seg_t)dst_seg, src_off, (seg_t)src_seg, count);
}

/* memset XMS or far memory, INT 15 not yet supported */
void xms_fmemset(void *dst_off, ramdesc_t dst_seg, byte_t val, size_t count)
{
	int	need_xms_dst = dst_seg >> 16;

	if (need_xms_dst) {
		if (!xms_enabled) panic("xms_fmemset");

#ifdef CONFIG_FS_XMS_INT15
		panic("xms_fmemset int15");
#else
		linear32_fmemset(dst_off, dst_seg, val, count);
#endif
		return;
	}
	fmemsetb(dst_off, (seg_t)dst_seg, val, count);
}

#ifdef CONFIG_FS_XMS_INT15
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
	gp->access_byte = 0x93;		/* present, rignt 0, data, expand-up, writable, accessed */
	//gp->flags_limit_19_16 = 0;	/* byte-granular, 16-bit, limit=64K */
	//gp->flags_limit_19_16 = 0xCF;	/* page-granular, 32-bit, limit=4GB */
	gp->base_31_24 = src_seg >> 24;

	gp = &gdt_table[3];		/* dest descriptor*/
	gp->limit_15_0 = 0xffff;
	gp->base_15_0 = (word_t)dst_seg;
	gp->base_23_16 = dst_seg >> 16;
	gp->access_byte = 0x93;		/* present, rignt 0, data, expand-up, writable, accessed */
	//gp->flags_limit_19_16 = 0;	/* byte-granular, 16-bit, limit=64K */
	//gp->flags_limit_19_16 = 0xCF;	/* page-granular, 32-bit, limit=4GB */
	gp->base_31_24 = dst_seg >> 24;
	block_move(gdt_table, count);
}
#endif

#endif /* CONFIG_FS_XMS_BUFFER */
