/*	mms.h - definitions for the Linux/MT Memory Manager Switch (tm)
	(C) 1995 Chad Page

	These definitions are *not* arch-specific, although each arch could
	have different mms implementations in the same build (for instance,
	the 80x86/16bit arch tree could have RealMode, EMS, and DPMI MMS 
	handlers...
*/

/* The __hMMS 'structure' is actually a void pointer so any handler can
 * use it's own structure */

typedef void * __hMMS;

struct _mms_globalheapops 
{
	__hMMS _VFUNCTION(init, ());
}; 

typedef _mms_gheapops __MMS_gheapops;
typedef _mms_gheapops __pMMS_gheapops;

