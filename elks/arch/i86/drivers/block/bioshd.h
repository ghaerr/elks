#ifndef _BIOSHD_H
#define _BIOSHD_H

/* the following must match with /dev minor numbering scheme*/
#define NUM_MINOR       8       /* max minor devices per drive*/
#define MINOR_SHIFT     3       /* =log2(NUM_MINOR) shift to get drive num*/
#define MAX_DRIVES      8       /* <=256/NUM_MINOR*/
#define DRIVE_HD0       0
#define DRIVE_FD0       4       /* =MAX_DRIVES/2 first floppy drive*/

#define HD_DRIVES       4       /* max hard drives */
#ifdef CONFIG_ARCH_PC98
#define FD_DRIVES       4
#else
#define FD_DRIVES       2       /* max floppy drives */
#endif
#define NUM_DRIVES      (HD_DRIVES+FD_DRIVES) /* max number of drives (<=256/NUM_MINOR) */

#endif
