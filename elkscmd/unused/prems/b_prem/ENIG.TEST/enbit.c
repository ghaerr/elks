/*
 * Funkcioj por enigi kodojn pobite
 * Provizora versio - por kontroli la funkciadon
 *
 * $Header$
 * $Log$
 */

#include "enbit.h"
#include <stdio.h>

static char _b_[100];

/* iniciati enigon */
void
ek_enbit()
{
}

/* enigi biton */
int
en_bit()
{
    char b[80];

    gets( b );
    if ( !strcmp( b, "NODKOP" ) ) return( NODKOP );
    if ( !strcmp( b, "FOLIKOP" ) ) return( FOLIKOP );
    fprintf( stderr, "en_bit: MALVERA BITO <%s>\n", b );
}

/* enigi kodon de minimuma longeco */
unsigned int
en_min_kod( makskod )
int makskod;
{
    int k, mk;

    gets( _b_ );
    sscanf( _b_, "%d %d ", &k, &mk );
    if ( mk == makskod && k < mk ) return( k );
    fprintf( stderr,
"en_min_kod: MALVERA MAKSKODO: %d el %d anstataw NN el %d\n",
                                k,   mk,          makskod );
}

/* enigi longecon de folio */
NIVEL
en_flon()
{
    int lon;

    gets( _b_ );
    sscanf( _b_, "%d ", &lon );
    return( lon );
}

/* enigi relativan numeron de folio */
NODNUM
en_fnum( diapazon )
int diapazon;
{
    int k, mk;

    gets( _b_ );
    sscanf( _b_, "%d %d ", &k, &mk );
    if ( mk == diapazon && k < mk ) return( k );
    fprintf( stderr,
"en_fnum: MALVERA DIAPAZONO: %d el %d anstataw NN el %d\n",
                              k,   mk,         diapazon );
}

/* enigi literalon */
int
en_lit()
{
    int lon;
    register LITER * d;
    register int c;

    gets( _b_ );
    sscanf( _b_, "%d ", &lon );
    ++lon;
    if ( lon > MAKSLITLON ) fprintf( stderr,
        "en_lit: MALVERA LONGECO DE LITERALO: %d\n", lon );
    for ( d=mn_l; d<mfin && d-mn_l<lon && (c=getchar()) != EOF; d++ ){
        *d = c;
        mkre_litf( d );
    }
    if ( d-mn_l < lon ){
        if ( d == mfin ) fprintf( stderr,
            "en_lit: NEATENDITA FINO DE LA PLENIGATA BLOKO\n" );
        if ( c == EOF ) fprintf( stderr,
            "en_lit: NEATENDITA FINO DE LA ENIRA FAJLO\n" );
    }
    mn_l = d;
    return( lon < MAKSLITLON );
}
