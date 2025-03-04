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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


/*
   Don't touch this file.  Last time we changed it, there were 3 of us,
   and we still messed it up.  If you don't believe me, go talk to Jack,
   John or Geno.
*/
#if defined(__AXP__)
// do nothing
#elif defined(__PPC__)
// do nothing
#elif defined(__MIPS__)
// do nothing
#elif defined(__386__)
    #pragma aux if_rtn __parm [__eax __ebx __ecx __edx __8087];
    #if defined(__SW_3S)
        #ifdef __FLAT__
            #pragma aux (if_rtn) if_rtn __modify [__gs];
        #else
            #pragma aux (if_rtn) if_rtn __modify [__gs __fs __es];
        #endif
        #if defined(__FPI__)
            #pragma aux (if_rtn) if_rtn __value [__8087];
        #endif
    #endif
#else
    #pragma aux if_rtn __parm [__ax __bx __cx __dx __8087];
#endif

_WMRTLINK extern float _IF_acos( float );
_WMRTLINK extern double _IF_dacos( double );
_WMRTLINK extern float _IF_asin( float );
_WMRTLINK extern double _IF_dasin( double );
_WMRTLINK extern float _IF_atan( float );
_WMRTLINK extern double _IF_datan( double );
_WMRTLINK extern float _IF_atan2( float, float );
_WMRTLINK extern double _IF_datan2( double, double );
_WMRTLINK extern float _IF_cosh( float );
_WMRTLINK extern double _IF_dcosh( double );
_WMRTLINK extern float _IF_exp( float );
_WMRTLINK extern double _IF_dexp( double );
_WMRTLINK extern float _IF_fabs( float );
_WMRTLINK extern double _IF_dfabs( double );
_WMRTLINK extern float _IF_fmod( float, float );
_WMRTLINK extern double _IF_dfmod( double, double );
_WMRTLINK extern float _IF_log( float );
_WMRTLINK extern double _IF_dlog( double );
_WMRTLINK extern float _IF_log10( float );
_WMRTLINK extern double _IF_dlog10( double );
_WMRTLINK extern float _IF_pow( float, float );
_WMRTLINK extern double _IF_dpow( double, double );
_WMRTLINK extern float _IF_sin( float );
_WMRTLINK extern double _IF_dsin( double );
_WMRTLINK extern float _IF_cos( float );
_WMRTLINK extern double _IF_dcos( double );
_WMRTLINK extern float _IF_tan( float );
_WMRTLINK extern double _IF_dtan( double );
_WMRTLINK extern float _IF_sinh( float );
_WMRTLINK extern double _IF_dsinh( double );
_WMRTLINK extern float _IF_sqrt( float );
_WMRTLINK extern double _IF_dsqrt( double );
_WMRTLINK extern float _IF_tanh( float );
_WMRTLINK extern double _IF_dtanh( double );

_WMRTLINK double __acosh( double x );
_WMRTLINK double __asinh( double x );
_WMRTLINK double __atanh( double x );
_WMRTLINK double __log2( double x );
_WMRTLINK long _IF_ipow( long base, long power );
_WMRTLINK extern float _IF_powi( float, long );
_WMRTLINK extern double _IF_dpowi( double, long );
//_WMRTLINK double _IF_PowRI( double base, intstar4 power );
//_WMRTLINK intstar4 _IF_powii( intstar4 base, intstar4 power );

#if defined(_M_IX86)
  #pragma aux (if_rtn) _IF_acos "IF@ACOS";
  #pragma aux (if_rtn) _IF_dacos "IF@DACOS";
  #pragma aux (if_rtn) _IF_asin "IF@ASIN";
  #pragma aux (if_rtn) _IF_dasin "IF@DASIN";
  #pragma aux (if_rtn) _IF_atan "IF@ATAN";
  #pragma aux (if_rtn) _IF_datan "IF@DATAN";
  #pragma aux (if_rtn) _IF_atan2 "IF@ATAN2";
  #pragma aux (if_rtn) _IF_datan2 "IF@DATAN2";
  #pragma aux (if_rtn) _IF_cosh "IF@COSH";
  #pragma aux (if_rtn) _IF_dcosh "IF@DCOSH";
  #pragma aux (if_rtn) _IF_exp "IF@EXP";
  #pragma aux (if_rtn) _IF_dexp "IF@DEXP";
  #pragma aux (if_rtn) _IF_fabs "IF@FABS";
  #pragma aux (if_rtn) _IF_dfabs "IF@DFABS";
  #pragma aux (if_rtn) _IF_fmod "IF@FMOD";
  #pragma aux (if_rtn) _IF_dfmod "IF@DFMOD";
  #pragma aux (if_rtn) _IF_log "IF@LOG";
  #pragma aux (if_rtn) _IF_dlog "IF@DLOG";
  #pragma aux (if_rtn) _IF_log10 "IF@LOG10";
  #pragma aux (if_rtn) _IF_dlog10 "IF@DLOG10";
  #pragma aux (if_rtn) _IF_pow "IF@POW";
  #pragma aux (if_rtn) _IF_dpow "IF@DPOW";
  #pragma aux (if_rtn) _IF_sin "IF@SIN";
  #pragma aux (if_rtn) _IF_dsin "IF@DSIN";
  #pragma aux (if_rtn) _IF_cos "IF@COS";
  #pragma aux (if_rtn) _IF_dcos "IF@DCOS";
  #pragma aux (if_rtn) _IF_tan "IF@TAN";
  #pragma aux (if_rtn) _IF_dtan "IF@DTAN";
  #pragma aux (if_rtn) _IF_sinh "IF@SINH";
  #pragma aux (if_rtn) _IF_dsinh "IF@DSINH";
  #pragma aux (if_rtn) _IF_sqrt "IF@SQRT";
  #pragma aux (if_rtn) _IF_dsqrt "IF@DSQRT";
  #pragma aux (if_rtn) _IF_tanh "IF@TANH";
  #pragma aux (if_rtn) _IF_dtanh "IF@DTANH";

  #pragma aux __acosh "IF@DACOSH";
  #pragma aux __asinh "IF@DASINH";
  #pragma aux __atanh "IF@DATANH";
  #pragma aux __log2 "IF@DLOG2";
#if defined(__386__)
  #pragma aux (if_rtn) _IF_ipow "IF@IPOW" __value [__eax];
#elif defined( _M_I86 )
  #pragma aux (if_rtn) _IF_ipow "IF@IPOW";
#endif
  #pragma aux (if_rtn) _IF_dpowi "IF@DPOWI";
  #pragma aux (if_rtn) _IF_powi "IF@POWI";
#endif

