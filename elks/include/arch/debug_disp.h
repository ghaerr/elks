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
 * Revision 1.1  2000/02/02 17:54:18  plattner
 * First checkin
 *
 *
 */

#ifndef __debug_disp_h__
#define __debug_disp_h__

#define DEBUG_DISP_PORT   0x0280

#ifdef DEBUG_DISP
#define DDISP(v) outb_p(v, DEBUG_DISP_PORT)
#else
#define DDISP(v)
#endif

#endif
