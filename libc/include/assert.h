#ifndef	__ASSERT_H
#define	__ASSERT_H
#include <features.h>

/* If NDEBUG is defined, do nothing.
   If not, and EXPRESSION is zero, print an error message and abort.  */

#ifdef	NDEBUG

#define	assert(expr)		((void) 0)

#else /* Not NDEBUG.  */

extern void __assert __P((const char *, const char *, int));

#define	assert(expr)							      \
  ((void) ((expr) ||							      \
	   (__assert (__STRING(expr),				      \
			   __FILE__, __LINE__), 0)))

#endif /* NDEBUG.  */

#endif /* __ASSERT_H */
