/*
 * Krei nodon de donita nivelo antaw la donita nodo.
 * Krei folion post kreita nodo.
 *
 * $Header$
 * $Log$
 */

#include "elarbo.h"

HASHNUM
kre_nod(         post, niv )
register HASHNUM post;
         NIVEL         niv;
{
    register HASHNUM nod;
    register HASHNUM i;
             HASHNUM novpost;

    if ( maksintern + maksfoli == MAKSNOD ) return( post );
    /* ni metos la novan nodon anstataw `post' */
    /* serchu la lokon en la hash-tabelo por la posta */
    if ( nod_d[ nod=
             (((*(nod_d[post]+niv)+12347)*(int)n_l)&077777)%HDIM
             ] != 0 ){
        /* Ne estas libera, serchu plu */
        i = (nod) ? HDIM-nod : 1;
        while ( 1 ){
            if ( (nod -= i) < 0 ) nod += HDIM;
            if ( nod_d[nod] == 0 ) break; /* liberas */
        }
    }
    /* trovis liberan lokon */
    /* movu la postan nodon */
    nod_d[nod] = nod_d[post];
    nod_nivel[nod] = nod_nivel[post];
    nod_patro[nod] = n_l; /* identigilo de la nova nodo */
    nod_num[nod] = nod_num[post];
    /* por la nova */
    nod_d[post] = n_l; /* komenco de la frazo */
    nod_nivel[post] = niv;
    /* la patro estas jam ghusta */
    nod_num[post] = maksintern++;

    /* kreu novan folion post la kreita nodo */
    if ( maksintern + maksfoli == MAKSNOD ) return( nod );
    novpost = nod;
    /* serchu la liberan lokon */
    if ( nod_d[ nod=
             (((*(n_l+niv)+12347)*(int)n_l)&077777)%HDIM
             ] != 0 ){
        /* Ne estas libera, serchu plu */
        i = (nod) ? HDIM-nod : 1;
        while ( 1 ){
            if ( (nod -= i) < 0 ) nod += HDIM;
            if ( nod_d[nod] == 0 ) break; /* liberas */
        }
    }
    nod_d[nod] = n_l;
    nod_nivel[nod] = FOLIO;
    nod_patro[nod] = n_l; /* identigilo de la nova */
    nod_num[nod] = maksfoli++;
    return( novpost );
}
