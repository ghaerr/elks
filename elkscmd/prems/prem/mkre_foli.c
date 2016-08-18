/*
 * Krei folion, komencighantan en folio,
 * do internan nodon kaj folion.
 *
 * $Header$
 * $Log$
 */

#include "enarbo.h"

void
mkre_foli( post, lon )
register NODNUM post;
NIVEL lon;
{
    register FRAZ el, en, f;
    register NODNUM nod, patr;

    /* kopiu la frazon */
    for ( f=(el=mnod_d[post])+mnod_nivel[mnod_patro[post]]+lon,en=mn_l;
         el < f && en < mfin;
         *en++ = *el++ ) ;
    if ( el != f ) longjmp( meraro, -1 );
    /* kreu la internan nodon */
    if ( mmaksintern + mmaksfoli < MAKSNOD ){
        patr = nod = MAKSNOD - 1 - mmaksintern++;
        mnod_d[nod] = mn_l;
        mnod_nivel[nod] =
            mnod_nivel[mnod_patro[nod]=mnod_patro[post]] + lon;
        mnod_patro[post] = nod;
        /* kreu la folion */
        if ( mmaksintern + mmaksfoli < MAKSNOD ){
            nod = mmaksfoli++;
            mnod_d[nod] = mn_l;
            mnod_nivel[nod] = FOLIO;
            mnod_patro[nod] = patr;
        }
    }
    mn_l = en;
}
