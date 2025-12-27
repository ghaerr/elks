#ifndef NODEBUG				/* NODEBUG inhibits debugging */
#ifndef DEBUG				/* and if DEBUG not already defined */
#ifndef MINSIZE				/* MINSIZE inhibits debugging */
#ifndef DEBUG
#define DEBUG
#endif /* DEBUG */
#endif /* MINSIZE */
#endif /* DEBUG */
#endif /* NODEBUG */

#ifdef DEBUG				/* Debugging included... */
/* dodebug() function codes... */
#define DB_OPN 1			/* Open log */
#define DB_LOG 2			/* Write label+string or int to log */
#define DB_MSG 3			/* Write message to log */
#define DB_CHR 4			/* Write label + char to log */
#define DB_PKT 5			/* Record a Kermit packet in log */
#define DB_CLS 6			/* Close log */

void dodebug(int, UCHAR *, UCHAR *, long); /* Prototype */
/*
  dodebug() is accessed throug a macro that:
   . Coerces its args to the required types.
   . Accesses dodebug() directly or thru a pointer according to context.
   . Makes it disappear entirely if DEBUG not defined.
*/
#ifdef KERMIT_C
/* In kermit.c we debug only through a function pointer */
#define debug(a,b,c,d) \
if(*(k->dbf))(*(k->dbf))(a,(UCHAR *)b,(UCHAR *)c,(long)(d))

#else  /* KERMIT_C */
/* Elsewhere we can call the debug function directly */
#define debug(a,b,c,d) dodebug(a,(UCHAR *)b,(UCHAR *)c,(long)(d))
#endif /* KERMIT_C */

#else  /* Debugging not included... */

#define debug(a,b,c,d)
#endif /* DEBUG */
