/*
   This file is the interface to the rest of rc for any locally
   defined addon builtins.  By default there are none.
   The interface consists of the following macro.
  
   ADDONS	A comma-separated list of pairs of function pointers
		and string literals.
  
   The addon functions must also have proper prototypes in this file.
   The builtins all have the form:
  
	void b_NAME(char **av);
  
   Builtins report their exit status using set(TRUE) or set(FALSE).
  
   Example:
  
	#define ADDONS	{ b_test, "test" },
	extern void b_test(char **av);
*/

#define ADDONS		/* no addons by default */

#ifdef	DWS

/*
   This is what DaviD Sanderson (dws@cs.wisc.edu) uses.
*/

#undef	ADDONS
#define ADDONS	{ b_access,	"access" },\
		{ b_test, 	"test" },\
		{ b_test,	"[" },

extern void b_access(char **av);
extern void b_test(char **av);

#endif
