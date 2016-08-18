/*
 * Dekodigi el la originala kodo (rig. x_kod.c)
 *
 * $Header$
 * $Log$
 */

#include "enbit.h"

#ifdef X_KOD

struct x_mkalk * x_mk /* [256] */; /* tabelo de frekvencoj */

LITER * x_rmlit /* [256] */; /* tabelo de nerenkontighintaj literoj */

static int x_mmaks; /* diapazono de eniraj kodoj */

static struct x_mkalk * x_mmez; /* adaptigaj parametroj */
static unsigned int x_mmezs;
static unsigned int x_mentute;

void ek_x_malkod()
{
    register LITER * d, * f;

    for ( f=(d=x_rmlit)+256; d<f; *d = d-x_rmlit, ++d ) ;
    x_mmaks = 1;
    x_mmez = x_mk;
    x_mmezs = x_mentute = 0;
}

LITER x_malkod()
{
    register struct x_mkalk * d, * dm;
    register unsigned int k, l;
    register int i;

    EN__KOD( i, x_mmez-x_mk, x_mmaks );
    if ( i != 0 ){ /* ne nova litero */
        --i;
        l = (dm=d=x_mk+i)->l;
        k = d->k;
        /* reordigu law novaj frekvencoj */
        while ( d-- > x_mk && k == d->k ) ;
        ++((++d)->k);
        dm->l = d->l;
    } else {
        register LITER * di, * dim, *fi;
        EN_MIN_KOD( i, 256-(x_mmaks-1) );
        l = x_rmlit[i];
        for ( dim=(di=x_rmlit+i)+1,
              fi=x_rmlit+256-x_mmaks;
             di < fi;
             *di++ = *dim++ ) ;
        /* metu novan literon al la fino */
        d = x_mk + (x_mmaks++) - 1;
        d->k = 1;
    }
    /* rekalkulu la poziciojn de x_mmez */
    if ( d < x_mmez ) ++x_mmezs;
#define KONTROL( p, p_s ) \
if ( k < p_s ) p_s -= (--p)->k; \
else if ( k > p_s + p->k ) p_s += (p++)->k;
    k = (++x_mentute)>>1; /* duono */
    KONTROL( x_mmez, x_mmezs );
#undef KONTROL
    return( d->l=l );
}
#endif /* X_KOD */
