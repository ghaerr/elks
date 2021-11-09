/*
 * XMS memory management support.
 *
 * Nov 2021 Greg Haerr
 */

#include <linuxmt/types.h>
#include <linuxmt/memory.h>
#include <linuxmt/kernel.h>

/*
 * ramdesc_t: physical RAM descriptor (32 bits), used for possible XMS access.
 * When <= 65535, low 16 bits are used as the (seg_t) physical segment.
 * When > 65535, all 32 bits are used as a linear address in unreal mode.
 *
 * addr_t: linear 32-bit RAM address, used directly by CPU using 32-bit
 *     address operand prefix and unreal mode for indexing off of DS:ESI or ES:EDI
 *     in linear32_fmemcypw.
 */

int xms_enabled;

void xms_fmemcpyw(void *dst_off, ramdesc_t dst_seg, void *src_off, ramdesc_t src_seg,
		size_t count)
{
#ifdef CONFIG_FS_XMS_BUFFER
	int	need_xms_src = src_seg >> 16;
	int	need_xms_dst = dst_seg >> 16;

	if (need_xms_src || need_xms_dst) {
		if (!xms_enabled)
			panic("bad xms_fmemcpyw");

		if (!need_xms_src) src_seg <<= 4;
		if (!need_xms_dst) dst_seg <<= 4;

//printk("linear32_fmemcpyw %lx:%x %lx:%x %d\n", dst_seg, dst_off, src_seg, src_off, count);
		linear32_fmemcpyw(dst_off, dst_seg, src_off, src_seg, count);

	} else {
#endif
//printk("fmemcpyw %lx:%x %lx:%x %d\n", dst_seg, dst_off, src_seg, src_off, count);
		fmemcpyw(dst_off, (seg_t)dst_seg, src_off, (seg_t)src_seg, count);
	}
}

void xms_fmemcpyb(void *dst_off, ramdesc_t dst_seg, void *src_off, ramdesc_t src_seg,
		size_t count)
{
#ifdef CONFIG_FS_XMS_BUFFER
	int	need_xms_src = src_seg >> 16;
	int	need_xms_dst = dst_seg >> 16;

	if (need_xms_src || need_xms_dst) {
		if (!xms_enabled)
			panic("bad xms_fmemcpyb");

		if (!need_xms_src) src_seg <<= 4;
		if (!need_xms_dst) dst_seg <<= 4;

//printk("linear32_fmemcpyb %lx:%x %lx:%x %d\n", dst_seg, dst_off, src_seg, src_off, count);
		linear32_fmemcpyb(dst_off, dst_seg, src_off, src_seg, count);

	} else {
#endif
//printk("fmemcpyb %lx:%x %lx:%x %d\n", dst_seg, dst_off, src_seg, src_off, count);
		fmemcpyb(dst_off, (seg_t)dst_seg, src_off, (seg_t)src_seg, count);
	}
}
