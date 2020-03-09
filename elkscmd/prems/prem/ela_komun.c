/*
 * Komunaj dataoj por uzado de Patricia-arbo por frazaro
 *
 * $Header$
 * $Log$
 */

#include "elarbo.h"

/* DATAOJ POR LA NODOJ DE LA ARBO */
/* post la hash-tabelo kushas nekodotaj nodoj (kun nivelo==1) */
FRAZ *
    /* [HDIM+NEKODOT] */
    nod_d; /* direktilo al la komenco de la frazo           */
           /* en la enira teksto.                           */
           /* GHI ESTAS UNIKA IDENTIGILO POR INTERNAJ NODOJ */
           /* la nenula valoro signifas uzitan              */
           /* lokon en la tabelo                            */
NIVEL *
    /* [HDIM+NEKODOT] */
    nod_nivel; /* longeco de la konforma parto */
               /* de la frazo, aw FOLIO        */
FRAZ *
    /* [HDIM] */
    nod_patro; /* identigilo (nod_d) de la patra nodo */
NODNUM *
    /* [HDIM] */
    nod_num; /* vica numero de la interna nodo aw de la folio*/

NODNUM maksfoli;     /* nombro de uzitaj folioj */
NODNUM maksintern;   /* nombro de uzitaj internaj nodoj */
HASHNUM hpoz; /* hash-pozicio */

FRAZ  n_l; /* direktilo al la nuna litero */
FRAZ  fin; /* direktilo post la lasta prilaborota litero */

HASHNUM   kod_nod; /* direktilo al la nodo por kodigi */
NIVEL     nod_lon; /* longeco en la nodo */
NIVEL    maks_lon; /* maks. ebla longeco en la nodo */
