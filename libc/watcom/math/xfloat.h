/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2024 The Open Watcom Contributors. All Rights Reserved.
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
* Description:  C/C++ run-time library floating-point definitions.
*
****************************************************************************/


#ifndef _XFLOAT_H_INCLUDED
#define _XFLOAT_H_INCLUDED

#include <stddef.h>     // for wchar_t
#include <float.h>      // for LDBL_DIG

#ifdef __cplusplus
extern "C" {
#endif

#if defined( __WATCOMC__ ) && defined( _M_IX86 )
 #define _LONG_DOUBLE_
#endif

#if defined( _M_I86 )
typedef unsigned long   u4;
typedef long            i4;
#else
typedef unsigned int    u4;
typedef int             i4;
#endif

typedef struct {                // This layout matches Intel 8087
  #ifdef _LONG_DOUBLE_
    unsigned long low_word;     // - low word of fraction
    unsigned long high_word;    // - high word of fraction
    unsigned short exponent;    // - exponent and sign
  #else                         // use this for all other 32-bit RISC
    union {
        double  value;          // - double value
        u4      word[2];        // - so we can access bits
    } u;
  #endif
} long_double;

typedef struct {                // Layout of IEEE 754 double (FD)
    union {
        double  value;          // - double value
        u4      word[2];        // - so we can access bits
    } u;
} float_double;

typedef struct {                // Layout of IEEE 754 single (FS)
    union {
        float   value;          // - double value
        u4      word;           // - so we can access bits
    } u;
} float_single;

enum    ldcvt_flags {
    FPCVT_NONE          = 0x0000,
    FPCVT_E_FMT         = 0x0001,   // 'E' format
    FPCVT_F_FMT         = 0x0002,   // 'F' format
    FPCVT_G_FMT         = 0x0004,   // 'G' format
    FPCVT_F_CVT         = 0x0008,   // __cvt routine format rules
    FPCVT_F_DOT         = 0x0010,   // always put '.' in result
    FPCVT_LONG_DOUBLE   = 0x0020,   // number is true long double
    FPCVT_NO_TRUNC      = 0x0040,   // always provide ndigits in buffer
    FPCVT_IN_CAPS       = 0x0080,   // alpha characters are output uppercased
    FPCVT_IS_INF_NAN    = 0x0100,   // number is inf/nan (output flag)
    FPCVT_A_FMT         = 0x0200,   // 'A' format
};

typedef struct cvt_info {
    int         ndigits;        // INPUT: number of digits
    int         scale;          // INPUT: FORTRAN scale factor
    int         flags;          // INPUT/OUTPUT: flags (see ldcvt_flags)
    int         expchar;        // INPUT: exponent character to use
    int         expwidth;       // INPUT/OUTPUT: number of exponent digits
    int         sign;           // OUTPUT: 0 => +ve; otherwise -ve
    int         decimal_place;  // OUTPUT: position of '.'
    int         n1;             // OUTPUT: number of leading characters
    int         nz1;            // OUTPUT: followed by this many '0's
    int         n2;             // OUTPUT: followed by these characters
    int         nz2;            // OUTPUT: followed by this many '0's
} CVT_INFO;

/*
 * Depending on the target, some functions expect near pointer arguments
 * to be pointing into the stack segment, while in other cases they must
 * point into the data segment.
 */
#if !defined( _M_IX86 ) || defined( __FLAT__ )
typedef long_double                                     *ld_stk_ptr;
typedef double                                          *dbl_stk_ptr;
typedef float                                           *flt_stk_ptr;
typedef char                                            *buf_stk_ptr;
typedef void                                            *i8_stk_ptr;
typedef void                                            *u8_stk_ptr;
#else
typedef long_double __based( __segname( "_STACK" ) )    *ld_stk_ptr;
typedef double      __based( __segname( "_STACK" ) )    *dbl_stk_ptr;
typedef float       __based( __segname( "_STACK" ) )    *flt_stk_ptr;
typedef char        __based( __segname( "_STACK" ) )    *buf_stk_ptr;
typedef void        __based( __segname( "_STACK" ) )    *i8_stk_ptr;
typedef void        __based( __segname( "_STACK" ) )    *u8_stk_ptr;
#endif

#if defined( __WATCOMC__ )
_WMRTLINK extern void __LDcvt(
                        long_double *pld,       // pointer to long_double
                        CVT_INFO    *cvt,       // conversion info
                        char        *buf );     // buffer
_WMRTLINK extern int __Strtold(
                        const char  *bufptr,
                        long_double *pld,
                        char        **endptr );
_WMRTLINK extern int __wStrtold(
                        const wchar_t *bufptr,
                        long_double *pld,
                        wchar_t     **endptr );
#endif
extern  int     __LDClass( long_double * );
extern  void    __ZBuf2LD( buf_stk_ptr, ld_stk_ptr );
extern  void    __ZXBuf2LD( buf_stk_ptr buf, ld_stk_ptr ld, int *exponent );
extern  void    _LDScale10x( ld_stk_ptr, int );
#ifdef _LONG_DOUBLE_
extern  void    __iLDFD( ld_stk_ptr, dbl_stk_ptr );
extern  void    __iLDFS( ld_stk_ptr, flt_stk_ptr );
extern  void    __iFDLD( dbl_stk_ptr, ld_stk_ptr );
extern  void    __iFSLD( flt_stk_ptr, ld_stk_ptr );
extern  i4      __LDI4( ld_stk_ptr );
extern  void    __I4LD( i4, ld_stk_ptr );
extern  void    __U4LD( u4, ld_stk_ptr);
//The 64bit types change depending on what's being built.
//(u)int64* (un)signed_64* don't seem suitable, and we use void* instead.
extern  void    __LDI8( ld_stk_ptr, i8_stk_ptr );
extern  void    __I8LD( i8_stk_ptr, ld_stk_ptr );
extern  void    __U8LD( u8_stk_ptr, ld_stk_ptr );
extern  void    __FLDA( ld_stk_ptr, ld_stk_ptr, ld_stk_ptr );
extern  void    __FLDS( ld_stk_ptr, ld_stk_ptr, ld_stk_ptr );
extern  void    __FLDM( ld_stk_ptr, ld_stk_ptr, ld_stk_ptr );
extern  void    __FLDD( ld_stk_ptr, ld_stk_ptr, ld_stk_ptr );
extern  int     __FLDC( ld_stk_ptr, ld_stk_ptr );
#endif

#ifdef __WATCOMC__
#if __WATCOMC__ < 1300
/* fix for OW 1.9 */
#define float_fixup float
#else
#define float_fixup __float
#endif
#if defined(__386__)
 #pragma aux __ZBuf2LD "*"  __parm __caller [__eax] [__edx]
 #if defined(__FPI__)
  extern unsigned __Get87CW(void);
  extern void __Set87CW(unsigned short);
  #pragma aux   __Get87CW = \
                    "push 0" \
        float_fixup "fstcw [esp]" \
        float_fixup "fwait" \
                    "pop eax" \
        __value [__eax]
  #pragma aux   __Set87CW = \
                    "push eax" \
        float_fixup "fldcw [esp]" \
                    "pop eax" \
        __parm __caller [__eax]
  #pragma aux   __FLDA = \
        float_fixup "fld tbyte ptr [eax]" \
        float_fixup "fld tbyte ptr [edx]" \
        float_fixup "faddp st(1),st" \
        float_fixup "fstp tbyte ptr [ebx]" \
        __parm __caller [__eax] [__edx] [__ebx]
  #pragma aux   __FLDS = \
        float_fixup "fld tbyte ptr [eax]" \
        float_fixup "fld tbyte ptr [edx]" \
        float_fixup "fsubp st(1),st" \
        float_fixup "fstp tbyte ptr [ebx]" \
        __parm __caller [__eax] [__edx] [__ebx]
  #pragma aux   __FLDM = \
        float_fixup "fld tbyte ptr [eax]" \
        float_fixup "fld tbyte ptr [edx]" \
        float_fixup "fmulp st(1),st" \
        float_fixup "fstp tbyte ptr [ebx]" \
        __parm __caller [__eax] [__edx] [__ebx]
  #pragma aux   __FLDD = \
        float_fixup "fld tbyte ptr [eax]" \
        float_fixup "fld tbyte ptr [edx]" \
        float_fixup "fdivp st(1),st" \
        float_fixup "fstp tbyte ptr [ebx]" \
        __parm __caller [__eax] [__edx] [__ebx]
  #pragma aux   __FLDC = \
        float_fixup "fld tbyte ptr [edx]" /* ST(1) */ \
        float_fixup "fld tbyte ptr [eax]" /* ST(0) */ \
        float_fixup "fcompp" /* compare ST(0) with ST(1) */ \
        float_fixup "fstsw  ax" \
                    "sahf" \
                    "sbb  edx,edx" \
                    "shl  edx,1" \
                    "shl  ah,2" \
                    "cmc" \
                    "adc  edx,0" \
                    /* edx will be -1,0,+1 if [eax] <, ==, > [edx] */\
        __parm __caller [__eax] [__edx] \
        __value [__edx]
  #pragma aux   __LDI4 = \
        float_fixup "fld tbyte ptr [eax]" \
                    "push  eax" \
                    "push  eax" \
        float_fixup "fstcw [esp]" \
        float_fixup "fwait" \
                    "pop eax" \
                    "push eax" \
                    "or ah,0x0c" \
                    "push eax" \
        float_fixup "fldcw [esp]" \
                    "pop eax" \
        float_fixup "fistp dword ptr 4[esp]" \
        float_fixup "fldcw [esp]" \
                    "pop   eax" \
                    "pop   eax" \
        __parm __caller [__eax] \
        __value [__eax]
  #pragma aux   __I4LD = \
                    "push  eax" \
        float_fixup "fild  dword ptr [esp]" \
                    "pop   eax" \
        float_fixup "fstp tbyte ptr [edx]" \
        __parm __caller [__eax] [__edx]
  #pragma aux   __U4LD = \
                    "push  0" \
                    "push  eax" \
        float_fixup "fild  qword ptr [esp]" \
                    "pop   eax" \
                    "pop   eax" \
        float_fixup "fstp tbyte ptr [edx]" \
        __parm __caller [__eax] [__edx]
  #pragma aux   __LDI8 = \
        float_fixup "fld tbyte ptr [eax]" \
                    "push  eax" \
                    "push  eax" \
        float_fixup "fstcw [esp]" \
        float_fixup "fwait" \
                    "pop eax" \
                    "push eax" \
                    "or ah,0x0c" \
                    "push eax" \
        float_fixup "fldcw [esp]" \
                    "pop eax" \
        float_fixup "fistp qword ptr [edx]" \
        float_fixup "fldcw [esp]" \
                    "pop   eax" \
                    "pop   eax" \
        __parm __caller [__eax] [__edx]
  #pragma aux   __I8LD = \
        float_fixup "fild  qword ptr [eax]" \
        float_fixup "fstp  tbyte ptr [edx]" \
        __parm __caller [__eax] [__edx]
  #pragma aux   __U8LD = \
                    "push  [eax+4]" \
                    "and   [esp],0x7fffffff" \
                    "push  [eax]" \
        float_fixup "fild  qword ptr [esp]" \
                    "push  [eax+4]" \
                    "and   [esp],0x80000000" \
                    "push  0" \
        float_fixup "fild  qword ptr [esp]" \
        float_fixup "fchs" \
        float_fixup "faddp st(1),st" \
        float_fixup "fstp  tbyte ptr [edx]" \
                    "lea   esp,[esp+16]" \
        __parm __caller [__eax] [__edx]
  #pragma aux   __iFDLD = \
        float_fixup "fld  qword ptr [eax]" \
        float_fixup "fstp tbyte ptr [edx]" \
        __parm __caller [__eax] [__edx]
  #pragma aux   __iFSLD = \
        float_fixup "fld  dword ptr [eax]" \
        float_fixup "fstp tbyte ptr [edx]" \
        __parm __caller [__eax] [__edx]
  #pragma aux   __iLDFD = \
        float_fixup "fld  tbyte ptr [eax]" \
        float_fixup "fstp qword ptr [edx]" \
        __parm __caller [__eax] [__edx]
  #pragma aux   __iLDFS = \
        float_fixup "fld  tbyte ptr [eax]" \
        float_fixup "fstp dword ptr [edx]" \
        __parm __caller [__eax] [__edx]
 #else  // floating-point calls
  #pragma aux   __FLDA  "*"  __parm __caller [__eax] [__edx] [__ebx]
  #pragma aux   __FLDS  "*"  __parm __caller [__eax] [__edx] [__ebx]
  #pragma aux   __FLDM  "*"  __parm __caller [__eax] [__edx] [__ebx]
  #pragma aux   __FLDD  "*"  __parm __caller [__eax] [__edx] [__ebx]
  #pragma aux   __LDI4  "*"  __parm __caller [__eax] __value [__eax]
  #pragma aux   __I4LD  "*"  __parm __caller [__eax] [__edx]
  #pragma aux   __U4LD  "*"  __parm __caller [__eax] [__edx]
  #pragma aux   __LDI8  "*"  __parm __caller [__eax] [__edx]
  #pragma aux   __I8LD  "*"  __parm __caller [__eax] [__edx]
  #pragma aux   __U8LD  "*"  __parm __caller [__eax] [__edx]
  #pragma aux   __iFDLD "*"  __parm __caller [__eax] [__edx]
  #pragma aux   __iFSLD "*"  __parm __caller [__eax] [__edx]
  #pragma aux   __iLDFD "*"  __parm __caller [__eax] [__edx]
  #pragma aux   __iLDFS "*"  __parm __caller [__eax] [__edx]
  #pragma aux   __FLDC  "*"  __parm __caller [__eax] [__edx] __value [__eax]
 #endif
#elif defined( _M_I86 )            // 16-bit pragmas
 #pragma aux     __ZBuf2LD      "*"  __parm __caller [__ax] [__dx]
 #if defined(__FPI__)
  extern unsigned __Get87CW(void);
  extern void __Set87CW(unsigned short);
  #pragma aux   __Get87CW = \
                    "push ax" \
                    "push bp" \
                    "mov  bp,sp" \
        float_fixup "fstcw 2[bp]" \
        float_fixup "fwait" \
                    "pop bp" \
                    "pop ax" \
        __value [__ax]
  #pragma aux   __Set87CW = \
                    "push ax" \
                    "push bp" \
                    "mov  bp,sp" \
        float_fixup "fldcw 2[bp]" \
                    "pop bp" \
                    "pop ax" \
        __parm __caller [__ax]
  #pragma aux   __FLDA = \
                    "push bp" \
                    "mov  bp,ax" \
        float_fixup "fld  tbyte ptr [bp]" \
                    "mov  bp,dx" \
        float_fixup "fld  tbyte ptr [bp]" \
        float_fixup "faddp st(1),st" \
                    "mov  bp,bx" \
        float_fixup "fstp tbyte ptr [bp]" \
                    "pop  bp" \
        __parm __caller [__ax] [__dx] [__bx]
  #pragma aux   __FLDS = \
                    "push bp" \
                    "mov  bp,ax" \
        float_fixup "fld  tbyte ptr [bp]" \
                    "mov  bp,dx" \
        float_fixup "fld  tbyte ptr [bp]" \
        float_fixup "fsubp st(1),st" \
                    "mov  bp,bx" \
        float_fixup "fstp tbyte ptr [bp]" \
                    "pop  bp" \
        __parm __caller [__ax] [__dx] [__bx]
  #pragma aux   __FLDM = \
                    "push bp" \
                    "mov  bp,ax" \
        float_fixup "fld  tbyte ptr [bp]" \
                    "mov  bp,dx" \
        float_fixup "fld  tbyte ptr [bp]" \
        float_fixup "fmulp st(1),st" \
                    "mov  bp,bx" \
        float_fixup "fstp tbyte ptr [bp]" \
                    "pop  bp" \
        __parm __caller [__ax] [__dx] [__bx]
  #pragma aux   __FLDD = \
                    "push bp" \
                    "mov  bp,ax" \
        float_fixup "fld  tbyte ptr [bp]" \
                    "mov  bp,dx" \
        float_fixup "fld  tbyte ptr [bp]" \
        float_fixup "fdivp st(1),st" \
                    "mov  bp,bx" \
        float_fixup "fstp tbyte ptr [bp]" \
                    "pop  bp" \
        __parm __caller [__ax] [__dx] [__bx]
  #pragma aux   __FLDC = \
                    "push bp" \
                    "mov  bp,dx" \
        float_fixup "fld  tbyte ptr [bp]" /* ST(1) */\
                    "mov  bp,ax" \
        float_fixup "fld  tbyte ptr [bp]" /* ST(0) */\
        float_fixup "fcompp" /* compare ST(0) with ST(1) */\
                    "push ax" \
                    "mov  bp,sp" \
        float_fixup "fstsw 0[bp]" \
        float_fixup "fwait" \
                    "pop  ax" \
                    "sahf" \
                    "sbb  dx,dx" \
                    "shl  dx,1" \
                    "shl  ah,1" \
                    "shl  ah,1" \
                    "cmc" \
                    "adc  dx,0" \
                    "pop  bp" \
        __parm __caller [__ax] [__dx] \
        __value [__dx]
  #pragma aux   __LDI4 = \
                    "push  bp" \
                    "mov   bp,ax" \
        float_fixup "fld   tbyte ptr [bp]" \
                    "push  dx" \
                    "push  ax" \
                    "push  ax" \
                    "mov   bp,sp" \
        float_fixup "fstcw [bp]" \
        float_fixup "fwait" \
                    "pop   ax" \
                    "push  ax" \
                    "or    ah,0x0c" \
                    "mov   2[bp],ax" \
        float_fixup "fldcw 2[bp]" \
        float_fixup "fistp dword ptr 2[bp]" \
        float_fixup "fldcw [bp]" \
                    "pop   ax" \
                    "pop   ax" \
                    "pop   dx" \
                    "pop   bp" \
        __parm __caller [__ax] \
        __value [__dx __ax]
  #pragma aux   __I4LD = \
                    "push  bp" \
                    "push  dx" \
                    "push  ax" \
                    "mov   bp,sp" \
        float_fixup "fild  dword ptr [bp]" \
                    "pop   ax" \
                    "pop   dx" \
                    "mov   bp,bx" \
        float_fixup "fstp  tbyte ptr [bp]" \
                    "pop   bp" \
        __parm __caller [__dx __ax] [__bx]
  #pragma aux   __U4LD = \
                    "push  bp" \
                    "push  ax" \
                    "push  ax" \
                    "push  dx" \
                    "push  ax" \
                    "mov   bp,sp" \
                    "sub   ax,ax" \
                    "mov   4[bp],ax" \
                    "mov   6[bp],ax" \
        float_fixup "fild  qword ptr [bp]" \
                    "pop   ax" \
                    "pop   dx" \
                    "pop   bp" \
                    "pop   bp" \
                    "mov   bp,bx" \
        float_fixup "fstp  tbyte ptr [bp]" \
                    "pop   bp" \
        __parm __caller [__dx __ax] [__bx]
  #pragma aux   __LDI8 = \
                    "push  bp" \
                    "mov   bp,ax" \
        float_fixup "fld   tbyte ptr [bp]" \
                    "push  ax" \
                    "push  ax" \
                    "mov   bp,sp" \
        float_fixup "fstcw [bp]" \
        float_fixup "fwait" \
                    "pop   ax" \
                    "push  ax" \
                    "or    ah,0x0c" \
                    "push  ax" \
        float_fixup "fldcw [bp-2]" \
                    "pop   ax" \
                    "mov   bp,dx" \
        float_fixup "fistp qword ptr [bp]" \
                    "mov   bp,sp" \
        float_fixup "fldcw [bp]" \
                    "pop   ax" \
                    "pop   ax" \
                    "pop   bp" \
        __parm __caller [__ax] [__dx]
  #pragma aux   __I8LD = \
                    "push  bp" \
                    "mov   bp,ax" \
        float_fixup "fild  qword ptr [bp]" \
                    "mov   bp,dx" \
        float_fixup "fstp  tbyte ptr [bp]" \
                    "pop   bp" \
        __parm __caller [__ax] [__dx]
  #pragma aux   __U8LD = \
                    "push  bp" \
                    "mov   bp,ax" \
                    "push  6[bp]" \
                    "push  4[bp]" \
                    "push  2[bp]" \
                    "push  [bp]" \
                    "push  6[bp]" \
                    "xor   bp,bp" \
                    "push  bp" \
                    "push  bp" \
                    "push  bp" \
                    "mov   bp,sp" \
                    "and   byte ptr [bp+8+7],0x7f" \
        float_fixup "fild  qword ptr [bp+8]" \
                    "and   word ptr [bp+6],0x8000" \
        float_fixup "fild  qword ptr [bp]" \
        float_fixup "fchs" \
        float_fixup "faddp st(1),st" \
                    "mov   bp,dx" \
        float_fixup "fstp  tbyte ptr [bp]" \
                    "add   sp,16" \
                    "pop   bp" \
        __parm __caller [__ax] [__dx]
  #pragma aux   __iFDLD = \
                    "push  bp" \
                    "mov   bp,ax" \
        float_fixup "fld   qword ptr [bp]" \
                    "mov   bp,dx" \
        float_fixup "fstp  tbyte ptr [bp]" \
                    "pop   bp" \
        __parm __caller [__ax] [__dx]
  #pragma aux   __iFSLD = \
                    "push  bp" \
                    "mov   bp,ax" \
        float_fixup "fld   dword ptr [bp]" \
                    "mov   bp,dx" \
        float_fixup "fstp  tbyte ptr [bp]" \
                    "pop   bp" \
        __parm __caller [__ax] [__dx]
  #pragma aux   __iLDFD = \
                    "push  bp" \
                    "mov   bp,ax" \
        float_fixup "fld   tbyte ptr [bp]" \
                    "mov   bp,dx" \
        float_fixup "fstp  qword ptr [bp]" \
                    "pop   bp" \
        __parm __caller [__ax] [__dx]
  #pragma aux   __iLDFS = \
                    "push  bp" \
                    "mov   bp,ax" \
        float_fixup "fld   tbyte ptr [bp]" \
                    "mov   bp,dx" \
        float_fixup "fstp  dword ptr [bp]" \
                    "pop   bp" \
        __parm __caller [__ax] [__dx]
 #else  // floating-point calls
  #pragma aux   __FLDA  "*"  __parm __caller [__ax] [__dx] [__bx]
  #pragma aux   __FLDS  "*"  __parm __caller [__ax] [__dx] [__bx]
  #pragma aux   __FLDM  "*"  __parm __caller [__ax] [__dx] [__bx]
  #pragma aux   __FLDD  "*"  __parm __caller [__ax] [__dx] [__bx]
  #pragma aux   __LDI4  "*"  __parm __caller [__ax] __value [__dx __ax]
  #pragma aux   __I4LD  "*"  __parm __caller [__dx __ax] [__bx]
  #pragma aux   __U4LD  "*"  __parm __caller [__dx __ax] [__bx]
  #pragma aux   __LDI8  "*"  __parm __caller [__ax] [__dx]
  #pragma aux   __I8LD  "*"  __parm __caller [__ax] [__dx]
  #pragma aux   __U8LD  "*"  __parm __caller [__ax] [__dx]
  #pragma aux   __iFDLD "*"  __parm __caller [__ax] [__dx]
  #pragma aux   __iFSLD "*"  __parm __caller [__ax] [__dx]
  #pragma aux   __iLDFD "*"  __parm __caller [__ax] [__dx]
  #pragma aux   __iLDFS "*"  __parm __caller [__ax] [__dx]
  #pragma aux   __FLDC  "*"  __parm __caller [__ax] [__dx] __value [__ax]
 #endif
#endif
#endif

/*
 * define number of significant digits for long double numbers (80-bit)
 * it will be defined in float.h as soon as OW support long double
 * used in mathlib/c/ldcvt.c
 */
#ifdef _LONG_DOUBLE_
#if LDBL_DIG == 15
#undef LDBL_DIG
#define LDBL_DIG        19
#else
#error LDBL_DIG has changed from 15
#endif
#endif

/*
 * floating point conversion buffer length definition
 * is defined in lib_misc/h/cvtbuf.h
 * used by various floating point conversion routines
 * used in clib/startup/c/cvtbuf.c, lib_misc/h/thread.h
 * and mathlib/c/efcvt.c
 * it must be equal maximum FP precision ( LDBL_DIG )
 * hold lib_misc/h/cvtbuf.h in sync with LDBL_DIG
 */
#ifdef __cplusplus
};
#endif
#endif
