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
 */

int xms_enabled;

void xms_fmemcpyw(void * dst_off, ramdesc_t dst_seg, void * src_off, ramdesc_t src_seg,
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

//printk("linear32_fmemcpyw %x,%lx %x,%lx %d\n", dst_off, dst_seg, src_off, src_seg, count);
		linear32_fmemcpyw(dst_off, dst_seg, src_off, src_seg, count);

	} else
#endif
		fmemcpyw(dst_off, (seg_t)dst_seg, src_off, (seg_t)src_seg, count);
}
