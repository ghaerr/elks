/*
 * Krei folion, komencighantan en literalo
 *
 * $Header$
 * $Log$
 */

#include "enarbo.h"

void
mkre_litf( d )
register FRAZ d;
{
    register NODNUM nod;

    if ( mmaksintern + mmaksfoli == MAKSNOD ) return;
    /* necesas aldoni folion */
    nod = mmaksfoli++;
    mnod_d[nod] = d;
    mnod_nivel[nod] = FOLIO;
    mnod_patro[nod] = MAKSNOD; /* nekodota nodo de nivelo 1 */
}
