/*
 * Funkcioj por enigi kodojn pobite
 *
 * $Header$
 * $Log$
 */

#include "enbit.h"

/* iniciati enigon */
void
ek_enbit()
{
    enb_nb = 0;
#ifdef DIAP_KOD
    den_Dia = 21; /* por diapazonigita kodo (0, 2, 4) */
    den_start = 1;
#endif
#ifdef X_KOD
    ek_x_malkod();
#endif
}
