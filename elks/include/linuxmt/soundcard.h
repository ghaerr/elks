/*
 * OSS /dev/dsp ioctl definitions.
 *
 * This is the single definition of the sound ABI.  User programs reach it
 * through <sys/soundcard.h>, which is a __SYSINC__ shim onto this file, so
 * there is no second copy to drift out of sync.  Do not create one.
 *
 * The /dev/dsp driver plays raw unsigned 8-bit PCM through 8-bit ISA DMA
 * (8237 channel 1 or 3).  There is no recording path, no mixer, no MIDI and
 * no sequencer, so the ioctls for those are deliberately absent rather than
 * present and failing.  Command numbers match upstream OSS, so an ioctl that
 * is not implemented here returns -EINVAL under its usual number.
 */

#ifndef __LINUXMT_SOUNDCARD_H
#define __LINUXMT_SOUNDCARD_H

#include <linuxmt/types.h>

/*
 * An ELKS ioctl command is a 16-bit int, but the Linux OSS encoding puts its
 * direction bits and payload size above bit 15, where they cannot survive the
 * ELKS kernel ABI.  Keep only the low command word: the resulting numbers are
 * still the ones user programs pass to /dev/dsp.
 */
#define OSS__SIOC(x, y) \
    ((int)((((unsigned int)(x) & 0xffU) << 8) | ((unsigned int)(y) & 0xffU)))

typedef __s32 oss_int32_t;

/*
 * Implemented commands.  R/W marks the ones that read the caller's value and
 * write back the value actually accepted, which callers must re-read: SPEED in
 * particular is quantised by the Sound Blaster time constant.
 */
#define SNDCTL_DSP_RESET        OSS__SIOC('P', 0)   /* halt playback         */
#define SNDCTL_DSP_SYNC         OSS__SIOC('P', 1)   /* drain then halt       */
#define SNDCTL_DSP_SPEED        OSS__SIOC('P', 2)   /* R/W sample rate in Hz */
#define SNDCTL_DSP_STEREO       OSS__SIOC('P', 3)   /* R/W 0 = mono          */
#define SNDCTL_DSP_GETBLKSIZE   OSS__SIOC('P', 4)   /* R   DMA chunk bytes   */
#define SNDCTL_DSP_SETFMT       OSS__SIOC('P', 5)   /* R/W one AFMT_ value   */
#define SNDCTL_DSP_CHANNELS     OSS__SIOC('P', 6)   /* R/W channel count     */
#define SNDCTL_DSP_POST         OSS__SIOC('P', 8)   /* end of a short sound  */
#define SNDCTL_DSP_GETFMTS      OSS__SIOC('P', 11)  /* R   AFMT_ mask        */
#define SNDCTL_DSP_GETERROR     OSS__SIOC('P', 25)  /* R   struct audio_errinfo */

/* SNDCTL_DSP_HALT is the modern name for RESET; both are the same command. */
#define SNDCTL_DSP_HALT         SNDCTL_DSP_RESET

/* Names some OSS programs still use for the commands above. */
#define SOUND_PCM_RESET         SNDCTL_DSP_RESET
#define SOUND_PCM_SYNC          SNDCTL_DSP_SYNC
#define SOUND_PCM_WRITE_RATE    SNDCTL_DSP_SPEED
#define SOUND_PCM_WRITE_CHANNELS SNDCTL_DSP_CHANNELS
#define SOUND_PCM_WRITE_BITS    SNDCTL_DSP_SETFMT
#define SOUND_PCM_SETFMT        SNDCTL_DSP_SETFMT
#define SOUND_PCM_GETFMTS       SNDCTL_DSP_GETFMTS
#define SOUND_PCM_POST          SNDCTL_DSP_POST

/*
 * Sample formats.  The full set is listed because programs test for their
 * format by name and expect the symbol to exist; only AFMT_U8 is supported,
 * and SNDCTL_DSP_GETFMTS reports exactly that.
 */
#define AFMT_QUERY              0x00000000U /* ask, do not set */
#define AFMT_MU_LAW             0x00000001U
#define AFMT_A_LAW              0x00000002U
#define AFMT_IMA_ADPCM          0x00000004U
#define AFMT_U8                 0x00000008U /* the only supported format */
#define AFMT_S16_LE             0x00000010U
#define AFMT_S16_BE             0x00000020U
#define AFMT_S8                 0x00000040U
#define AFMT_U16_LE             0x00000080U
#define AFMT_U16_BE             0x00000100U

/*
 * Returned by SNDCTL_DSP_GETERROR.  play_underruns counts DMA blocks whose
 * completion deadline passed, which is the only way a program can tell that
 * audio broke up rather than played cleanly.  The remaining fields are kept
 * at their OSS names and reported as zero.
 */
struct audio_errinfo {
    oss_int32_t play_underruns;
    oss_int32_t rec_overruns;
    __u32       play_ptradjust;
    __u32       rec_ptradjust;
    oss_int32_t play_errorcount;
    oss_int32_t rec_errorcount;
    oss_int32_t play_lasterror;
    oss_int32_t rec_lasterror;
    oss_int32_t play_errorparm;
    oss_int32_t rec_errorparm;
    oss_int32_t filler[16];
};

typedef struct audio_errinfo audio_errinfo;

#endif
