/*  -*-c-*-
 * File:		debug_disp.h
 * Date:		Sun Jan 30 18:41:56 2000
 * Author:		 (plattner)
 *  
 * Abstract:
 *      Debugging facility over i/o port 
 * Modifications:
 * 
 * $Log$
 * Revision 1.2  2002/02/24 17:29:00  rhw2
 * Fixed #else and #endif to not fox the bcc -ansi option. Reformatted scripts/Configure and scripts/Menuconfig to a consistent style. Removed ELKS dependency on the Linux kernel source being available.
 *
 * Revision 1.1  2000/02/02 17:54:18  plattner
 * First checkin
 *
 *
 */

#ifndef LX86_ARCH_DEBUG_DISP_H
#define LX86_ARCH_DEBUG_DISP_H

#define DEBUG_DISP_PORT   0x0280

#ifdef DEBUG_DISP
#define DDISP(v) outb_p(v, DEBUG_DISP_PORT)
#else
#define DDISP(v)
#endif

#endif
