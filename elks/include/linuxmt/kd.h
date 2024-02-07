#ifndef __LINUXMT_KD_H
#define __LINUXMT_KD_H

#ifndef __ASSEMBLER__
# include <linuxmt/types.h>
#endif

#define __KD_MAJ 	('K'<<8)

#define KIOCSOUND	(__KD_MAJ + 0x2f)

#endif
