#ifndef __LINUXMT_AUDIO_H
#define __LINUXMT_AUDIO_H

/*
 * /dev/dsp audio ioctl definitions.
 *
 * This is the single definition of the audio ABI.  User programs reach it
 * through <sys/audio.h>, which is a __SYSINC__ shim onto this file, so
 * there is no second copy to drift out of sync.
 *
 * The /dev/dsp driver plays raw unsigned 8-bit PCM through 8-bit ISA DMA
 * (8237 channel 1 or 3).  There is no recording path, no mixer, no MIDI and
 * no sequencer.  Command numbers match upstream OSS, so an ioctl that is
 * not implemented here returns -EINVAL under its usual number.
 *
 * Copyright (C) 2026 G Keet
 * Licensed under the GNU General Public License version 2, the same
 * terms as the ELKS kernel.
 * The ioctl numbers and encoding follow the Linux 2.x OSS sound interface.
 */

#include <linuxmt/types.h>

/*
 * An ELKS ioctl command is a 16-bit int, but the Linux OSS encoding puts its
 * direction bits and payload size above bit 15, where they cannot survive the
 * ELKS kernel ABI.  Keep only the low command word: the resulting numbers are
 * still the ones user programs pass to /dev/dsp.
 */
#define OSS__SIOC(x, y) \
    ((int)((((unsigned int)(x) & 0xffU) << 8) | ((unsigned int)(y) & 0xffU)))

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
#define SNDCTL_DSP_SETFMT       OSS__SIOC('P', 5)   /* R/W one DSP_FMT_ value */
#define SNDCTL_DSP_CHANNELS     OSS__SIOC('P', 6)   /* R/W channel count     */
#define SNDCTL_DSP_POST         OSS__SIOC('P', 8)   /* end of a short sound  */
#define SNDCTL_DSP_GETFMTS      OSS__SIOC('P', 11)  /* R   DSP_FMT_ mask     */
#define SNDCTL_DSP_GETERROR     OSS__SIOC('P', 25)  /* R   struct audio_errinfo */

/*
 * Sample formats, numbered as in OSS.  Unsigned 8-bit PCM is the only
 * supported format, and SNDCTL_DSP_GETFMTS reports exactly that.
 */
#define DSP_FMT_QUERY           0   /* SETFMT: ask, do not set */
#define DSP_FMT_U8              8

/*
 * Returned by SNDCTL_DSP_GETERROR.  play_underruns counts DMA blocks whose
 * completion deadline passed, which is the only way a program can tell that
 * audio broke up rather than played cleanly.  The other named fields are
 * kept at their OSS names, and they and the filler words are reported as
 * zero.
 */
struct audio_errinfo {
    __s32       play_underruns;
    __s32       rec_overruns;
    __u32       play_ptradjust;
    __u32       rec_ptradjust;
    __s32       play_errorcount;
    __s32       rec_errorcount;
    __s32       play_lasterror;
    __s32       rec_lasterror;
    __s32       play_errorparm;
    __s32       rec_errorparm;
    __s32       filler[16];
};

typedef struct audio_errinfo audio_errinfo;

#endif
