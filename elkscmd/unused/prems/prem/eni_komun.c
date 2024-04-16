/*
 * Komunaj dataoj por enigi bitojn
 *
 * $Header$
 * $Log$
 */

#include "enbit.h"

char enb_bb; /* bita bufro */
int  enb_nb; /* montrilo de bito en la bufro */

#ifdef DIAP_KOD
unsigned int
den_Dia; /* la diapazono por diapazonigita unuara kodo */
unsigned int den_start;
#endif

#ifdef X_KOD
struct x_mkalk * x_mk /* [256] */; /* tabelo de frekvencoj */
LITER * x_rmlit /* [256] */; /* tabelo de nerenkontighintaj literoj */
#endif
