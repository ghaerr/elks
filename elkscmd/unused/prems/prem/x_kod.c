/*
 * Originala algoritmo uzata por kodigi literalojn.
 * Oni kodigas nur renkontighintajn literojn
 * per kodo, proksimume adaptita al la frekvencoj.
 *
 * $Header$
 * $Log$
 */

#include "elbit.h"

#ifdef X_KOD

static int x_maks; /* diapazono de transdonataj kodoj */

static struct x_kalk * x_mez;
static unsigned int x_mezs; /* sumo maldekstre de "mezo" */
static unsigned int x_entute; /* kvanto da prilaboritaj kodoj */

void ek_x_kod()
{
    {
        register struct x_kalk ** d, ** f;
        for ( f=(d=x_indeks)+256; d < f; *d++ = 0 ) ;
    }
    {
        register LITER * d, * f;
        for ( f=(d=x_rindeks)+256; d < f; ++d ){
            *d = d - x_rindeks;
        }
    }
    x_maks = 1; /* unu kodo jam ekzistas - por signi novan literon */
    x_mez = x_k;
    x_mezs = x_entute = 0;
}

void x_kod( lit )
LITER lit;
{
    register struct x_kalk * d, * dm;
    register unsigned int k;

    if ( dm=d=x_indeks[lit] ){ /* jam estis */
        EL__KOD( d-x_k+1, x_mez-x_k, x_maks );
        /* reordigu law la frekvencoj */
        k = d->k;
        /* movu ghis la sekva frekvenco */
        while ( d-- > x_k && k == d->k ) ;
        ++((++d)->k);
        x_indeks[dm->l=d->l] = dm;
        x_indeks[d->l=lit] = d;
    } else { /* nova litero */
        register LITER * di, * fi;
        EL__KOD( 0, x_mez-x_k, x_maks );
        EL_MIN_KOD( x_rindeks[lit], 256-(x_maks-1) );
        for ( di=x_rindeks+lit, fi=x_rindeks+256; di<fi; --(*di++) ) ;
        (d=x_indeks[lit]=x_k+(x_maks++)-1)->k = 1;
        d->l = lit;
    }
    /* rekalkulu la pozicion de x_mez */
    if ( d < x_mez ) ++x_mezs;
#define KONTROL( p, p_s ) \
if ( k < p_s ) p_s -= (--p)->k; \
else if ( k > p_s + p->k ) p_s += (p++)->k;
    k = (++x_entute)>>1; /* duono */
    KONTROL( x_mez, x_mezs );
#undef KONTROL
}
#endif /* X_KOD */
