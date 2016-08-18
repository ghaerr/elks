/*
 * Eligi literalon.
 * Redonas (n_l - komenc) % MAKSLITLON
 *
 * $Header$
 * $Log$
 */

#include "elbit.h"

int el_lit(FRAZ komenc)
{
    register FRAZ d, f;

    while ( n_l - komenc > MAKSLITLON ){
        EL_B_1(); /* EL_BIT( FOLIKOP ); */
        EL_FLON( 0 );
        EL_LLON( MAKSLITLON-1 );
#ifdef X_KOD
        for ( f=(d=komenc)+MAKSLITLON; d < f; x_kod( *d++ ) ) ;
#else
        for ( f=(d=komenc)+MAKSLITLON; d < f; d++ ){
            register int i, j = *d;
            for ( i=0x80; i; i>>=1 ){
                EL_BIT( i&j );
            }
        }
#endif /* X_KOD */
        komenc += MAKSLITLON;
    }
    EL_B_1(); /* EL_BIT( FOLIKOP ); */
    EL_FLON( 0 );
    EL_LLON( (NIVEL)(n_l-komenc-1) );
#ifdef X_KOD
    for ( d=komenc; d<n_l; x_kod( *d++ ) ) ;
#else
    for ( d=komenc; d<n_l; d++ ){
        register int i, j = *d;
        for ( i=0x80; i; i>>=1 ){
            EL_BIT( i&j );
        }
    }
#endif /* X_KOD */
    return( n_l - komenc < MAKSLITLON );
}
