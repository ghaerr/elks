/*
 * Funkcioj por eligi kodojn pobite
 *
 * $Header$
 * $Log$
 */

#include "elbit.h"

/* iniciati eligon */
void
ek_elbit()
{
    elb_nb = 0;
#ifdef DIAP_KOD
    del_Dia = 21; /* por kodo ( 0, 2, 4 ) */
    del_start = 1;
#endif
#ifdef X_KOD
    ek_x_kod();
#endif
}

/* eligi la restajhojn el la bita bufro */
void
fin_elbit()
{
    if ( elb_nb ){
        elb_bb.bb_i <<= 16-elb_nb;
        el_l( elb_bb.bb_c.su );
        if ( elb_nb > 8 ) el_l( elb_bb.bb_c.ms );
    }
}
