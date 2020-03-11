/*
 * Difinoj por uzado de Patricia-arbo por frazaro
 *
 * $Header$
 * $Log$
 */

#ifdef lint
#define void int
#endif

#include <alloc.h>

#define MAKSNOD   4096   /* maksimuma kvanto da nodoj en la arbo */
#define NEKODOT   256    /* 256 nodoj kun nivelo==1,             */
                         /* kiuj neniam estas kodataj            */

#define FOLIO     32767
                  /* signo de fina parto de la frazo, */
                  /* neeble granda valoro por longeco */
                  /* de la premotaj dataoj            */

typedef unsigned char LITER;  /* tipo de literoj */

typedef LITER *        FRAZ;  /* direktilo al frazo */
typedef int          NODNUM;  /* numero de nodo */
typedef int           NIVEL;  /* longeco de frazo, nivelo de nodo */
