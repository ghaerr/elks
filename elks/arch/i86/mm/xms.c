/*
 * XMS memory management support.
 *
 * Nov 2021 Greg Haerr
 */

#include <linuxmt/types.h>
#include <linuxmt/memory.h>
#include <linuxmt/kernel.h>

#ifdef CONFIG_FS_XMS_BUFFER

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
static long_t xms_alloc_ptr = 0x00100000L;	/* 1M */

/* try to enable unreal mode and A20 gate. Return 1 if successful */
int xms_init(void)
{
	if (enable_unreal_mode() > 0) {
		if (/*verify_a20() ||*/ enable_a20_gate()) {
			xms_enabled = 1;	/* enables xms_fmemcpyw()*/

			printk("xms: unreal mode and A20 enabled\n");
		} else
			printk("xms: can't enable A20 gate\n");
	} else
		printk("xms: can't enable unreal mode (needs 386)\n");

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

		linear32_fmemcpyw(dst_off, dst_seg, src_off, src_seg, count);
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

		linear32_fmemcpyb(dst_off, dst_seg, src_off, src_seg, count);
		return;
	}
	fmemcpyb(dst_off, (seg_t)dst_seg, src_off, (seg_t)src_seg, count);
}

#endif /* CONFIG_FS_XMS_BUFFER */
