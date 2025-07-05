#ifndef _ATA_H
#define _ATA_H

#include <linuxmt/memory.h>

#define ATA_RETRY           10
#define ATA_SECTOR_SIZE     512
#define ATA_TIMEOUT         1024

void ata_reset(void);
sector_t ata_init(unsigned int drive);
int ata_read(unsigned int drive, sector_t sector, char *buf, ramdesc_t seg);
int ata_write(unsigned int drive, sector_t sector, char *buf, ramdesc_t seg);

#endif
