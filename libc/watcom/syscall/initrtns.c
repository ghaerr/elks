/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  Initialization and termination of clib.
*
* 16 Jul 2024 Port for ELKS by Greg Haerr
****************************************************************************/

#include <sys/rtinit.h>
#include <sys/cdefs.h>

#define FAR2NEAR(t,f)   ((t __wcnear *)(long)(f))

typedef void (__wcfar * __wcnear fpfn)(void);
typedef void (__wcnear * __wcnear npfn)(void);
typedef struct _rt_init __wcnear *struct_rt_init_ptr;

extern void save_dx( void );
#pragma aux save_dx = __modify __exact [__dx]
extern void save_ds( void );
#pragma aux save_ds = "push ds" __modify __exact [__sp]
extern void restore_ds( void );
#pragma aux restore_ds = "pop ds" __modify __exact [__sp]
#define save_es()
#define restore_es()
#define __GETDS()

#if defined(__SMALL__) || defined(__MEDIUM__)
static void callit_near( npfn *f )
{
    if( *f ) {
        save_dx();
        save_ds();
        (void)(**f)();
        restore_ds();
    }
}
#else
static void callit_far( fpfn *f )
{
    if( *f ) {
        save_ds();
        (void)(**f)();
        restore_ds();
    }
}
#endif

/*
; - takes priority limit parm in eax, code will run init routines whose
;       priority is < eax (really al [0-255])
;       eax==255 -> run all init routines
;       eax==15  -> run init routines whose priority is <= 15
;
*/
void __InitRtns(void)
{
    unsigned char local_limit;
    struct_rt_init_ptr  pnext;
    save_ds();
    save_es();
    __GETDS();

    local_limit = 255;
    for( ;; ) {
        {
            unsigned char working_limit;
            struct_rt_init_ptr  pcur;

            pcur = FAR2NEAR( struct _rt_init, &_Start_XI );
            pnext = FAR2NEAR( struct _rt_init, &_End_XI );
            working_limit = local_limit;

            // walk list of routines
            while( pcur < FAR2NEAR( struct _rt_init, &_End_XI ) ) {
                // if this one hasn't been called
                if( !pcur->done) {
                    // if the priority is better than best so far
                    if( pcur->priority <= working_limit ) {
                        // remember this one
                        pnext = pcur;
                        working_limit = pcur->priority;
                    }
                }
                // advance to next entry
                pcur++;
            }
            // check to see if all done, if we didn't find any
            // candidates then we can return
            if( pnext == FAR2NEAR( struct _rt_init, &_End_XI ) ) {
                break;
            }
        }
#if defined(__SMALL__) || defined(__MEDIUM__)
        callit_near( (npfn *)&pnext->func );
#else
        callit_far( (fpfn *)&pnext->func );
#endif
        // mark entry as invoked
        pnext->done = 1;
    }
    restore_es();
    restore_ds();
}

/*
; - takes priority range parms in eax, edx, code will run fini routines whose
;       priority is >= eax (really al [0-255]) and <= edx (really dl [0-255])
;       eax==0,  edx=255 -> run all fini routines
;       eax==16, edx=255 -> run fini routines in range 16..255
;       eax==16, edx=40  -> run fini routines in range 16..40
*/
void __FiniRtns(void)
{
    unsigned char local_min_limit;
    unsigned char local_max_limit;
    struct_rt_init_ptr  pnext;
    save_ds();
    save_es();
    __GETDS();

    local_min_limit = 0;
    local_max_limit = 255;
    for( ;; ) {
        {
            unsigned char working_limit;
            struct_rt_init_ptr  pcur;

            pcur = FAR2NEAR( struct _rt_init, &_Start_YI );
            pnext = FAR2NEAR( struct _rt_init, &_End_YI );
            working_limit = local_min_limit;

            // walk list of routines
            while( pcur < FAR2NEAR( struct _rt_init, &_End_YI ) ) {
                // if this one hasn't been called
                if( !pcur->done) {
                    // if the priority is better than best so far
                    if( pcur->priority >= working_limit ) {
                        // remember this one
                        pnext = pcur;
                        working_limit = pcur->priority;
                    }
                }
                // advance to next entry
                pcur++;
            }
            // check to see if all done, if we didn't find any
            // candidates then we can return
            if( pnext == FAR2NEAR( struct _rt_init, &_End_YI ) ) {
                break;
            }
        }
        if( pnext->priority <= local_max_limit ) {
#if defined(__SMALL__) || defined(__MEDIUM__)
            callit_near( (npfn *)&pnext->func );
#else
            callit_far( (fpfn *)&pnext->func );
#endif
        }
        // mark entry as invoked even if we don't call it
        // if we didn't call it, it is because we don't want to
        // call finirtns with priority > max_limit, in that case
        // marking the function as called, won't hurt anything
        pnext->done = 1;
    }
    restore_es();
    restore_ds();
}
