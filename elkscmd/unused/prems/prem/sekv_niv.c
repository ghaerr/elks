/*
 * Malsuprenighi al la sekva nivelo,
 * redonas hash-numeron de la konforma nodo
 * aw (-1) se tia ne ekzistas.
 *
 * $Header$
 * $Log$
 */

#include "elarbo.h"

HASHNUM
sekv_niv( nun, lit )
HASHNUM   nun;      /* nuna pozicio */
register LITER lit; /* sekvanta litero */
{
    register HASHNUM nod;
    register NIVEL   niv; /* nivelo */
    register HASHNUM i;

    /* nuna nodo estas plene koincidanta */
    niv = nod_nivel[nun]; /* plia nivelo */
    /* serchu la sekvan nodon */
    if ( nod_d[nod=(((lit+12347)*(int)(nod_d[nun]))&077777)%HDIM] != 0 ){
        /* Vershajne estas tia nodo,
           komencu serchi de la vershajna loko */
        if ( nod_patro[nod] == nod_d[nun] && *(nod_d[nod]+niv) == lit )
            return( nod ); /* tawgas! */
        /* Ne tuj klaras, dawrigu la serchadon */
        i = (nod) ? HDIM-nod : 1;
        while ( 1 ){
            if ( (nod -= i) < 0 ) nod += HDIM;
            if ( nod_d[nod] == 0 ) break; /* ne ekzistas */
            if ( nod_patro[nod]==nod_d[nun] && *(nod_d[nod]+niv)==lit )
                return( nod ); /* tawgas! */
        }
    }
    /* ne ekzistas tia nodo */
    hpoz = nod; /* libera pozicio */
    return( -1 );
}
