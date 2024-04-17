/*
 * Funkcioj por eligi kodojn pobite
 * Provizora versio - por kontroli la funkciadon
 *
 * $Header$
 * $Log$
 */

#include "elbit.h"

/* iniciati eligon */
void
ek_elbit()
{
}

/* eligi biton */
void
el_bit( bit )
int bit;
{
    printf( bit==NODKOP ? "NODKOP\n" : "FOLIKOP\n" );
}

/* eligi kodon de minimuma longeco */
void
el_min_kod( kod, makskod )
int kod, makskod;
{
    printf( "%d %d\n", kod, makskod );
}

/* eligi longecon de folio */
void
el_flon( lon )
int lon;
{
    printf( "%d\n", lon );
}

/* eligi relativan numeron de folio */
void
el_fnum( num, diapazon )
int num, diapazon;
{
    printf( "%d %d\n", num, diapazon );
}

/* eligi la restajhojn el la bita bufro */
void
fin_elbit()
{
}

/* eligi literalon */
int
el_lit( komenc )
LITER * komenc;
{
    register LITER * d, * f;

    while ( n_l - komenc > MAKSLITLON ){
        el_bit( FOLIKOP );
        el_flon( 0 );
        printf( "%d\n", MAKSLITLON-1 );
        for ( d=komenc, f=komenc+MAKSLITLON; d < f; putchar(*d++) ) ;
        komenc += MAKSLITLON;
    }
    el_bit( FOLIKOP );
    el_flon( 0 );
    printf( "%d\n", n_l - komenc - 1 );
    for ( d=komenc; d<n_l; putchar(*d++) ) ;
    return( n_l - komenc );
}
