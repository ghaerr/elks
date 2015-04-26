/*
 * Difinoj por uzado de Patricia-arbo por frazaro
 *
 * $Header$
 * $Log$
 */

#include "arbo.h"

#define HDIM      5003   /* dimensio de la hash-tabelo */
                         /* (primo > MAKSNOD)          */
/* eblaj valoroj de HDIM: 691 1291 2591 5003 9001 18013, */
/* por alia hash-formulo eble ankaw 35023 kaj 69001      */

typedef int HASHNUM; /* numeroj de nodoj en la tabelo */

/* DATAOJ POR LA NODOJ DE LA ARBO */
/* post la hash-tabelo kushas nekodotaj nodoj (kun nivelo==1) */
extern FRAZ *
    /* [HDIM+NEKODOT] */
    nod_d; /* direktilo al la komenco de la frazo           */
           /* en la enira teksto.                           */
           /* GHI ESTAS UNIKA IDENTIGILO POR INTERNAJ NODOJ */
           /* la nenula valoro signifas uzitan              */
           /* lokon en la tabelo                            */
extern NIVEL *
    /* [HDIM+NEKODOT] */
    nod_nivel; /* longeco de la konforma parto */
               /* de la frazo, aw FOLIO        */
extern FRAZ *
    /* [HDIM] */
    nod_patro; /* identigilo (nod_d) de la patra nodo */
extern NODNUM *
    /* [HDIM] */
    nod_num; /* vica numero de la interna nodo aw de la folio*/

extern NODNUM maksfoli;     /* nombro de uzitaj folioj */
extern NODNUM maksintern;   /* nombro de uzitaj internaj nodoj */
extern HASHNUM hpoz; /* hash-pozicio */

extern FRAZ  n_l; /* direktilo al la nuna litero */
extern FRAZ  fin; /* direktilo post la lasta prilaborota litero */

extern HASHNUM   kod_nod; /* direktilo al la nodo por kodigi */
extern NIVEL     nod_lon; /* longeco en la nodo */
extern NIVEL    maks_lon; /* maks. ebla longeco en la nodo */

HASHNUM  sekv_niv();
HASHNUM  kre_nod();
void     kre_hfoli();
FRAZ     serch();
