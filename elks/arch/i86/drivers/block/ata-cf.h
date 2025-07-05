#ifndef _ATA_CF_H
#define _ATA_CF_H

#define MAJOR_NR        ATHD_MAJOR

/* the following must match with BIOSHD /dev minor numbering scheme*/
#define NUM_MINOR       8       /* max minor devices per drive*/
#define MINOR_SHIFT     3       /* =log2(NUM_MINOR) shift to get drive num*/
#define MAX_DRIVES      8       /* <=256/NUM_MINOR*/

#define NUM_DRIVES      2

#endif
