/*
 * Difinoj por uzado de Patricia-arbo por frazaro
 *
 * $Header$
 * $Log$
 */

#include "arbo.h"
#include <setjmp.h>

/* DATAOJ POR LA ARBO */
/* komence kushas folioj law kreska ordo,      */
/* fine de la tabelo kushas "nekodota nodo",   */
/* antaw ghi kushas internaj nodoj invers-orde */
extern FRAZ *
    /* [MAKSNOD] */
    mnod_d; /* direktilo al la komenco de la frazo */
            /* en la enira teksto                  */
extern NIVEL *
    /* [MAKSNOD+1] */
    mnod_nivel; /* longeco de la konforma parto */
                /* de la frazo, aw FOLIO        */
extern NODNUM *
    /* [MAKSNOD] */
    mnod_patro; /* numero de la patra nodo */

extern NODNUM  mmaksfoli;     /* nombro da uzitaj folioj */
extern NODNUM  mmaksintern;   /* nombro da uzitaj internaj nodoj */

extern FRAZ   mn_l; /* direktilo al la nuna litero */
extern FRAZ   mfin; /* direktilo post la lasta prilaborota litero */

extern jmp_buf meraro; /* por fini se aperis eraro */

void  mkre_foli(), mkre_litf(), mkre_nod();
