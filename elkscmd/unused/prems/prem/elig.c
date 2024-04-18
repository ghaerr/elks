/*
 * Funkcioj por eligi kodojn pobite
 *
 * $Header$
 * $Log$
 */

#include "elbit.h"

#ifndef MAKRO
/* eligi longecon de literalo */
void el_llon( val ) /* uzatas unuara kodo ( 0, 1, 5 ) */
NIVEL val;
{
    register NIVEL _v = val,
                   _s = 1;

    while ( _v >= _s ){
        EL_B_1();
        _v -= _s;
        _s <<= 1;
    }
    if ( _s < 1<<5 ){
        EL_B_0(); /* aldonu nulon post la unuoj */
    }
    for ( _s>>=1; _s; _s>>=1 )
        EL_BIT( _s&_v );
}

/* eligi longecon de folio */
void el_flon( val ) /* uzatas unuara kodo ( 0, 1, ... ) */
NIVEL val;
{
    register NIVEL _v = val,
                   _s = 1;

    while ( _v >= _s ){
        EL_B_1();
        _v -= _s;
        _s <<= 1;
    }
    EL_B_0(); /* aldonu nulon post la unuoj */
    for ( _s>>=1; _s; _s>>=1 )
        EL_BIT( _s&_v );
}

/* eligi kodon de minimuma longeco */
void el_min_kod( kod, makskod )
unsigned int kod, makskod;
{
    register unsigned int _i, _j, _k, _kod, _maks;

    _kod = kod; _i = _maks = makskod;
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
    if ( _kod < _j ) /* reordigu */
        _kod = _j - _kod - 1;
    /* eligu "mallongan" kodon */
    _k = _j; /* supra bito de makskod */
    _j >>= 1;
    for ( _i=_kod; _j; _j>>=1 ) EL_BIT( _i&_j );
    /* en _k - supra bito de makskod */
    if ( _kod >= _k
     || _kod < (_maks & ~_k) ){ /* ne eblas malplilongigi */
        EL_BIT( _k&_kod );
    }
}

#ifdef DIAP_KOD
/* eligi relativan numeron de folio */
void
el_fnum( val, dia )
NODNUM val, dia;
{
    /* eligi "diapazonigitan" unuaran kodon ( 0, 2, 4 ) */
    register NODNUM _v, _d, _start;

    _v = val; _d = dia;
    while ( del_Dia < _d ){
        del_Dia <<= 1;
        del_start <<= 1;
    }
    _start = del_start;
    while ( _v >= _start && (_start<<1) <= _d ){
        EL_B_1();
        _v -= _start;
        _d -= _start;
        _start <<= 2; /* pash */
    }
    if ( (_start<<1) <= _d ){
        EL_B_0(); /* aldonu nulon post la unuoj */
        for ( _start>>=1; _start; _start>>=1 )
            EL_BIT( _v&_start );
    } else { /* (_start<<1) > _d */
        EL_MIN_KOD( _v, _d );
    }
}
#endif /* DIAP_KOD */
#endif /* MAKRO */
