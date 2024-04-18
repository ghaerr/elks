/*
 * Enigi literalon.
 * Redonas 1, se la literalo estis ne maksimume longa,
 *    alie 0.
 *
 * $Header$
 * $Log$
 */

#include "enbit.h"

int en_lit(void)
{
    NIVEL lon;
    register FRAZ d;

    EN_LLON( lon );
    ++lon;
    for ( d=mn_l; d<mfin && d-mn_l<lon; d++ ){
#ifdef X_KOD
        *d = x_malkod();
#else 
        { register int i, j = 0;
            for ( i=0x80; i; i >>= 1 ){
                if ( EN_BIT() ) j += i;
            }
            *d = j;
        }
#endif /* X_KOD */
        mkre_litf( d );
    }
    if ( d-mn_l < lon ) longjmp( meraro, -1 );
    mn_l = d;
    return( lon < MAKSLITLON );
}
