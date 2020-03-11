/*
 * Difinoj por uzi la programojn de bloka premo/malpremo
 *
 * $Header: prem.h,v 1.1 91/01/15 22:36:05 pro Exp $
 * $Log:	prem.h,v $
 * Revision 1.1  91/01/15  22:36:05  pro
 * Initial revision
 * 
 * 
 */

#define LITER unsigned char

/* premi blokon - redonas longecon de premita bloko se sukcesis  */
/* negativan nombron se fiaskis                                  */
extern int
b_prem( /* LITER * el, */          /* premotaj dataoj            */
        /* unsigned int el_lon, */ /* origina longeco            */
        /* LITER * al */        ); /*  bloko por la rezulto      */
                                   /* (samlonga kun la origina)  */

/* malpremi - redonas 0 se sukcesis,                             */
/* negativan nombron se estas eraro en la dataoj                 */
extern int
b_malprem( /* LITER * al, */             /* bloko por la rezulto */
           /* unsigned int el_lon */ );  /* la ORIGINA longeco   */

/* por malpremado LA UZANTO devas aranghi */
/* la funkcion por enigi vican bajton     */
extern int en_l();
