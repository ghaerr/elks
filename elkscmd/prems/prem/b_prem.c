/*
 * Premo de unu bloko
 *
 * $Header$
 * $Log$
 */

#ifndef lint
static char rcsid[] = "$Header$";
#endif

#include "elbit.h"
#include <setjmp.h>

static void ek_arb();
static jmp_buf malsukces;

static FRAZ elk, elf; /* direktiloj al elira areo */

/*
 * Premi eniran blokon, plenigante la eliran.
 * Redonas la longecon de la elira, se sukcese malpliigis
 * ghin kompare kun la enira,
 * aliokaze redonas (-1).
 *
 * La loko por la elira bloko devas esti samlonga
 * kiel la enira.
 */
int b_prem( elb, lon, alb )
FRAZ  elb; /* adreso de la enira bloko  */
NIVEL lon; /* longeco de la enira bloko */
FRAZ  alb; /* adreso de la elira bloko  */
{
    register FRAZ l; /* direktilo al nuna enira bajto */
    register FRAZ lit_kom;
    register NODNUM omksi;
    register NODNUM omksf;

    if ( lon < 2 ) return( -1 ); /* malsukceso */
    if ( (nod_d=(FRAZ *)calloc(HDIM+NEKODOT,sizeof(FRAZ))) == 0
     || (nod_nivel=(NIVEL *)malloc(sizeof(NIVEL)*(HDIM+NEKODOT))) == 0
     || (nod_patro=(FRAZ *)malloc(sizeof(FRAZ)*HDIM)) == 0
     || (nod_num=(NODNUM *)malloc(sizeof(NODNUM)*HDIM)) == 0
#ifdef X_KOD
     || (x_k=(struct x_kalk *)malloc(sizeof(*x_k)*256)) == 0
     || (x_indeks=(struct x_kalk **)malloc(sizeof(*x_indeks)*256)) == 0
     || (x_rindeks=(LITER *)malloc(sizeof(*x_rindeks)*256)) == 0
#endif
    ){
        write( 2, "b_prem: mankas memoro\n", 22 );
        exit( 1 );  /* ne sufichas memoro */
    }
    if ( setjmp( malsukces ) ){
        /* redonu la memoron */
        free( (char *)nod_d );
        free( (char *)nod_nivel );
        free( (char *)nod_patro );
        free( (char *)nod_num );
#ifdef X_KOD
        free( (char *)x_k );
        free( (char *)x_indeks );
        free( (char *)x_rindeks );
#endif
        return( -1 ); /* malsukceso */
    }
    ek_arb( elb, lon, alb );
    l = lit_kom = elb;
    omksi = maksintern;
    omksf = maksfoli;
    for ( ; l < fin; n_l=l, omksi=maksintern, omksf=maksfoli ){
        l = serch();
        /* trovita estas la sekva koincido */
        if ( l - n_l > 1 ){ /* kopiu frazon el frazaro */
            register HASHNUM nod;
            if ( nod_nivel[nod=kod_nod] != FOLIO
            ){ /*estas interna nodo*/
                if ( n_l > lit_kom ) el_lit( lit_kom );
                EL_B_0(); /* EL_BIT( NODKOP ); */
                EL_MIN_KOD( nod_num[nod], omksi );
                EL_MIN_KOD( nod_lon, maks_lon );
            } else { /* estas folio */
                if ( n_l > lit_kom && el_lit( lit_kom ) ) --nod_lon;
                                                   /* estis literalo */
                EL_B_1(); /* EL_BIT( FOLIKOP ); */
                EL_FLON( nod_lon );
                EL_FNUM( omksf-nod_num[nod]-1, omksf );
            }
            lit_kom = l;
        } else ;  /*  l-n_l==1  => ne trovighis kopiebleco */
    }
    /* fino de la bloko */
    if ( n_l > lit_kom ) el_lit( lit_kom );
    fin_elbit();
    /* redonu la memoron */
    free( (char *)nod_d );
    free( (char *)nod_nivel );
    free( (char *)nod_patro );
    free( (char *)nod_num );
#ifdef X_KOD
    free( (char *)x_k );
    free( (char *)x_indeks );
    free( (char *)x_rindeks );
#endif
    return( elk - alb );
}

static void ek_arb( el, lon, al ) /* fari komencajn starigojn por la arbo */
FRAZ  el;
NIVEL lon;
FRAZ  al;
{
    register HASHNUM i;

    fin = (n_l=el) + lon;
    elf = (elk=al) + lon - 1;
    /* iniciatu la nekodotajn nodojn */
    for ( i=0; i<NEKODOT; ++i ) nod_nivel[HDIM+i] = 1;
    maksfoli = maksintern = 0;
    ek_elbit();
}

/* eligo de vica premita litero */
void el_l( lit )
int lit;
{
    if ( elk >= elf ) longjmp( malsukces, -1 );
    *(elk++) = lit;
}
