#ifdef ZX81_RNG
/*
 * This is my favorite tiny RNG, If you had a ZX81 you may recognise it :-)
 *								(RdeBath)
 */

#include <stdlib.h>

#define MAXINT (((unsigned)-1)>>1)

static unsigned int sseed = 0;

int rand()
{
   return ( sseed = (((sseed+1L)*75L)%65537L)-1 ) & MAXINT;
}

void srand(seed)
unsigned int seed;
{
   sseed=seed;
}

#else

/*
 * This generator is a combination of three linear congruential generators
 * with periods or 2^15-405, 2^15-1041 and 2^15-1111. It has a period that
 * is the product of these three numbers.
 */

static int seed1 = 1;
static int seed2 = 1;
static int seed3 = 1;
#define MAXINT (((unsigned)-1)>>1)

#define CRANK(a,b,c,m,s) 	\
	q = s/a;		\
	s = b*(s-a*q) - c*q;	\
	if(s<0) s+=m;

int rand()
{
   register int q, z;
   CRANK(206, 157,  31, 32363, seed1);
   CRANK(217, 146,  45, 31727, seed2);
   CRANK(222, 142, 133, 31657, seed3);

   return seed1^seed2^seed3;
}

void srand(seed)
unsigned int seed;
{
   seed &= MAXINT;
   seed1= seed%32362 + 1;
   seed2= seed%31726 + 1;
   seed3= seed%31656 + 1;
}

#endif
