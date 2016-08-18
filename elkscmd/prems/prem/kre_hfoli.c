/*
 * Krei folion post donita nodo,
 * se estas konata la necesa pozicio en la hash-tabelo.
 *
 * $Header$
 * $Log$
 */

#include "elarbo.h"

void
kre_hfoli( ant )
HASHNUM ant;
{
    if ( maksintern + maksfoli < MAKSNOD ){
        nod_d[hpoz] = n_l;
        nod_nivel[hpoz] = FOLIO;
        nod_patro[hpoz] = nod_d[ant];
        nod_num[hpoz] = maksfoli++;
    }
}
