/*
 * Serchi frazon en la Patricia-arbo,
 * redonas adreson de vica enira litero.
 * Aldonas la frazon al la frazaro.
 * Starigas numeron de kodota nodo
 * kaj longecon en la nodo.
 *
 * Eblaj rezultoj:
 *   1. enira fluo finighas pli frue ol la koincido
 *        ni kodigu la nodon ghis la fino de la fluo
 *   2. koincido finighas inter nodoj (ne estas sekva nodo malsupren)
 *        ni kodigu la nodon ghis la fino
 *        kreu folion post la nodo
 *   3. koincido finighas en nodo
 *        ni kodigu la nodon ghis la fino de la koincido
 *        kreu nodon kun intera nivelo
 *        transportu la kodatan nodon al la fino de la intera nodo
 *        kreu folion post la intera nodo
 *
 * $Header$
 * $Log$
 */

#include "elarbo.h"

FRAZ
serch()
{
    register FRAZ       n; /* nuna litero */
             FRAZ      on;
    register HASHNUM  nod;
             HASHNUM onod, oonod;
    register FRAZ       f; /* fina pozicio en la nuna nodo */
    register FRAZ       l; /* litero */

    if ( nod_d[onod = HDIM + *n_l] == 0 ) /* se necesas, tuj kreu */
        nod_d[onod] = n_l;               /* la komencan nodon    */
    if ( (n=n_l+1) < fin ) for ( ;
                               (nod=sekv_niv(onod,*n)) >= 0;
                               oonod = onod, onod = nod ){
        /* koincidis unu vica litero */
        on = n; /* memorfiksu la komencon de noda frazparto */
        if ( ++n == fin ) break; /* al la sekva litero */
        if ( nod_nivel[nod] == n-n_l ) continue; /* unulitera nodo */
        /* eble estas plia koincido */
        l = nod_d[nod] + (n - n_l);
        f = nod_d[nod] + nod_nivel[nod];
        for ( ; l != f && n != fin && *n == *l ; ++n, ++l ) ;
        if ( n == fin ) break;
        if ( l == f ) continue; /* plena koincido */
        /* 3. ne plena koincido en chi nodo */
        nod_lon = n - on;
        maks_lon = nod_nivel[nod] - nod_nivel[onod];
        /* kreu nodon kun intera nivelo kaj la folion */
        kod_nod = kre_nod( nod, (NIVEL)(n-n_l) );
        return( n );
    }
    /* 1. elcherpighis la eniraj literoj */
    /*   aw                              */
    /* 2. ne estas sekva nodo            */
    if ( n == fin ){ /* elcherpighis la eniraj literoj */
        if ( n - n_l > 1 ){ /* estas kion kodigi */
            kod_nod = nod;
            maks_lon = nod_nivel[nod] - nod_nivel[onod];
            if ( (nod_lon=n-on) == maks_lon ) nod_lon = 0;
        }
    } else { /* ne estas la sekva nodo */
        if ( n - n_l > 1 ){ /* estas kion kodigi */
            kod_nod = onod;
            maks_lon = nod_nivel[onod] - nod_nivel[oonod];
            nod_lon = 0;
        }
        kre_hfoli( onod );
    }
    return( n );
}
