#ifndef ALREADY_INCLUDED_GETFLAG
#define ALREADY_INCLUDED_GETFLAG
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Copyright (c) 2002, 2003 Magnus Lind.
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

/**
 * Global state of the getflag function. Set this to 1 to start
 * the flag parsing from the beginning. (IN/OUT)
 */
extern int flagind;
/**
 * The current flag. (OUT)
 */
extern int flagflag;
/**
 * The argument of the current flag, NULL for flags without args. (OUT)
 */
extern const char *flagarg;

/**
 * Incrementally get flags from the given arguments.
 * Returns -1 if all flags have been returned.
 * Returns '?' if the flag is unknown. Check flagflag for the actual flag.
 */
int getflag(int argc, char *argv[], const char *flags);

#ifdef __cplusplus
}
#endif
#endif
