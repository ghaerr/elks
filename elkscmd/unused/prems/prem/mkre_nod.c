/*
 * Krei folion, komencighantan post interna nodo
 * aw internan nodon kaj folion post ghi.
 *
 * $Header$
 * $Log$
 */

#include "enarbo.h"

void
mkre_nod( old, lon )
NODNUM old;
NIVEL lon;
{
    register FRAZ el, en, f;
    register NODNUM nod;

    /* kopiu la frazon */
    f =
     (el=mnod_d[old]) +
     ( lon ? mnod_nivel[mnod_patro[old]]+lon : mnod_nivel[old] );
    for ( en=mn_l; el < f && en < mfin;
        *en++ = *el++ ) ;
    if ( el != f ) longjmp( meraro, -1 );
    if ( mmaksintern + mmaksfoli < MAKSNOD ){
        if ( lon ){ /* bezonatas intera nodo */
            /* kreu la internan nodon */
            nod = MAKSNOD - 1 - mmaksintern++;
            mnod_d[nod] = mn_l;
            mnod_nivel[nod] =
                mnod_nivel[mnod_patro[nod]=mnod_patro[old]] + lon;
            mnod_patro[old] = nod;
            old = nod; /* patro por la kreota folio */
        }
        /* kreu la folion */
        if ( mmaksintern + mmaksfoli < MAKSNOD ){
            nod = mmaksfoli++;
            mnod_d[nod] = mn_l;
            mnod_nivel[nod] = FOLIO;
            mnod_patro[nod] = old;
        }
    }
    mn_l = en;
}
