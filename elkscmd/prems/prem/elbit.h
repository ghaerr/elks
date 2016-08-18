/*
 * Funkcioj por eligi kodojn pobite
 *
 * ek_elbit()                  /* iniciati eligon
 * fin_elbit()                 /* eligi la bitan bufron
 * el_lit( komenc )            /* eligi literalon, redonas
 *                             /* (n_l - komenc) % MAKSLITLON
 *
 * $Header$
 * $Log$
 */

#include "bit.h"
#include "elarbo.h"

union elb_b {
    int bb_i;
    struct {
        char ms;
        char su;
    } bb_c;
};
extern union elb_b elb_bb; /* eliga bufro por bitoj */
extern int elb_nb; /* kvanto da bitoj en la bufro */

#ifdef DIAP_KOD
extern unsigned int del_Dia; /* maksimuma diapazono */
extern unsigned int del_start;
#endif /* DIAP_KOD */

#ifdef X_KOD
struct x_kalk {
    unsigned int k; /* kalkulilo por tiu chi indekso */
    LITER l; /* konforma litero */
};
extern struct x_kalk * x_k /* [256] */, ** x_indeks /* [256] */;
extern LITER * x_rindeks /* [256] */;
#endif /* X_KOD */

/* eligi unu biton */ /* chiam makro */
#define EL_BIT( bit ) { \
        elb_bb.bb_i <<= 1; if ( bit ) ++elb_bb.bb_i; \
        ++elb_nb; \
        if ( (elb_nb&=0xf) == 0 ){ \
            el_l( elb_bb.bb_c.su ); \
            el_l( elb_bb.bb_c.ms ); \
        } \
    }
#define EL_B_0() { \
        elb_bb.bb_i <<= 1; \
        ++elb_nb; \
        if ( (elb_nb&=0xf) == 0 ){ \
            el_l( elb_bb.bb_c.su ); \
            el_l( elb_bb.bb_c.ms ); \
        } \
    }
#define EL_B_1() { \
        elb_bb.bb_i <<= 1; ++elb_bb.bb_i; \
        ++elb_nb; \
        if ( (elb_nb&=0xf) == 0 ){ \
            el_l( elb_bb.bb_c.su ); \
            el_l( elb_bb.bb_c.ms ); \
        } \
    }

#ifndef MAKRO

void el_llon(), el_flon(), el_min_kod(), el_fnum();
#define EL_LLON( val ) el_llon( val ) /* por longecoj de literaloj */
#define EL_FLON( val ) el_flon( val ) /* por longecoj de folioj */
#define EL_MIN_KOD( val, diap ) el_min_kod( val, diap )
#ifdef DIAP_KOD
#define EL_FNUM( num, dia ) \
        el_fnum( num, dia ) /* relativa numero de folio */
#else  /* DIAP_KOD */
#define EL_FNUM( num, dia ) el_min_kod( num, dia )
#endif /* DIAP_KOD */

#else /* MAKRO */

/* eligi longecon de literalo */
#define EL_LLON( val ) /* uzatas unuara kodo ( 0, 1, 5 ) */ \
{ \
    register NIVEL _v = val, \
                   _s = 1; \
 \
    while ( _v >= _s ){ \
        EL_B_1(); \
        _v -= _s; \
        _s <<= 1; \
    } \
    if ( _s < 1<<5 ){ \
        EL_B_0(); /* aldonu nulon post la unuoj */ \
    } \
    for ( _s>>=1; _s; _s>>=1 ) \
        EL_BIT( _s&_v ); \
}

/* eligi longecon de folio */
#define EL_FLON( val ) /* uzatas unuara kodo ( 0, 1, ... ) */ \
{ \
    register NIVEL _v = val, \
                   _s = 1; \
 \
    while ( _v >= _s ){ \
        EL_B_1(); \
        _v -= _s; \
        _s <<= 1; \
    } \
    EL_B_0(); /* aldonu nulon post la unuoj */ \
    for ( _s>>=1; _s; _s>>=1 ) \
        EL_BIT( _s&_v ); \
}

/* eligi kodon de minimuma longeco */
#define EL_MIN_KOD( kod, makskod ) \
{ \
    register unsigned int _i, _j, _k, _kod, _maks; \
 \
    _kod = kod; _i = _maks = makskod; \
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
    if ( _kod < _j ) /* reordigu */ \
        _kod = _j - _kod - 1; \
    /* eligu "mallongan" kodon */ \
    _k = _j; /* supra bito de makskod */ \
    _j >>= 1; \
    for ( _i=_kod; _j; _j>>=1 ) EL_BIT( _j&_i ); \
    /* en _k - supra bito de makskod */ \
    if ( _kod >= _k \
     || _kod < (_maks & ~_k) ){ /* ne eblas malplilongigi */ \
        EL_BIT( _k&_kod ); \
    } \
}

/* eligi relativan numeron de folio */
#ifndef DIAP_KOD
#define EL_FNUM( val, dia ) EL_MIN_KOD( val, dia )
#else DIAP_KOD
/* eligi "diapazonigitan" unuaran kodon ( 0, 2, 4 ) */
#define EL_FNUM( val, dia ) \
{ \
    register NODNUM _v, _d, _start; \
 \
    _v = val; _d = dia; \
    while ( del_Dia < _d ){ \
        del_Dia <<= 1; \
        del_start <<= 1; \
    } \
    _start = del_start; \
    while ( _v >= _start && (_start<<1) <= _d ){ \
        EL_B_1(); \
        _v -= _start; \
        _d -= _start; \
        _start <<= 2; /* pash */ \
    } \
    if ( (_start<<1) <= _d ){ \
        EL_B_0(); /* aldonu nulon post la unuoj */ \
        for ( _start>>=1; _start; _start>>=1 ) \
            EL_BIT( _start&_v ); \
    } else { /* (_start<<1) > _d */ \
        EL_MIN_KOD( _v, _d ); \
    } \
}
#endif /* DIAP_KOD */
#endif /* MAKRO */

#ifdef X_KOD
/* eligi "efektivan" kodon */
#define EL__KOD( val, mez, dia ) \
{ \
    register unsigned int __v, __m; \
 \
    __v = val; __m = mez; \
    if ( __v < __m ){ /* unua duono de kodoj */ \
        EL_B_0(); \
        EL_MIN_KOD( __v, __m ); \
    } else { /* dua duono */ \
        EL_B_1(); \
        EL_MIN_KOD( __v - __m, dia - __m ); \
    } \
}

void ek_x_kod(), x_kod();
#endif /* X_KOD */

void ek_elbit(), fin_elbit();
int  el_lit();

extern void el_l(); /* eligo de bajto eksteren */
