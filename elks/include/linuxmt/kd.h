#ifndef __LINUXMT_KD_H
#define __LINUXMT_KD_H

#include <linuxmt/types.h>

/*@-namechecks@*/

#define __KD_MAJ 	('K'<<8)

/*@+namechecks@*/

#define KIOCSOUND	(__KD_MAJ + 0x2f)

#endif
