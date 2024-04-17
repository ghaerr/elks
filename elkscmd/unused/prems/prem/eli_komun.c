/*
 * Komunaj dataoj por eligi bitojn
 *
 * $Header$
 * $Log$
 */

#include "elbit.h"

union elb_b elb_bb; /* eliga bufro por bitoj */
int elb_nb; /* kvanto da bitoj en la bufro */

#ifdef DIAP_KOD
unsigned int
del_Dia; /* maksimuma diapazono por diapazonigita kodo (0, 2, 4) */
unsigned int del_start;
#endif

#ifdef X_KOD
struct x_kalk * x_k /* [256] */, ** x_indeks /* [256] */;
LITER * x_rindeks /* [256] */;
#endif
