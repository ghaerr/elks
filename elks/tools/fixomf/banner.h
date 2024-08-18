/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2023 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  Open Watcom banner strings and version defines.
*
****************************************************************************/


#define _BETAVER            1
#define _OWURL              "https://github.com/open-watcom/open-watcom-v2#readme"

#define _DOSTR( p )         #p
#define _MACROSTR( p )      _DOSTR( p )

#ifndef _BETASTR_
#define _BETASTR_           "beta"
#endif

#ifdef _BETAVER
#define _BETA_              " " _BETASTR_
#else
#define _BETA_
#endif

#ifdef _BETAVER
#define _BANEXTRA           __DATE__ " " __TIME__ STR_BITNESS
#else
#define _BANEXTRA           __DATE__ STR_BITNESS
#endif

#define banner1t(p)         "Open Watcom " p
#define banner1v(v)         "Version " v " " _BANEXTRA

#define banner1(p,v)        p " " banner1v(v)
#define banner1w(p,v)       banner1t(p) " " banner1v(v)

#define banner21            "Copyright (c) 2002-" _MACROSTR( _CYEAR ) " The Open Watcom Contributors."
#define banner21a(year)     "Portions Copyright (c) " _DOSTR( year ) "-2002 Sybase, Inc."

#define banner2             banner21 " All Rights Reserved."
#define banner2a(year)      banner21a(year) " All Rights Reserved."

#define banner3             "Source code is available under the Sybase Open Watcom Public License."
#define banner3a            "See " _OWURL " for details."

#define banner1ps(p,v)      "Powersoft " p " " banner1v(v)
#define banner2ps           banner21a( 1984 )
#define banner3ps           "All rights reserved.  Powersoft is a trademark of Sybase, Inc."

#if defined( _M_I86 )
  #define STR_BITNESS " (16-bit)"
#elif defined( _M_IX86 )
 #if defined( __WINDOWS__ )
  #define STR_BITNESS " (32-bit Extender)"
 #else
  #define STR_BITNESS " (32-bit)"
 #endif
#elif defined( _M_X64 )
  #define STR_BITNESS " (64-bit)"
#elif defined( __i386__ ) || defined( __i386 )
  #define STR_BITNESS " (32-bit)"
#elif defined( __AMD64__ ) || defined( __amd64 )
  #define STR_BITNESS " (64-bit)"
#else
  #define STR_BITNESS
#endif

// the following macros define the delimeters used by the resource
// compiler when concatenating strings
#define _RC_DELIM_LEFT_         [
#define _RC_DELIM_RIGHT_        ]

#if _BLDVER == 1200
    #define BAN_VER_STR "1.0" _BETA_
#elif _BLDVER == 1210
    #define BAN_VER_STR "1.1" _BETA_
#elif _BLDVER == 1220
    #define BAN_VER_STR "1.2" _BETA_
#elif _BLDVER == 1230
    #define BAN_VER_STR "1.3" _BETA_
#elif _BLDVER == 1240
    #define BAN_VER_STR "1.4" _BETA_
#elif _BLDVER == 1250
    #define BAN_VER_STR "1.5" _BETA_
#elif _BLDVER == 1260
    #define BAN_VER_STR "1.6" _BETA_
#elif _BLDVER == 1270
    #define BAN_VER_STR "1.7" _BETA_
#elif _BLDVER == 1280
    #define BAN_VER_STR "1.8" _BETA_
#elif _BLDVER == 1290
    #define BAN_VER_STR "1.9" _BETA_
#elif _BLDVER == 1300
    #define BAN_VER_STR "2.0" _BETA_
#elif _BLDVER == 1310
    #define BAN_VER_STR "2.1" _BETA_
#else
    #error **** Specified Banner version not supported ****
    #define BAN_VER_STR "12.0" _BETA_
#endif

/* these should all be _BLDVER/100.0 */
#define _WCC_VERSION_           BAN_VER_STR
#define _WPP_VERSION_           BAN_VER_STR
#define _WCL_VERSION_           BAN_VER_STR
#define _WFC_VERSION_           BAN_VER_STR
#define _WFL_VERSION_           BAN_VER_STR
#define _WLINK_VERSION_         BAN_VER_STR
#define _WMAKE_VERSION_         BAN_VER_STR
#define _WASM_VERSION_          BAN_VER_STR
#define _WASAXP_VERSION_        BAN_VER_STR
#define _WASPPC_VERSION_        BAN_VER_STR
#define _WASMIPS_VERSION_       BAN_VER_STR
/* these can be what ever they want to be */
#define _BPATCH_VERSION_        BAN_VER_STR
#define _MOUSEFIX_VERSION_      BAN_VER_STR
#define _XXXSERV_VERSION_       BAN_VER_STR
#define _RFX_VERSION_           BAN_VER_STR
#define _WD_VERSION_            BAN_VER_STR
#define _WBED_VERSION_          BAN_VER_STR
#define _WCEXP_VERSION_         BAN_VER_STR
#define _WHELP_VERSION_         BAN_VER_STR
#define _WDISASM_VERSION_       BAN_VER_STR
#define _FCENABLE_VERSION_      BAN_VER_STR
#define _MS2WLINK_VERSION_      BAN_VER_STR
#define _EXE2BIN_VERSION_       BAN_VER_STR
#define _WLIB_VERSION_          BAN_VER_STR
#define _WOMP_VERSION_          BAN_VER_STR
#define _WPROF_VERSION_         BAN_VER_STR
#define _WSAMP_VERSION_         BAN_VER_STR
#define _WSTRIP_VERSION_        BAN_VER_STR
#define _WTOUCH_VERSION_        BAN_VER_STR
#define _WBIND_VERSION_         BAN_VER_STR
#define _PERES_VERSION_         BAN_VER_STR
#define _EDBIND_VERSION_        BAN_VER_STR
#define _WBRW_VERSION_          BAN_VER_STR
#define _WBRG_VERSION_          BAN_VER_STR
#define _IDE_VERSION_           BAN_VER_STR
#define _RESEDIT_VERSION_       BAN_VER_STR
#define _WRC_VERSION_           BAN_VER_STR
#define _SPY_VERSION_           BAN_VER_STR
#define _HEAPWALKER_VERSION_    BAN_VER_STR
#define _DDESPY_VERSION_        BAN_VER_STR
#define _DRWATCOM_VERSION_      BAN_VER_STR
#define _DRNT_VERSION_          BAN_VER_STR
#define _ZOOM_VERSION_          BAN_VER_STR
#define _VI_VERSION_            BAN_VER_STR
#define _ASAXP_CLONE_VERSION_   BAN_VER_STR
#define _CL_CLONE_VERSION_      BAN_VER_STR
#define _CVTRES_CLONE_VERSION_  BAN_VER_STR
#define _LIB_CLONE_VERSION_     BAN_VER_STR
#define _LINK_CLONE_VERSION_    BAN_VER_STR
#define _NMAKE_CLONE_VERSION_   BAN_VER_STR
#define _RC_CLONE_VERSION_      BAN_VER_STR
#define _WIC_VERSION_           BAN_VER_STR
#define _WGML_VERSION_          BAN_VER_STR
#define _WIPFC_VERSION_         BAN_VER_STR

#define _DEFWIN_VERSION_        "1.0" _BETA_

/*
 * Java Tools
 */
#define JAVA_BAN_VER_STR        "1.0" _BETA_

#define _WJDUMP_VERSION_        JAVA_BAN_VER_STR
#define _JLIB_VERSION_          JAVA_BAN_VER_STR
#define _JAVAC_VERSION_         JAVA_BAN_VER_STR

/*
 * Versions of Microsoft tools with OW clones are compatible
 */
#define _MS_CL_VERSION_         "13.0.0"
#define _MS_LINK_VERSION_       "7.0.0"
