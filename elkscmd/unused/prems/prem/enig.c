/*
 * Funkcioj por enigi kodojn pobite
 *
 * $Header$
 * $Log$
 */

#include "enbit.h"

#ifndef MAKRO
/* enigi longecon de literalo */
NIVEL en_llon()
{
    register NIVEL _s = 1,
                   _r = 0;

    for ( ; _s < 1<<5 && EN_BIT(); _r+=_s, _s<<=1 ) ;
    /* nun _s estas 2^<longeco de la kodo> */
    for ( _s>>=1; _s; _s>>=1 )
        if ( EN_BIT() ) _r += _s;
    return( _r );
}

/* enigi longecon de folio */ /* uzatas unuara kodo ( 0, 1, ... ) */
NIVEL en_flon()
{
    register NIVEL _s = 1,
                   _r = 0;

    for ( ; _s /* kontraw fusha eniro */ && EN_BIT(); _r+=_s, _s<<=1 ) ;
    /* nun _s estas 2^<longeco de la kodo> */
    for ( _s>>=1; _s; _s>>=1 )
        if ( EN_BIT() ) _r += _s;
    return( _r );
}

/* enigi kodon de minimuma longeco */
unsigned int en_min_kod(  makskod )
unsigned int makskod;
{
    register unsigned int _i, _j;
    register unsigned int _m;

    _i = makskod;
    /* eksciu la longecon de makskod */
    if ( _i&0xff00 )
        if ( _i&0xf000 )
            if ( _i&0xc000 )
                if ( _i&0x8000 )
                    _j = 0x8000;
                else
                    _j = 0x4000;
            else
                if ( _i&0x2000 )
                    _j = 0x2000;
                else
                    _j = 0x1000;
        else
            if ( _i&0x0c00 )
                if ( _i&0x0800 )
                    _j = 0x0800;
                else
                    _j = 0x0400;
            else
                if ( _i&0x0200 )
                    _j = 0x0200;
                else
                    _j = 0x0100;
    else
        if ( _i&0x00f0 )
            if ( _i&0x00c0 )
                if ( _i&0x0080 )
                    _j = 0x0080;
                else
                    _j = 0x0040;
            else
                if ( _i&0x0020 )
                    _j = 0x0020;
                else
                    _j = 0x0010;
        else
            if ( _i&0x000c )
                if ( _i&0x0008 )
                    _j = 0x0008;
                else
                    _j = 0x0004;
            else
                if ( _i&0x0002 )
                    _j = 0x0002;
                else
                    _j = 0x0001;
    /* en _j - supra bito de makskod */
    /* en _i metu "mallongan" kodon */
    _m = _j;
    for ( _i=0, _j>>=1; _j; _j>>=1 ){
        if ( EN_BIT() ) _i += _j;
    }
    /* en _m - supra bito de makskod */
    if (   _i < (makskod & ~_m) /* necesas enigi plian supran biton */
       && EN_BIT()           )
        _i += _m;
    else _i = _m - _i - 1; /* reordigu */
    return( _i );
}

#ifdef DIAP_KOD
/* enigi relativan numeron de folio */
NODNUM
en_fnum( dia )
NODNUM dia;
{
    register NODNUM _start, _d, _r;

    _d = dia;
    while ( den_Dia < _d ){
        den_Dia <<= 1;
        den_start <<= 1;
    }
    _start = den_start;
    _r = 0;
    for ( ; (_start<<1) <= _d && EN_BIT();
           _r+=_start, _d-=_start, _start<<=2 ) ;
    if ( (_start<<1) <= _d ){
        for ( _start>>=1; _start; _start>>=1 )
            if ( EN_BIT() ) _r += _start;
        return( _r );
    } else { /* (_start<<1) > _d */
        EN_MIN_KOD( _start, _d );
        return( _r + _start );
    }
}
#endif /* DIAP_KOD */
#endif /* MAKRO */
