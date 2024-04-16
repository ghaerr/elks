/*
 * Komunaj dataoj
 *
 * $Header$
 * $Log$
 */

#include "enarbo.h"

/* DATAOJ POR LA ARBO */
/* komence kushas folioj law kreska ordo,      */
/* fine de la tabelo kushas "nekodota nodo",   */
/* antaw ghi kushas internaj nodoj invers-orde */
FRAZ *
    /* [MAKSNOD] */
    mnod_d; /* direktilo al la komenco de la frazo */
            /* en la enira teksto                  */
NIVEL *
    /* [MAKSNOD+1] */
    mnod_nivel; /* longeco de la konforma parto */
                /* de la frazo, aw FOLIO        */
NODNUM *
    /* [MAKSNOD] */
    mnod_patro; /* numero de la patra nodo */

NODNUM  mmaksfoli;     /* nombro da uzitaj folioj */
NODNUM  mmaksintern;   /* nombro da uzitaj internaj nodoj */

FRAZ   mn_l; /* direktilo al la nuna litero */
FRAZ   mfin; /* direktilo post la lasta prilaborota litero */

jmp_buf meraro; /* por fini se aperis eraro */
