/*
 * Sound Blaster card registers, DSP commands and mixer layout, used by the
 * /dev/dsp driver (audio-sb.c).  Compiled-in port/IRQ/DMA defaults are the
 * SB_XXX defines in arch/ports.h.
 */

#ifndef __ARCH_AUDIO_SB_H
#define __ARCH_AUDIO_SB_H

#include <arch/ports.h>     /* SB_XXX defaults and struct isa_conf */
#include <linuxmt/init.h>   /* INITPROC, FARPROC */

/* Driver private bits in the flags field of sb= in /bootopts */
#define ISAF_EXTWRITE   0x0001      /* set the 8237 Extended Write strobe */

/* DSP register offsets from the card base address */
#define SB_RESET        0x06        /* w  write 1 then 0 to reset the DSP */
#define SB_READ_DATA    0x0A        /* r  DSP data */
#define SB_WRITE_DATA   0x0C        /* rw bit 7 of read = write not ready */
#define SB_READ_STATUS  0x0E        /* r  bit 7 = data available, acks IRQ */
#define SB_MIXER_ADDR   0x04        /* w  mixer register select */
#define SB_MIXER_DATA   0x05        /* rw mixer register value */

/* DSP commands used by the driver */
#define DSP_SET_RATE    0x40        /* followed by the time constant */
#define DSP_DMA_OUT_8   0x14        /* followed by length-1, single block */
#define DSP_DMA_BLKSIZE 0x48        /* followed by block-1, auto-init IRQ size */
#define DSP_DMA_OUT_8AI 0x1C        /* 8-bit auto-init output, needs DSP 2.00+ */
#define DSP_DMA_EXIT_AI 0xDA        /* leave auto-init after the current block */
#define DSP_HALT_DMA    0xD0
#define DSP_SPEAKER_ON  0xD1
#define DSP_SPEAKER_OFF 0xD3
#define DSP_GET_VERSION 0xE1
#define DSP_READY       0xAA        /* reset acknowledge byte */

/* Mixer registers, SB Pro layout, kept by the SB16 for compatibility */
#define SB_MIX_VOICE    0x04
#define SB_MIX_MIC      0x0A
#define SB_MIX_OUTFILT  0x0E        /* output mode: bit 1 stereo, bit 5 filter bypass */
#define SB_MIX_MASTER   0x22
#define SB_MIX_FM       0x26
#define SB_MIX_CD       0x28
#define SB_MIX_LINE     0x2E

#define SB_FILT_OFF     0x20        /* set = SB Pro output filter bypassed */

#ifndef __ASSEMBLER__
/* sb= and mad16= routes, parsed in init/main.c; indices in linuxmt/audio.h */
extern struct isa_conf audio_conf[];

/* audio-mad.c, only linked when CONFIG_AUDIO_MAD is set */
extern int  INITPROC mad16_early_init(unsigned int port, int irq, int dma);
extern void FARPROC mad16_restore_profile(void);
extern void FARPROC mad16_codec_fix_fmt(void);
#endif

#endif
