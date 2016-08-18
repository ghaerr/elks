/*
 * Malpremo de unu bloko
 * Redonas 0
 * aw (-1) se okazis eraro.
 * La uzanto devas difini la globalan funkcion de enigo de unu bajto:
 *   int en_l();
 *
 * $Header$
 * $Log$
 */

#ifndef lint
static char rcsid[] = "$Header$";
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "enbit.h"

static void ek_arb();

/*
 * Malpremi blokon
 */
int
b_malprem( alb, lon )
FRAZ alb; /* adreso de plenigota bufro */
NIVEL lon; /* longeco de la plenigota bufro */
{
    register NODNUM nod;
    register NIVEL i;
    register int mestis_literalo = 0;
    register NODNUM rrrr;

    if ( (mnod_d=(FRAZ *)calloc(MAKSNOD,sizeof(FRAZ))) == 0
     || (mnod_nivel=(NIVEL *)malloc(sizeof(NIVEL)*(MAKSNOD+1))) == 0
     || (mnod_patro=(NODNUM *)malloc(sizeof(NODNUM)*MAKSNOD)) == 0
#ifdef X_KOD
     || (x_mk=(struct x_mkalk *)malloc(sizeof(*x_mk)*256)) == 0
     || (x_rmlit=(LITER *)malloc(sizeof(*x_rmlit)*256)) == 0
#endif /* X_KOD */
    ){
        write( 2, "b_malprem: mankas memoro\n", 25 ); /* mankas memoro*/
        exit( 1 );
    }
    if ( setjmp( meraro ) ){
mera:
        /* redonu la memoron */
        free( (char *)mnod_d );
        free( (char *)mnod_nivel );
        free( (char *)mnod_patro );
#ifdef X_KOD
        free( (char *)x_mk );
        free( (char *)x_rmlit );
#endif /* X_KOD */
        return( -1 ); /* aperis eraro */
    }
    for ( ek_arb(alb,lon); mn_l < mfin; ){
        if ( EN_BIT() /* == FOLIKOP */ ){ /* folio aw literalo */
            EN_FLON( i );
            if ( i == 0 && !mestis_literalo ){ /* literalo */
                /* ne maks. longa ? */
                mestis_literalo = en_lit();
                continue;
            }
            /* folio */
            if ( mestis_literalo ) ++i, mestis_literalo = 0;
            EN_FNUM( rrrr, mmaksfoli );
            if ( rrrr < 0 || rrrr >= mmaksfoli ) goto mera;
            mkre_foli( mmaksfoli-rrrr-1, i );
            continue;
        }
        /* interna nodo */
        mestis_literalo = 0;
        EN_MIN_KOD( rrrr, mmaksintern );
        if ( rrrr < 0 || rrrr >= mmaksintern ) goto mera;
        nod = MAKSNOD - 1 - rrrr;                        /* nodo */
        EN_MIN_KOD( rrrr,
           mnod_nivel[nod]-mnod_nivel[mnod_patro[nod]] );/* longeco */
        mkre_nod( nod, rrrr );
    }
    /* redonu memoron */
    free( (char *)mnod_d );
    free( (char *)mnod_nivel );
    free( (char *)mnod_patro );
#ifdef X_KOD
    free( (char *)x_mk );
    free( (char *)x_rmlit );
#endif
    return( 0 );
}

static void
ek_arb( al, lon ) /* fari komencajn starigojn por la arbo */
LITER * al;
NIVEL lon;
{
    mfin = (mn_l=al) + lon;
    /* iniciatu la "nekodotan nodon" */
    mnod_nivel[MAKSNOD] = 1;
    mmaksfoli = mmaksintern = 0;
    ek_enbit();
}
