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


#ifndef _CPLX_H_INCLUDED
#define _CPLX_H_INCLUDED

#include <math.h>
#include "watcom.h"


typedef    signed_8     logstar1;       // 8-bit logical
typedef    signed_32    logstar4;       // 32-bit logical
typedef    signed_8     intstar1;       // 8-bit integer
typedef    signed_16    intstar2;       // 16-bit integer
typedef    signed_32    intstar4;       // 32-bit integer

#ifndef __cplusplus

typedef    float        real;
typedef    real         single;         // single precision
typedef    long double  extended;       // extended precision

typedef struct {         // single precision complex
    single      realpart;
    single      imagpart;
} complex;

typedef struct {        // extended precision complex
    extended    realpart;
    extended    imagpart;
} xcomplex;

#endif

typedef struct {        // double precision complex
    double      realpart;
    double      imagpart;
} dcomplex;

#define _NO_EXT_KEYS
#define __NO_MATH_OPS

#ifdef __cplusplus
extern "C" {
#endif

_WMRTLINK extern void       __rterrmsg( const int, const char * );
_WMRTLINK extern dcomplex   __qmath1err( unsigned int, dcomplex * );
_WMRTLINK extern dcomplex   __qmath2err( unsigned int, dcomplex *, dcomplex * );
_WMRTLINK extern complex    __zmath1err( unsigned int, complex * );
_WMRTLINK extern complex    __zmath2err( unsigned int, complex *, complex * );

_WMRTLINK extern dcomplex   _IF_C16Div( double r1, double i1, double r2, double i2 );
_WMRTLINK extern dcomplex   _IF_C16Mul( double r1, double i1, double r2, double i2 );
_WMRTLINK extern dcomplex   _IF_C16Pow( double base_r, double base_i, double power_r, double power_i );
_WMRTLINK extern dcomplex   _IF_C16PowI( double a, double b, intstar4 i );

_WMRTLINK extern double     _IF_CDABS( double r, double i );
_WMRTLINK extern dcomplex   _IF_CDCOS( double r, double i );
_WMRTLINK extern dcomplex   _IF_CDEXP( double r, double i );
_WMRTLINK extern dcomplex   _IF_CDLOG( double r, double i );
_WMRTLINK extern dcomplex   _IF_CDSIN( double r, double i );
_WMRTLINK extern dcomplex   _IF_CDSQRT( double r, double i );

#ifndef __cplusplus

_WMRTLINK extern intstar4   _IF_powii( intstar4 base, intstar4 power );
_WMRTLINK extern double     _IF_PowRI( double base, intstar4 power );
_WMRTLINK extern double     _IF_PowRR( double base, double power );
_WMRTLINK extern extended   _IF_PowXI( extended base, intstar4 power );

_WMRTLINK extern complex    _IF_C8Div( single r1, single i1, single r2, single i2 );
_WMRTLINK extern xcomplex   _IF_C32Div( extended r1, extended i1, extended r2, extended i2 );
_WMRTLINK extern complex    _IF_C8Mul( single r1, single i1, single r2, single i2 );
_WMRTLINK extern xcomplex   _IF_C32Mul( extended r1, extended i1, extended r2, extended i2 );
_WMRTLINK extern complex    _IF_C8Pow( single base_r, single base_i, single power_r, single power_i );
_WMRTLINK extern xcomplex   _IF_C32Pow( extended base_r, extended base_i, extended power_r, extended power_i );
_WMRTLINK extern complex    _IF_C8PowI( single a, single b, intstar4 i );
_WMRTLINK extern xcomplex   _IF_C32PowI( extended a, extended b, intstar4 i );

#endif

#ifdef __cplusplus
};
#endif

#define DACOS( x )      acos( x )
#define DASIN( x )      asin( x )
#define DATAN( x )      atan( x )
#define DATAN2( x, y )  atan2( x, y )
#define DCOS( x )       cos( x )
#define DCOSH( x )      cosh( x )
#define DEXP( x )       exp( x )
#define DLOG( x )       log( x )
#define DSIN( x )       sin( x )
#define DSINH( x )      sinh( x )
#define DTAN( x )       tan( x )
#define DTANH( x )      tanh( x )

#define ACOS( x )      acos( x )
#define ASIN( x )      asin( x )
#define ATAN( x )      atan( x )
#define ATAN2( x, y )  atan2( x, y )
#define COS( x )       cos( x )
#define COSH( x )      cosh( x )
#define EXP( x )       exp( x )
#define LOG( x )       log( x )
#define SIN( x )       sin( x )
#define SINH( x )      sinh( x )
#define TAN( x )       tan( x )
#define TANH( x )      tanh( x )

#if defined( __386__ )
  #pragma aux rt_rtn __parm __routine [__eax __ebx __ecx __edx __8087]
  #if defined( __SW_3S )
    #if defined( __FLAT__ )
      #pragma aux (rt_rtn) rt_rtn __modify [__8087 __gs]
    #else
      #pragma aux (rt_rtn) rt_rtn __modify [__8087 __es __gs]
    #endif
    #if defined( __FPI__ )
      #pragma aux (rt_rtn) flt_rt_rtn __value [__8087]
    #else
      #pragma aux (rt_rtn) flt_rt_rtn;
    #endif
  #else
    #pragma aux (rt_rtn) flt_rt_rtn;
  #endif
#elif defined( _M_I86 )
  #pragma aux rt_rtn __parm [__ax __bx __cx __dx __8087]
  #pragma aux (rt_rtn) flt_rt_rtn;
#endif

#if defined(_M_IX86)
  #pragma aux (rt_rtn) _IF_powii "IF@PowII";
  #pragma aux (flt_rt_rtn) _IF_PowRR "IF@PowRR";
  #pragma aux (flt_rt_rtn) _IF_PowRI "IF@PowRI";
  #pragma aux (flt_rt_rtn) _IF_PowXI "IF@PowXI";
  #pragma aux (flt_rt_rtn) _IF_CDABS "IF@CDABS";
  #pragma aux (rt_rtn) _IF_CDCOS "IF@CDCOS";
  #pragma aux (rt_rtn) _IF_CDEXP "IF@CDEXP";
  #pragma aux (rt_rtn) _IF_CDLOG "IF@CDLOG";
  #pragma aux (rt_rtn) _IF_CDSIN "IF@CDSIN";
  #pragma aux (rt_rtn) _IF_CDSQRT "IF@CDSQRT";
  #pragma aux (rt_rtn) _IF_C8Mul "IF@C8Mul";
  #pragma aux (rt_rtn) _IF_C16Mul "IF@C16Mul";
  #pragma aux (rt_rtn) _IF_C32Mul "IF@C32Mul";
  #pragma aux (rt_rtn) _IF_C8Div "IF@C8Div";
  #pragma aux (rt_rtn) _IF_C16Div "IF@C16Div";
  #pragma aux (rt_rtn) _IF_C32Div "IF@C32Div";
  #pragma aux (rt_rtn) _IF_C8Pow "IF@C8Pow";
  #pragma aux (rt_rtn) _IF_C16Pow "IF@C16Pow";
  #pragma aux (rt_rtn) _IF_C32Pow "IF@C32Pow";
  #pragma aux (rt_rtn) _IF_C8PowI "IF@C8PowI";
  #pragma aux (rt_rtn) _IF_C16PowI "IF@C16PowI";
  #pragma aux (rt_rtn) _IF_C32PowI "IF@C32PowI";
#endif
#endif
