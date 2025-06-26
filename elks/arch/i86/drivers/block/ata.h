#ifndef _ATA_H
#define _ATA_H

#include <linuxmt/sched.h>
#include <linuxmt/kdev_t.h>

#define ATA_SECTOR_SIZE 512

sector_t ata_init(unsigned int drive);
int ata_read(unsigned int drive, sector_t sector, char *buf, ramdesc_t seg);
int ata_write(unsigned int drive, sector_t sector, char *buf, ramdesc_t seg);

#endif
