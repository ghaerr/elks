#ifndef EXO_UTIL_ALREADY_INCLUDED
#define EXO_UTIL_ALREADY_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Copyright (c) 2008 - 2018 Magnus Lind.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software, alter it and re-
 * distribute it freely for any non-commercial, non-profit purpose subject to
 * the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must not
 *   claim that you wrote the original software. If you use this software in a
 *   product, an acknowledgment in the product documentation would be
 *   appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must not
 *   be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any distribution.
 *
 *   4. The names of this software and/or it's copyright holders may not be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 */

#include "log.h"
#include "buf.h"

/*
 * target is the basic token for the sys/call basic command
 * it may be -1 for hardcoded detection of a few targets.
 */
int find_sys(const unsigned char *buf, int target, int *stub_lenp);

enum file_type {UNKNOWN, RAW, ATARI_XEX, ORIC_TAP, APPLESINGLE,
                APPLESINGLE_SYS, BBC_INF, PRG};
struct load_info
{
    int basic_txt_start; /* in */
    int basic_var_start; /* out */
    int run; /* out */
    int start; /* out */
    int end; /* out */
    enum file_type type; /* out */
};

void load_located(const char *filename, unsigned char mem[65536],
                  struct load_info *info);

/*
 * returns 0 if the conversion was successful, 1 otherwise.
 *
 * if strp == NULL then str must be zero-terminated directly after the
 * value for the conversion to be considered successful.
 *
 *  if strp != NULL then on an successful conversion it will be set to
 * the character immediately following the last charachter in the
 * converted integer.
 */
int str_to_int(const char *str, int *value, const char **strp);

const char *fixup_appl(char *appl);

#ifdef __cplusplus
}
#endif
#endif
