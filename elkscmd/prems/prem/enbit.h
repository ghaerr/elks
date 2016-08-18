/*
 * Funkcioj por enigi kodojn pobite
 *
 * ek_enbit()             /* iniciati enigon
 * en_lit()               /* enigi literalon, redonas 1,
 *                        /* se ne maksimume longa, alie 0
 * $Header$
 * $Log$
 */

#include "bit.h"
#include "enarbo.h"

extern char enb_bb; /* bita bufro */
extern int  enb_nb; /* montrilo de bito en la bufro */

#ifdef DIAP_KOD
extern unsigned int
den_Dia; /* la diapazono por diapazonigita unuara kodo */
extern unsigned int den_start;
#endif

#ifdef X_KOD
struct x_mkalk {
    unsigned int k; /* kalkulilo */
    LITER l; /* konforma litero */
};
extern struct x_mkalk * x_mk /* [256] */; /* tabelo de frekvencoj */
extern
LITER * x_rmlit /* [256] */; /* tabelo de nerenkontighintaj literoj */
#endif

/* enigi unu biton */ /* chiam makro */
#define EN_BIT() \
( enb_nb>>=1,( enb_nb ? (enb_nb&enb_bb) \
                      : (enb_bb=en_l(),enb_nb=0x80,enb_nb&enb_bb) ) )

#ifndef MAKRO

NIVEL en_llon(), en_flon();
unsigned int en_min_kod();
NODNUM en_fnum();
#define EN_LLON( val ) val = en_llon() /* enigi longecon de literalo */
#define EN_FLON( val ) val = en_flon() /* enigi longecon de folio */
#define EN_MIN_KOD( val, dia ) val = en_min_kod( dia )
#ifdef DIAP_KOD
#define EN_FNUM( val, dia ) \
          val = en_fnum( dia ) /* enigi relativan numeron de folio */
#else
#define EN_FNUM( val, dia ) val = en_min_kod( dia )
#endif

#else /* MAKRO */

/* enigi longecon de literalo */
#define EN_LLON( val ) \
{ \
    register NIVEL _s = 1, \
                   _r = 0; \
 \
    for ( ; _s < 1<<5 && EN_BIT(); _r+=_s, _s<<=1 ) ; \
    /* nun _s estas 2^<longeco de la kodo> */ \
    for ( _s>>=1; _s; _s>>=1 ) \
        if ( EN_BIT() ) _r += _s; \
    val = _r; \
}

/* enigi longecon de folio */ /* uzatas unuara kodo ( 0, 1, ... ) */
#define EN_FLON( val ) \
{ \
    register NIVEL _s = 1, \
                   _r = 0; \
 \
    for ( ; _s /* kontraw fusho */ && EN_BIT(); _r+=_s, _s<<=1 ) ; \
    /* nun _s estas 2^<longeco de la kodo> */ \
    for ( _s>>=1; _s; _s>>=1 ) \
        if ( EN_BIT() ) _r += _s; \
    val = _r; \
}

/* enigi kodon de minimuma longeco */
#define EN_MIN_KOD( r, makskod ) \
{ \
    register unsigned int _i, _j; \
    register unsigned int _m; \
 \
    _i = makskod; \
    /* eksciu la longecon de makskod */ \
    if ( _i&0xff00 ) \
        if ( _i&0xf000 ) \
            if ( _i&0xc000 ) \
                if ( _i&0x8000 ) \
                    _j = 0x8000; \
                else \
                    _j = 0x4000; \
            else \
                if ( _i&0x2000 ) \
                    _j = 0x2000; \
                else \
                    _j = 0x1000; \
        else \
            if ( _i&0x0c00 ) \
                if ( _i&0x0800 ) \
                    _j = 0x0800; \
                else \
                    _j = 0x0400; \
            else \
                if ( _i&0x0200 ) \
                    _j = 0x0200; \
                else \
                    _j = 0x0100; \
    else \
        if ( _i&0x00f0 ) \
            if ( _i&0x00c0 ) \
                if ( _i&0x0080 ) \
                    _j = 0x0080; \
                else \
                    _j = 0x0040; \
            else \
                if ( _i&0x0020 ) \
                    _j = 0x0020; \
                else \
                    _j = 0x0010; \
        else \
            if ( _i&0x000c ) \
                if ( _i&0x0008 ) \
                    _j = 0x0008; \
                else \
                    _j = 0x0004; \
            else \
                if ( _i&0x0002 ) \
                    _j = 0x0002; \
                else \
                    _j = 0x0001; \
    /* en _j - supra bito de makskod */ \
    /* en _i metu "mallongan" kodon */ \
    _m = _j; \
    for ( _i=0, _j>>=1; _j; _j>>=1 ){ \
        if ( EN_BIT() ) _i += _j; \
    } \
    /* en _m - supra bito de makskod */ \
    if (   _i < (makskod & ~_m) /* necesas enigi plian supran biton */ \
       && EN_BIT()           ) \
        _i += _m; \
    else _i = _m - _i - 1; /* reordigu */ \
    r = _i; \
}

/* enigi relativan numeron de folio */
#ifndef DIAP_KOD
#define EN_FNUM( r, dia ) EN_MIN_KOD( r, dia )
#else DIAP_KOD
#define EN_FNUM( r, dia ) \
{ \
    register NODNUM _start, _r, _d; \
 \
    _d = dia; \
    while ( den_Dia < _d ){ \
        den_Dia <<= 1; \
        den_start <<= 1; \
    } \
    _start = den_start; \
    _r = 0; \
    for ( ; (_start<<1) <= _d && EN_BIT(); \
           _r+=_start, _d-=_start, _start<<=2 ) ; \
    if ( (_start<<1) <= _d ){ \
        for ( _start>>=1; _start; _start>>=1 ) \
            if ( EN_BIT() ) _r += _start; \
        r = _r; \
    } else { /* (_start<<1) > _d */ \
        EN_MIN_KOD( _start, _d ); \
        r = _r + _start; \
    } \
}
#endif /* DIAP_KOD */
#endif /* MAKRO */

#ifdef X_KOD
/* enigi "efektivan" kodon */
#define EN__KOD( r, mez, dia ) \
{ \
    register unsigned int __m, __k; \
 \
    if ( EN_BIT() == 0 ){ /* unua duono */ \
        EN_MIN_KOD( r, mez ); \
    } else { /* dua duono */ \
        __m = mez; \
        EN_MIN_KOD( __k, dia - __m ); \
        r = __m + __k; \
    } \
}

void ek_x_malkod();
LITER x_malkod();
#endif /* X_KOD */

void ek_enbit();
int  en_lit();

extern int en_l(); /* enigo de unu bajto */
