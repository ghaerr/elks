#pragma once

#define PROGRAMMER_VERSION_MAJOR 1
#define PROGRAMMER_VERSION_MINOR 1
#define PROGRAMMER_ADDRESS_BITS  18
#define PROGRAMMER_ROM_SIZE      (2UL<<(PROGRAMMER_ADDRESS_BITS))
#define PROGRAMMER_MAX_LEN       128
#define PROGRAMMER_SECTOR_SIZE   4096UL
#define PROGRAMMER_BAUD          B38400