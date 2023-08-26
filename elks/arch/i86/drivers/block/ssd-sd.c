/*
 * ELKS Solid State Disk generic SD card subdriver
 *
 * October 2021 Santiago Hormazabal.
 * 
 * Based on Greg Haerr SSD template driver,
 * PetitFs by ElmChan (sample init, disk read, disk write) and
 * Roland Riegel (www.roland-riegel.de) GPL2 SD Reader Library
 * (card capacity).
 *
 * Use the following command to mount a FAT16 formmated SD card:
 *	mount -t msdos /dev/ssd /mnt
 *
 * NOTE: Please create the msdos filesystem ON THE RAW card and
 * not on a partition.
 */
//#define DEBUG 1
#include <linuxmt/config.h>
#include <linuxmt/rd.h>
#include <linuxmt/major.h>
#include <linuxmt/fs.h>
#include <linuxmt/kernel.h>
#include <linuxmt/mm.h>
#include <linuxmt/errno.h>
#include <linuxmt/debug.h>
#include <linuxmt/memory.h>
#include "arch/io.h"
#include "ssd.h"
#include "spi-8018x.h"

enum SdR1Flags {
    IDLE = 1,
    ERASE_RESET = 2,
    ILLEGAL_COMMAND = 4,
    COM_CRC_ERROR = 8,
    ERASE_SEQUENCE_ERROR = 16,
    ADDRESS_ERROR = 32,
    PARAMETER_ERROR = 64
};

enum SdCommandIndices {
    CMD_GO_IDLE = 0,            /* GO_IDLE_STATE */
    CMD_SEND_IF_COND = 8,       /* SEND_IF_COND */
    CMD_SEND_CSD = 9,           /* SEND_CSD */
    CMD_SET_BLOCKLEN = 16,      /* SET_BLOCKLEN */
    CMD_READ_SINGLE_BLOCK = 17, /* READ_SINGLE_BLOCK */
    CMD_WRITE_BLOCK = 24,       /* WRITE_BLOCK */
    CMD_APP_CMD = 55,           /* APP_CMD */
    CMD_READ_OCR = 58,          /* READ_OCR */
    ACMD_SEND_OP_COND = 41,     /* SEND_OP_COND */
};

enum SdTypes {
    Unknown = 0,
    SDv1 = 1,
    SDv2 = 2,
    SDHC = 3
};

static enum SdTypes _sd_type = Unknown;


/**
 * Delays some amount of time (not exactly microseconds)
 */
static void delay_us(uint16_t n) {
    while (n--) asm("nop");
}

/**
 * Deselects the SD card by de-asserting its chip select
 * and then sends 8 dummy clocks.
 */
static void sd_release_spi(void) {
    spi_cs_1();
    spi_receive();
}

/**
 * Waits for the SD card to become ready (by returning
 * all ones while reading dummy bytes) for "counter" times.
 * If the SD card doesn't reply in time, -ETIMEDOUT is returned.
 * 
 * counter: Number of bytes to read waiting for the SD card
 * to become ready.
 * 
 * returns: 0 on success, -ETIMEDOUT if times out waiting.
 */
static int sd_wait_ready(uint16_t counter) {
    do {
        if (spi_receive() == 0xff) {
            return 0;
        }
    } while (--counter);
    return -ETIMEDOUT;
}

/**
 * Sends a command to the SD card, returning its response.
 * 
 * command: the command index (0 thru 63).
 * arg: the 32 bit argument to send.
 * 
 * returns: received response byte.
 * 
 * Note: To send app commands, use 'sd_send_acmd'.
 */
static uint8_t sd_send_cmd(uint8_t command, uint32_t arg)
{
    uint8_t ret;
    uint8_t i;

    /* Select the card */
    spi_cs_1();
    spi_receive();
    spi_cs_0();
    spi_receive();

    /* Send the command */
    spi_transmit((command & 0x3f) + 0x40);

    /* Send the argument, MSB first */
    spi_transmit((arg >> 24));
    spi_transmit((arg >> 16));
    spi_transmit((arg >> 8));
    spi_transmit(arg);

    /* hardcoded CRC values for some commands */
    if (command == CMD_GO_IDLE) {
        /* pre-calculated CRC value for the GO_IDLE_STATE command */
        spi_transmit(0x95);
    } else if (command == CMD_SEND_IF_COND) {
        /* Note: Assumes an argument of 0x1aa for this command */
        spi_transmit(0x87);
    } else {
        /* Dummy CRC value for all other commands */
        spi_transmit(0xff);
    }

    /* Give the card a some time to process the command */
    for (i = 0; ((ret = spi_receive()) & 0x80) && (i < 0x10); i++);

    return ret;
}

/**
 * Sends an application command to the SD card,
 * returning its response.
 * 
 * command: the application command index (0 thru 63).
 * arg: the 32 bit argument to send.
 * 
 * returns: received response byte.
 */
static uint8_t sd_send_acmd(uint8_t cmd, uint32_t arg)
{
    sd_send_cmd(CMD_APP_CMD, 0);
    return sd_send_cmd(cmd, arg);
}

/**
 * Sends a GO_IDLE_STATE command to the SD card.
 * 
 * returns: 0 on success, -EBUSY otherwise.
 */
static int sd_cmd_go_idle(void) {
    /* GO_IDLE_STATE */
    return sd_send_cmd(CMD_GO_IDLE, 0) == IDLE ? 0 : -EBUSY;
}

/**
 * Gets the SD version following the advised algorithm
 * on the phyisical layer specification of the SD alliance.
 * 
 * returns: 1 for SDv1, 2 for SDv2 and SDHC, -ENAVAIL when
 * the card can't be used (out of spec).
 */
static int sd_check_version(void) {
    uint8_t cmd8_voltage;
    uint8_t cmd8_check_pattern;

    /* SEND_IF_COND */
    if (sd_send_cmd(CMD_SEND_IF_COND, 0x1aa) != IDLE) {
        /* SDv1 doesn't accept CMD_SEND_IF_COND, assume it's one */
        return 1;
    }
    /* SDv2 accepts CMD_SEND_IF_COND, and should return an R7 response */

    /* skip bits 31-16 (contain "command version" + "reserved bits") */
    spi_send_ffs(2);
    
    /* only the lower 4 bits are the "accepted voltage" */
    cmd8_voltage = spi_receive();
    /* echo of our check pattern */
    cmd8_check_pattern = spi_receive();

    if ((cmd8_voltage != 0x01)
        || (cmd8_check_pattern != 0xaa)) {
        /* card shouldn't be used */
        return -ENAVAIL;
    }

    return 2;
}

/**
 * Sends a SEND_OP_COND command to leave the idle state.
 * 
 * hcs: set to non-zero to turn on the High Capacity bit,
 * required for SDHC cards.
 * 
 * returns: 0 on success, -ETIMEDOUT otherwise.
 */
static int sd_leave_idle(int hcs) {
    uint16_t counter = 1000; /* 1000 * 1000us = 1s */
    uint32_t argument = hcs ? 0x40000000 : 0;
    
    do {
        /* SEND_OP_COND */
        if (sd_send_acmd(ACMD_SEND_OP_COND, argument) == 0) {
            return 0;
        }
        delay_us(1000);
    } while(--counter);

    return -ETIMEDOUT;
}

/**
 * Checks if the SD card is SDHC or not by checking the
 * CCS bit on the OCR register.
 * 
 * returns: 1 if it's an SDHC card, 0 if not, -ETIMEDOUT otherwise.
 */
static int sd_is_hc(void) {
    uint8_t bits31_24;

    /* READ_OCR */
    if(sd_send_cmd(CMD_READ_OCR, 0) != 0) {
        return -ETIMEDOUT;
    }

    /* read bits 31 thru 24, where bit 30 is the CCS */
    bits31_24 = spi_receive();

    /* skip the rest of the OCR register */
    spi_send_ffs(3);

    /* if bit 30 is set, then this is a HC card */
    return bits31_24 & 0x40 ? 1 : 0;
}

/**
 * Sets the block length on SDv1 cards.
 * This number must be 512 so the same algorithms can be
 * used on different SD types.
 * 
 * n: block length in bytes.
 * 
 * returns: 0 on success, -EINVAL otherwise.
 */
static int sd_set_block_length(uint16_t n) {
    /* SET_BLOCKLEN */
    return sd_send_cmd(CMD_SET_BLOCKLEN, n) == 0 ? 0 : -EINVAL;
}

/**
 * Waits for a specified amount of time for a given
 * data-start token. It's set up to wait for 100mS and
 * time out if the token is not received.
 * 
 * token: expected data-start token.
 * 
 * returns: 0 if found, -ENAVAIL if another token was found,
 * -ETIMEDOUT if it times out waiting.
 */
static int sd_wait_for_token(uint8_t token) {
    uint16_t counter = 1000; /* 1000 * 100us = 0.1s */
    uint8_t rx;
    do {
        delay_us(100);
        rx = spi_receive();
        if (rx == token) {
            return 0;
        } else if (rx == 0xff) {
            continue;
        } else {
            return -ENAVAIL;
        }
    } while (--counter);

    return -ETIMEDOUT;
}

/**
 * Reads the CSD register (16 bytes) on the SD card.
 * 
 * *csd: pointer to the CSD buffer. Must be 16 bytes.
 * 
 * returns: 0 on succes, -EIO if SEND_CSD is not supported,
 * -ENAVAIL if data token wasn't found, -ETIMEDOUT if it
 * times out waiting for the data token.
 */
static int sd_read_csd(char * csd) {
    uint8_t i;
    int ret;

    /* SEND_CSD */
    if (sd_send_cmd(CMD_SEND_CSD, 0) != 0) {
        return -EIO;
    }

    ret = sd_wait_for_token(0xfe);
    if (ret) {
        return ret;
    }

    for (i = 0; i < 16; i++) {
        csd[i] = spi_receive();
    }

    return ret;
}

/**
 * Calculates the card capacity from the CSD buffer.
 * 
 * *csd: pointer to the 16 bytes CSD buffer read from the SD card.
 * *capacity: output calculated card capacity.
 * 
 * returns: 0 on success, -EINVAL otherwise.
 * 
 * NOTE: Heavily based off Roland Riegel's sd_raw.c of his SD Reader
 * library.
 */
static int sd_get_capacity_from_csd(uint8_t * csd, unsigned long * capacity) {
    uint8_t csd_read_bl_len = 0;
    uint8_t csd_c_size_mult = 0;
    uint32_t csd_c_size = 0;
    uint8_t csd_structure = 0;

    csd_structure = csd[0] >> 6;

    if (csd_structure == 0x01) {
        // 7
        csd_c_size <<= 8;
        csd_c_size |= csd[7] & 0x3f;
        // 8
        csd_c_size <<= 8;
        csd_c_size |= csd[8];
        // 9
        csd_c_size <<= 8;
        csd_c_size |= csd[9];
        ++csd_c_size;

        *capacity = csd_c_size * SD_FIXED_SECTOR_SIZE * 1024UL;
    } else if (csd_structure == 0x00) {
        // 5
        csd_read_bl_len = csd[5] & 0x0f;
        // 6
        csd_c_size = csd[6] & 0x03;
        csd_c_size <<= 8;
        // 7
        csd_c_size |= csd[7];
        csd_c_size <<= 2;
        // 8
        csd_c_size |= csd[8] >> 6;
        ++csd_c_size;
        // 9
        csd_c_size_mult = csd[9] & 0x03;
        csd_c_size_mult <<= 1;
        // 10
        csd_c_size_mult |= csd[10] >> 7;

        *capacity = (uint32_t) csd_c_size << (csd_c_size_mult + csd_read_bl_len + 2);
    } else {
        return -EINVAL;
    }

    return 0;
}

/**
 * Initializes an SD card.
 * 
 * card_size: returns the card size in bytes.
 * 
 * returns: 0 on success, negative value on failure.
 */
static int sd_initialize(unsigned long * card_size) {
    uint8_t buf[16];
    int ret;
    int version;

    spi_cs_1();
    /* Send 800 dummy clocks while deasserted */
    spi_send_ffs(10);

    /* Enter Idle state */
    ret = sd_cmd_go_idle();
    if (ret) {
        goto fail;
    }

    ret = sd_check_version();
    if (ret < 0) {
        _sd_type = Unknown;
        goto fail;
    }
    version = ret;

    ret = sd_leave_idle(version == 2 ? 1 : 0);
    if (ret) {
        goto fail;
    }

    if (version == 2) {
        /* SDv2 or SDHC */
        ret = sd_is_hc();
        if (ret < 0) {
            goto fail;
        } else if (ret) {
            _sd_type = SDHC;
        } else {
            _sd_type = SDv2;
        }
    } else {
        /* SDv1 or MMC (not supported) */
        ret = sd_set_block_length(SD_FIXED_SECTOR_SIZE);
        if (ret) {
            goto fail;
        }
        _sd_type = SDv1;
    }

    /* Read CSD */
    ret = sd_read_csd((char*)buf);
    if (ret) {
        goto fail;
    }

    /* return card size in bytes */
    if (card_size) {
        ret = sd_get_capacity_from_csd(buf, card_size);
        if (ret) {
            goto fail;
        }
    }

    ret = 0;

fail:
    sd_release_spi();

    return ret;
}

/**
 * Reads an entire sector off the SD card.
 * 
 * *buffer: __far pointer of the buffer to be written with the
 * SD card's contents. Must be 512 bytes long.
 * sector: sector number.
 * 
 * returns: 0 on success, negative value on failure.
 */
static int sd_read(char *buf, ramdesc_t seg, sector_t sector) {
    /* init far pointer to segment:offset. FIXME won't work with XMS buffers*/
    unsigned char __far *buffer = _MK_FP((seg_t)seg, (unsigned)buf);
    int ret;
    uint16_t count = SD_FIXED_SECTOR_SIZE;

    if (_sd_type != SDHC) {
        /* SDv1 and SDv2 use address instead of sectors */
        sector *= SD_FIXED_SECTOR_SIZE;
    }

    /* READ_SINGLE_BLOCK */
    if (sd_send_cmd(CMD_READ_SINGLE_BLOCK, sector) != 0) {
        ret = -EIO;
        goto fail;
    }

    ret = sd_wait_for_token(0xfe);
    if (ret < 0) {
        goto fail;
    }

    do {
        *buffer++ = spi_receive();
    } while (--count);

    /* Skip trailing CRC */
    spi_send_ffs(2);

fail:
    sd_release_spi();

    return ret;
}

/**
 * Writes an entire sector off the SD card.
 * 
 * *buffer: __far pointer of the buffer containing data to be
 * written on the SD card. Must be 512 bytes long.
 * sector: sector number.
 * 
 * returns: 0 on success, negative value on failure.
 */
static int sd_write(char *buf, ramdesc_t seg, sector_t sector) {
    /* init far pointer to segment:offset. FIXME won't work with XMS buffers*/
    unsigned char __far *buffer = _MK_FP((seg_t)seg, (unsigned)buf);
    int ret;
    uint16_t count = SD_FIXED_SECTOR_SIZE;

    if (_sd_type != SDHC) {
        /* SDv1 and SDv2 use address instead of sectors */
        sector *= SD_FIXED_SECTOR_SIZE;
    }

    /* WRITE_SINGLE_BLOCK */
    if (sd_send_cmd(CMD_WRITE_BLOCK, sector) != 0) {
        ret = -EIO;
        goto fail;
    }

    /* Send data block header */
    spi_transmit(0xff);
    spi_transmit(0xfe);

    /* Send the entire sector data */
    do {
        spi_transmit(*buffer++);
    } while (--count);

    /* Send a dummy CRC */
    spi_transmit(0);
    spi_transmit(0);

    /* Check data response */
    if ((spi_receive() & 0x1F) != 0x05) {
        ret = -EIO;
        goto fail;
    }

    /* wait for the SD card to be ready */
    ret = sd_wait_ready(10000);

fail:
    sd_release_spi();

    return ret;
}

/**
 * SSD API: Init this ssd device.
 * 
 * returns: 0 on failure, size of the card in 1K blocks on success.
 */
sector_t ssddev_init(void) {
    unsigned long card_size;
    int ret;

    /* low level initialization of the SPI pins */
    spi_init_ll();

    /* init the actual SD card */
    ret = sd_initialize(&card_size);
    
    if (!ret) {
        debug("ssdev_init SD card found, type %d, size %ld\n", _sd_type, card_size);
        ssd_initialized = 1;
        return card_size * 2UL;
    } else {
        debug("ssdev_init error initializing SD card (%d)\n", ret);
        return 0;
    }
}

/**
 * SSD API: ioctl.
 * 
 * returns: -EINVAL since it's not used.
 */
int ssddev_ioctl(struct inode *inode, struct file *file,
            unsigned int cmd, unsigned int arg)
{
    /* not used */
    return -EINVAL;
}

/**
 * SSD API: Writes one sector to the SD card.
 * 
 * returns: # sectors written.
 */
int ssddev_write(sector_t start, char *buf, ramdesc_t seg)
{
    int ret;

    ret = sd_write(buf, seg, start);
    if (ret != 0)       /* error, no sectors were written */
        return 0;
    return 1;
}

/**
 * SSD API: Reads one sector from the SD card.
 * 
 * returns: # sectors read.
 */
int ssddev_read(sector_t start, char *buf, ramdesc_t seg)
{
    int ret;

    ret = sd_read(buf, seg, start);
    if (ret != 0)       /* error, no sectors were read */
        return 0;
    return 1;
}
