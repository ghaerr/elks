;*****************************************************************************
;*
;*                            Open Watcom Project
;*
;*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
;*
;*  ========================================================================
;*
;*    This file contains Original Code and/or Modifications of Original
;*    Code as defined in and that are subject to the Sybase Open Watcom
;*    Public License version 1.0 (the 'License'). You may not use this file
;*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
;*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
;*    provided with the Original Code and Modifications, and is also
;*    available at www.sybase.com/developer/opensource.
;*
;*    The Original Code and all software distributed under the License are
;*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
;*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
;*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
;*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
;*    NON-INFRINGEMENT. Please see the License for the specific language
;*    governing rights and limitations under the License.
;*
;*  ========================================================================
;*
;* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
;*               DESCRIBE IT HERE!
;*
;*****************************************************************************


;========================================================================
;==     Name:           I8DQ,I8DR, U8DQ,U8DR                           ==
;==     Operation:      8 byte divide quotient and remainder           ==
;==     Inputs:         AX:BX:CX:DX     Dividend                       ==
;==                     [SS:SI]         Divisor                        ==
;==     Outputs:        AX:BX:CX:DX     Quotient (DQ), Remainder (DR)  ==
;==     Volatile:       none                                           ==
;========================================================================
include mdef.inc
include struct.inc

        modstart        i8d086

; this is a simple translation of the following C++ program
; if there is a bug, extract out the C++ program and setup the
; values, run the program, and the execution results should agree
; as you trace along.
comment $
#include <iostream.h>
// SP&E Vol.24(6) 579-601 (June 1994)
// "Multiple-length Division Revisited: a Tour of the Minefield"
// Per Brinch Hansen
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma inline_depth(0)

template <unsigned w,unsigned b>
    struct MLDiv {
        unsigned d[w+1];
        MLDiv( void ) {
            memset( this, 0xFF, sizeof( *this ) );
        }
        MLDiv( unsigned *p ) {
            unsigned i;

            for( i = 0; i <= w; ++i ) {
                if( p[i] == b ) break;
                d[i] = p[i];
            }
            for( i = i; i <= w; ++i ) {
                d[i] = 0;
            }
        }
        MLDiv( unsigned long x ) {
            unsigned i;

            for( i = 0; i <= w; ++i ) {
                if( x == 0 ) break;
                d[i] = x % b;
                x /= b;
            }
            for( i = i; i <= w; ++i ) {
                d[i] = 0;
            }
        }
        static void fatal( char *m ) {
            fprintf( stderr, "'%s'\n", m );
            exit( EXIT_FAILURE );
        }
        static unsigned length( MLDiv const &r ) {
            unsigned i;

            i = w;
            while( i != 0 ) {
                if( r.d[i] != 0 ) break;
                --i;
            }
            cout << "length(" << r << ") " << (i+1) << endl;
            return( i + 1 );
        }
        static void zero( MLDiv &r ) {
            memset( r.d, 0, sizeof( r.d ) );
        }
        static void product( MLDiv &x, MLDiv const &y, unsigned k ) {
            unsigned i;
            unsigned m;
            unsigned carry;
            unsigned temp;

            m = length( y );
            zero( x );
            carry = 0;
            for( i = 0; i < m; ++i ) {
                temp = y.d[i] * k + carry;
                x.d[i] = temp % b;
                carry = temp / b;
            }
            if( m <= w ) {
                x.d[m] = carry;
            } else {
                if( carry != 0 ) {
                    fatal( "product overflow" );
                }
            }
            cout << "product: " << y << "*" << k << "=" << x << endl;
        }
        static unsigned quotient( MLDiv &x, MLDiv const &y, unsigned k ) {
            int i;
            unsigned m;
            unsigned carry;
            unsigned temp;

            assert( &x != &y );
            m = length( y );
            zero( x );
            carry = 0;
            for( i = m-1; i >= 0; --i ) {
                temp = carry * b + y.d[i];
                x.d[i] = temp / k;
                carry = temp % k;
            }
            return( carry );
            cout << "quotient: " << y << "/" << k << "=" << x << endl;
        }
        static MLDiv prefix( MLDiv const &r, unsigned m, unsigned n ) {
            MLDiv p;

            zero( p );
            while( n != 0 ) {
                p.d[ n-1 ] = r.d[ m ];
                --n;
                --m;
            }
            return( p );
        }
        static unsigned trial( MLDiv const &r, MLDiv const &d, unsigned k, unsigned m ) {
            unsigned d2;
            unsigned km;
            unsigned r3;
            unsigned x;

            assert( 2 <= m && m <= (k+m) && (k+m) <= w );
            km = k + m;
            r3 = ( r.d[km]*b + r.d[ km-1 ] ) * b + r.d[ km-2 ];
            d2 = d.d[ m-1 ]*b + d.d[ m - 2 ];
            x = r3 / d2;
            if( (b-1) < x ) {
                x = b-1;
            }
            cout << "trial: " << prefix( r, km, 3 ) << "/" << prefix( d, m-1, 2 ) << "=" << x << endl;
            return( x );
        }
        static unsigned smaller( MLDiv const &r, MLDiv const &dq, unsigned k, unsigned m ) {
            unsigned i;
            unsigned j;
            int ret;

            assert( k <= (k+m) && (k+m) <= w );
            i = m;
            while( i != 0 ) {
                if( r.d[i+k] != dq.d[i] ) break;
                --i;
            }
            ret = ( r.d[i+k] < dq.d[i] );
            cout << "smaller: " << prefix( r, k+m, m + 1 ) << "<" << dq << "=" << ret << endl;
            return( ret );
        }
        static void difference( MLDiv &r, MLDiv const &dq, unsigned k, unsigned m ) {
            unsigned borrow;
            int diff;
            unsigned i;
            MLDiv sr( prefix( r, m+k, m+1 ) );

            assert( k <= (k+m) && (k+m) <= w );
            cout << "difference: " << sr << "-" << dq << "=" << prefix( r, m+k, m+1 ) << endl;
            borrow = 0;
            for( i = 0; i <= m; ++i ) {
                diff = r.d[ i+k ] - dq.d[i] - borrow + b;
                r.d[ i+k ] = diff % b;
                borrow = 1 - diff / b;
            }
            if( borrow != 0 ) {
                fatal( "difference overflow" );
            }
            cout << "difference: " << sr << "-" << dq << "=" << prefix( r, m+k, m+1 ) << endl;
        }
        static void longdivide( MLDiv const &x, MLDiv const &y,
                                MLDiv &q, MLDiv &r,
                                unsigned n, unsigned m ) {
            MLDiv d;
            MLDiv dq;
            MLDiv tr;
            unsigned f;
            int k;
            unsigned qt;

            assert( 2 <= m && m <= n && n <= w );
            f = b / ( y.d[m-1] + 1 );
            product( tr, x, f );
            product( d, y, f );
            zero( q );
            for( k = n-m; k >= 0; --k ) {
                assert( 2 <= m && m <= (k+m) && (k+m) <= n && n <= w );
                qt = trial( tr, d, k, m );
                product( dq, d, qt );
                if( smaller( tr, dq, k, m ) ) {
                    --qt;
                    product( dq, d, qt );
                }
                q.d[k] = qt;
                difference( tr, dq, k, m );
            }
            quotient( r, tr, f );
        }
        static void division( MLDiv const &x, MLDiv const &y,
                              MLDiv &q, MLDiv &r ) {
            unsigned m;
            unsigned n;
            unsigned y1;

            m = length( y );
            if( m == 1 ) {
                y1 = y.d[ m - 1 ];
                if( y1 > 0 ) {
                    unsigned carry = quotient( q, x, y1 );
                    zero( r );
                    r.d[0] = carry;
                } else {
                    fatal( "divide by 0" );
                }
            } else {
                n = length( x );
                if( m > n ) {
                    zero( q );
                    r = x;
                } else {
                    assert( 2 <= m && m <= n && n <= w );
                    longdivide( x, y, q, r, n, m );
                }
            }
        }
        friend ostream & operator <<( ostream &o, MLDiv const &r ) {
            int i;
            int j;

            for( i = w-1; i > 0; --i ) {
                if( r.d[i] != 0 ) break;
            }
            for( i = i; i >= 0; --i ) {
                printf( "%02x", r.d[i] );
                j = i & 0x01;
                if( j == 0 && i != 0 ) putchar( '_' );
            }
            return o;
        }
    };
unsigned d1[] = {
    0x00, 0x04, 0x00, 0x30, 0xff, 0xff, 0xff, 0xff, 0x100
};
unsigned d2[] = {
    0xff, 0x01, 0x00, 0x00, 0x55, 0x02, 0x00, 0x00,  0x100
};

main() {
    MLDiv<18,256> q;
    MLDiv<18,256> r;
    MLDiv<18,256> v1(d1);
    MLDiv<18,256> v2(d2);
    v1.division( v1, v2, q, r );
    cout << v1 << "/" << v2 << " = " << q << " (remainder " << r << ")" << endl;
}
$

        xdefp   __U8DQ
        xdefp   __U8DR
        xdefp   __I8DQ
        xdefp   __I8DR
        xdefp   __U8DQE
        xdefp   __U8DRE
        xdefp   __I8DQE
        xdefp   __I8DRE

q               equ     -10
r               equ     -20
d               equ     -30
tq              equ     -40
tr              equ     -50
ld_n            equ     -52
ld_m            equ     -54
ld_f            equ     -56
ld_qt           equ     -58
ld_k            equ     -60
pr_k            equ     -62
qu_k            equ     -64
what_reqd       equ     -66             ; 0x01 - rem, 0x02 - quot
prolog_save     equ     -68
amt             equ     -68
x               equ     amt-10
y               equ     amt-20

; zero( ss:bx )
zero            label   near
                mov     word ptr ss:[bx],0
                mov     word ptr ss:2[bx],0
                mov     word ptr ss:4[bx],0
                mov     word ptr ss:6[bx],0
                mov     word ptr ss:8[bx],0
                ret

; copy( ss:di, ss:si )
copy            label   near
                push    ds
                push    es
                push    ss
                pop     es
                push    ss
                pop     ds
                movsw
                movsw
                movsw
                movsw
                pop     es
                pop     ds
                ret

; _length( ss:bx ) => al (range 1-10)
_length         label   near
                xor     ax,ax
                or      ax,ss:8[bx]
                jne     len_10_9
                or      ax,ss:6[bx]
                jne     len_8_7
                or      ax,ss:4[bx]
                jne     len_6_5
                or      ax,ss:2[bx]
                jne     len_4_3
                or      ax,ss:0[bx]
                jne     len_2_1
len_plus_1:     inc     al
                ret
len_10_9:       mov     al,9
                jmp     len_check
len_8_7:        mov     al,7
                jmp     len_check
len_6_5:        mov     al,5
                jmp     len_check
len_4_3:        mov     al,3
                jmp     len_check
len_2_1:        mov     al,1
len_check:      test    ah,ah
                jne     len_plus_1
                ret

; product( ss:di x, ss:si y, ax k )
product         label   near
                mov     pr_k[bp],ax
                mov     bx,di
                call    zero
                lea     bx,7[di]
                dec     di
                xor     cx,cx
prod_loop       label   near
                xor     ah,ah
                mov     al,ss:[si]
                inc     si
                test    ax,ax
                je      quick_mul
                mul     word ptr pr_k[bp]
quick_mul:      add     ax,cx
                inc     di
                mov     ss:[di],al
                mov     cl,ah
                cmp     di,bx
                jne     prod_loop
                inc     di
                mov     ss:[di],cl
                ret

; quotient( ss:di x, ss:si y, ax k ) dx = carry
quotient        label   near
                mov     qu_k[bp],ax
                mov     bx,di
                call    zero
                xor     dl,dl
                lea     di,8[di]
                lea     si,8[si]
quotient_loop   label   near
                mov     ah,dl
                dec     si
                xor     dx,dx
                mov     al,ss:[si]
                test    ax,ax
                je      quick_divide
                div     word ptr qu_k[bp]
quick_divide:   dec     di
                mov     ss:[di],al
                cmp     bx,di
                jne     quotient_loop
                ret

; trial( tr[bp], d[bp], ld_k[bp], ld_m[bp] ) ax
trial           label   near
                mov     si,ld_k[bp]
                add     si,ld_m[bp]
                xor     dx,dx
                mov     ax,tr-2[si+bp]
                mov     dl,tr[si+bp]
                mov     si,ld_m[bp]
                div     word ptr d-2[bp+si]
                test    ax,0ff00h
                je      trial_ok
                mov     ax,0ffh
trial_ok:       ret

; smaller( tr, tq, ld_k, ld_m ) ax
smaller         label   near
                mov     di,ld_m[bp]
smaller_loop:   test    di,di
                je      smaller_done
                mov     si,ld_k[bp]
                add     si,di
                mov     al,tq[bp+di]
                sub     al,tr[bp+si]
                jne     smaller_ret
                dec     di
                jmp     smaller_loop
smaller_done    label   near
                mov     si,ld_k[bp]
                add     si,di
                mov     al,tq[bp+di]
                sub     al,tr[bp+si]
smaller_ret:    ret

; difference( tr, tq, k, m )
difference      label   near
                xor     di,di
                xor     cl,cl
diff_loop:      cmp     di,ld_m[bp]
                jg      diff_done
                mov     si,ld_k[bp]
                add     si,di
                xor     ch,ch
                xor     ax,ax
                mov     al,tr[bp+si]
                sub     ch,cl
                sbb     al,tq[bp+di]
                mov     tr[bp+si],al
                sbb     cx,cx
                inc     di
                neg     cx
                jmp     diff_loop
diff_done       label   near
                ret

; subtract( ss:di o, ss:si e )  o -= e
subtract        label   near
                mov     ax,ss:[di]
                sub     ax,ss:[si]
                mov     ss:[di],ax
                mov     ax,ss:2[di]
                sbb     ax,ss:2[si]
                mov     ss:2[di],ax
                mov     ax,ss:4[di]
                sbb     ax,ss:4[si]
                mov     ss:4[di],ax
                mov     ax,ss:6[di]
                sbb     ax,ss:6[si]
                mov     ss:6[di],ax
                mov     ax,ss:8[di]
                sbb     ax,ss:8[si]
                mov     ss:8[di],ax
                ret

; longdivision( x[bp] x, y[bp] y, q[bp] q, r[bp] r, ld_n[bp] n, ld_m[bp] m )
longdivision    label   near
                mov     di,ld_m[bp]
                dec     di
                xor     bx,bx
                mov     bl,y[bp+di]
                inc     bx
                mov     ax,0100h
                xor     dx,dx
                div     bx
                mov     ld_f[bp],ax
                lea     di,tr[bp]
                lea     si,x[bp]
                call    product
                mov     ax,ld_f[bp]
                lea     di,d[bp]
                lea     si,y[bp]
                call    product
                lea     bx,q[bp]
                call    zero
                mov     cx,ld_n[bp]
                sub     cx,ld_m[bp]
ld_loop:        test    cx,cx
                jl      ld_done
                mov     ld_k[bp],cx
                call    trial
                mov     ld_qt[bp],ax
                lea     si,d[bp]
                lea     di,tq[bp]
                call    product
                call    smaller
                jbe     ld_not_smaller
                dec     word ptr ld_qt[bp]
                lea     si,d[bp]
                lea     di,tq[bp]
                call    subtract
ld_not_smaller  label   near
                mov     di,ld_k[bp]
                mov     ax,ld_qt[bp]
                mov     q[bp+di],al
                call    difference
                mov     cx,ld_k[bp]
                dec     cx
                jmp     ld_loop
ld_done         label   near
                test    byte ptr what_reqd[bp],1
                je      ld_ret
                mov     ax,ld_f[bp]
                lea     di,r[bp]
                lea     si,tr[bp]
                call    quotient
ld_ret:         ret

; negate( ss:bx )
negate          label   near
                xor     cx,cx
                mov     ax,cx
                sub     ax,ss:[bx]
                mov     ss:[bx],ax
                mov     ax,cx
                sbb     ax,ss:2[bx]
                mov     ss:2[bx],ax
                mov     ax,cx
                sbb     ax,ss:4[bx]
                mov     ss:4[bx],ax
                mov     ax,cx
                sbb     ax,ss:6[bx]
                mov     ss:6[bx],ax
                ret

idivision       label   near
                mov     ax,x+6[bp]
                test    ax,ax
                jl      x_neg
                mov     ax,y+6[bp]
                test    ax,ax
                jge     x_pos_y_pos
                lea     bx,y[bp]
                call    negate
                call    division        ; +ve / -ve = -ve rem +ve
                and     word ptr what_reqd[bp],2
                jmp     neg_both
x_neg:          mov     ax,y+6[bp]
                test    ax,ax
                jl      x_neg_y_neg
                lea     bx,x[bp]
                call    negate
                call    division        ; -ve / +ve = -ve rem -ve
                jmp     neg_both
x_neg_y_neg:    lea     bx,x[bp]
                call    negate
                lea     bx,y[bp]
                call    negate
                call    division        ; -ve / -ve = +ve rem -ve
                and     word ptr what_reqd[bp],1
                jmp     neg_both
x_pos_y_pos:    call    division        ; +ve / +ve = +ve rem +ve
                jmp     idiv_ret
neg_both        label   near
neg_quot:       test    word ptr what_reqd[bp],2
                je      neg_rem
                lea     bx,q[bp]
                call    negate
neg_rem:        test    word ptr what_reqd[bp],1
                je      idiv_ret
                lea     bx,r[bp]
                call    negate
idiv_ret:       ret

division        label   near
                push    si
                push    di
                lea     bx,y[bp]
                call    _length
                cmp     al,1
                jne     di_y_not_1
                mov     ax,y[bp]
                lea     di,q[bp]
                lea     si,x[bp]
                call    quotient
                test    byte ptr what_reqd[bp],1
                je      di_done
                lea     bx,r[bp]
                call    zero
                mov     r[bp],dx
                jmp     di_done
di_y_not_1      label   near
                mov     cx,ax
                lea     bx,x[bp]
                call    _length
                cmp     al,cl
                jge     di_x_ge_y
                lea     bx,q[bp]
                call    zero
                test    byte ptr what_reqd[bp],1
                je      di_done
                lea     si,x[bp]
                lea     di,r[bp]
                call    copy
                jmp     di_done
di_x_ge_y       label near
                xor     ch,ch
                mov     ld_m[bp],cx
                xor     ah,ah
                mov     ld_n[bp],ax
                call    longdivision
di_done         label   near
                pop     di
                pop     si
                ret

di_prolog_ss    label   near
                xchg    ax,x+8[bp]
                mov     prolog_save[bp],ax
                xor     ax,ax
                xchg    ax,x+8[bp]
                push    ax
                push    bx
                push    cx
                push    dx              ; x
                push    x+8[bp]
                push    ss:6[si]
                push    ss:4[si]
                push    ss:2[si]
                push    ss:0[si]        ; y
                push    prolog_save[bp]
                ret

di_prolog_es    label   near
                xchg    ax,x+8[bp]
                mov     prolog_save[bp],ax
                xor     ax,ax
                xchg    ax,x+8[bp]
                push    ax
                push    bx
                push    cx
                push    dx              ; x
                push    x+8[bp]
                push    es:6[si]
                push    es:4[si]
                push    es:2[si]
                push    es:0[si]        ; y
                push    prolog_save[bp]
                ret


; u8 / u8
; [dx cx bx ax] __U8DQ [dx cx bx ax] [ss:si]

defp            __U8DQ
                push    bp
                mov     bp,sp
                lea     sp,amt[bp]
                call    di_prolog_ss
                mov     word ptr what_reqd[bp],2
                call    division
                lea     sp,q[bp]
                jmp     pop_ret
endproc         __U8DQ

; u8 % u8
; [dx cx bx ax] __U8DR [dx cx bx ax] [ss:si]

defp            __U8DR
                push    bp
                mov     bp,sp
                lea     sp,amt[bp]
                call    di_prolog_ss
                mov     word ptr what_reqd[bp],1
                call    division
                lea     sp,r[bp]
                jmp     pop_ret
endproc         __U8DR

; i8 / i8
; [dx cx bx ax] __I8DQ [dx cx bx ax] [ss:si]

defp            __I8DQ
                push    bp
                mov     bp,sp
                lea     sp,amt[bp]
                call    di_prolog_ss
                mov     word ptr what_reqd[bp],2
                call    idivision
                lea     sp,q[bp]
pop_ret         label   near
                pop     dx
                pop     cx
                pop     bx
                pop     ax
                mov     sp,bp
                pop     bp
                ret
endproc         __I8DQ

; i8 % i8
; [dx cx bx ax] __I8DR [dx cx bx ax] [ss:si]

defp            __I8DR
                push    bp
                mov     bp,sp
                lea     sp,amt[bp]
                call    di_prolog_ss
                mov     word ptr what_reqd[bp],1
                call    idivision
                lea     sp,r[bp]
                jmp     pop_ret
endproc         __I8DR

; u8 / u8
; [dx cx bx ax] __U8DQE [dx cx bx ax] [es:si]

defp            __U8DQE
                push    bp
                mov     bp,sp
                lea     sp,amt[bp]
                call    di_prolog_es
                mov     word ptr what_reqd[bp],2
                call    division
                lea     sp,q[bp]
                jmp     pop_ret
endproc         __U8DQE

; u8 % u8
; [dx cx bx ax] __U8DRE [dx cx bx ax] [es:si]

defp            __U8DRE
                push    bp
                mov     bp,sp
                lea     sp,amt[bp]
                call    di_prolog_es
                mov     word ptr what_reqd[bp],1
                call    division
                lea     sp,r[bp]
                jmp     pop_ret
endproc         __U8DRE

; i8 / i8
; [dx cx bx ax] __I8DQE [dx cx bx ax] [es:si]

defp            __I8DQE
                push    bp
                mov     bp,sp
                lea     sp,amt[bp]
                call    di_prolog_es
                mov     word ptr what_reqd[bp],2
                call    idivision
                lea     sp,q[bp]
                jmp     pop_ret
endproc         __I8DQE

; i8 % i8
; [dx cx bx ax] __I8DRE [dx cx bx ax] [es:si]

defp            __I8DRE
                push    bp
                mov     bp,sp
                lea     sp,amt[bp]
                call    di_prolog_es
                mov     word ptr what_reqd[bp],1
                call    idivision
                lea     sp,r[bp]
                jmp     pop_ret
endproc         __I8DRE


        endmod
        end
