/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _I86_DRIVERS_BLOCK_MFMHD_H
#define _I86_DRIVERS_BLOCK_MFMHD_H

/* Current ELKS port: G Keet <gatekeeper@xt-emporium.com>, 2026. */

/*
 * User-visible WD1002/Xebec-style MFM controller control codes.
 *
 * These are intentionally simple numeric ioctl values because this ELKS tree
 * does not provide Linux _IO/_IOR/_IOW helpers.  Keep them private to the MFM
 * block major; generic block ioctls such as HDIO_GETGEO are handled normally.
 */
#define MFMHDIOC_BASE           0x6d00U
#define MFMHDIOC_GETREGS        (MFMHDIOC_BASE + 0x01U)
#define MFMHDIOC_GETSENSE       (MFMHDIOC_BASE + 0x02U)
#define MFMHDIOC_RECALIBRATE    (MFMHDIOC_BASE + 0x03U)
#define MFMHDIOC_SEEK           (MFMHDIOC_BASE + 0x04U)
#define MFMHDIOC_VERIFY         (MFMHDIOC_BASE + 0x05U)
#define MFMHDIOC_CTRL_DIAG      (MFMHDIOC_BASE + 0x06U)
#define MFMHDIOC_BUFFER_DIAG    (MFMHDIOC_BASE + 0x07U)
#define MFMHDIOC_RESET          (MFMHDIOC_BASE + 0x08U)
#define MFMHDIOC_GETOPTS        (MFMHDIOC_BASE + 0x09U)
#define MFMHDIOC_SETOPTS        (MFMHDIOC_BASE + 0x0aU)
#define MFMHDIOC_READ_BUFFER    (MFMHDIOC_BASE + 0x0bU)
#define MFMHDIOC_WRITE_BUFFER   (MFMHDIOC_BASE + 0x0cU)
#define MFMHDIOC_TEST_READY     (MFMHDIOC_BASE + 0x0dU)
#define MFMHDIOC_DRIVE_DIAG     (MFMHDIOC_BASE + 0x0eU)
#define MFMHDIOC_FORMAT_DRIVE   (MFMHDIOC_BASE + 0x0fU)
#define MFMHDIOC_FORMAT_TRACK   (MFMHDIOC_BASE + 0x10U)
#define MFMHDIOC_FORMAT_BAD_TRACK (MFMHDIOC_BASE + 0x11U)
#define MFMHDIOC_INIT_PARAMS    (MFMHDIOC_BASE + 0x12U)
#define MFMHDIOC_READ_ECC_BURST (MFMHDIOC_BASE + 0x13U)
#define MFMHDIOC_READ_LONG      (MFMHDIOC_BASE + 0x14U)
#define MFMHDIOC_WRITE_LONG     (MFMHDIOC_BASE + 0x15U)

/* Hardware status register bits, port base+1. */
#define MFMHD_STAT_READY        0x01U   /* controller ready for next byte */
#define MFMHD_STAT_INPUT        0x02U   /* 1=controller->host, 0=host->controller */
#define MFMHD_STAT_COMMAND      0x04U   /* 1=command/status, 0=data */
#define MFMHD_STAT_SELECT       0x08U   /* controller selected */
#define MFMHD_STAT_REQUEST      0x10U   /* DMA/request line */
#define MFMHD_STAT_INTERRUPT    0x20U   /* interrupt pending */

/* Compatibility aliases for early userspace tools. */
#define MFMHD_STAT_REQ          MFMHD_STAT_READY
#define MFMHD_STAT_IO           MFMHD_STAT_INPUT
#define MFMHD_STAT_CD           MFMHD_STAT_COMMAND
#define MFMHD_STAT_BSY          MFMHD_STAT_SELECT
#define MFMHD_STAT_DRQ          MFMHD_STAT_REQUEST
#define MFMHD_STAT_IRQ          MFMHD_STAT_INTERRUPT

/* Port base+3 interrupt/DMA mask register bits. */
#define MFMHD_CTL_DRQEN         0x01U
#define MFMHD_CTL_IRQEN         0x02U
#define MFMHD_CTL_PIO_POLLED    0x00U

/* Six-byte command block byte 5: R1, R2, reserved zeroes, step code. */
#define MFMHD_CCB_R1_NO_RETRY       0x80U
#define MFMHD_CCB_R2_IMMEDIATE_ECC  0x40U
#define MFMHD_CCB_STEP_MASK         0x07U
#define MFMHD_CCB_RESERVED_MASK     0x38U

#define MFMHD_STEP_3MS_PREFERRED    0U
#define MFMHD_STEP_3MS_ALT          1U
#define MFMHD_STEP_60US             2U
#define MFMHD_STEP_18US_ALT         3U
#define MFMHD_STEP_200US            4U
#define MFMHD_STEP_70US             5U
#define MFMHD_STEP_30US             6U
#define MFMHD_STEP_18US             7U

/* WD1002/Xebec command opcodes. */
#define MFMHD_CMD_TEST_DRIVE_READY      0x00U
#define MFMHD_CMD_RECALIBRATE           0x01U
#define MFMHD_CMD_READ_STATUS           0x03U
#define MFMHD_CMD_FORMAT_DRIVE          0x04U
#define MFMHD_CMD_VERIFY_SECTORS        0x05U
#define MFMHD_CMD_FORMAT_TRACK          0x06U
#define MFMHD_CMD_FORMAT_BAD_TRACK      0x07U
#define MFMHD_CMD_READ_SECTORS          0x08U
#define MFMHD_CMD_WRITE_SECTORS         0x0aU
#define MFMHD_CMD_SEEK                  0x0bU
#define MFMHD_CMD_INIT_DRIVE_PARAMETERS 0x0cU
#define MFMHD_CMD_READ_ECC_BURST        0x0dU
#define MFMHD_CMD_READ_SECTOR_BUFFER    0x0eU
#define MFMHD_CMD_WRITE_SECTOR_BUFFER   0x0fU
#define MFMHD_CMD_BUFFER_DIAGNOSTIC     0xe0U
#define MFMHD_CMD_DRIVE_DIAGNOSTIC      0xe3U
#define MFMHD_CMD_CONTROLLER_DIAGNOSTIC 0xe4U
#define MFMHD_CMD_READ_LONG             0xe5U
#define MFMHD_CMD_WRITE_LONG            0xe6U
#define MFMHD_CMD_ST11_GET_GEOMETRY     0xf8U

struct mfmhd_ioc_regs {
    unsigned short port;
    unsigned char status;
    unsigned char jumper;
    unsigned char control;
    unsigned char last_csb;
    unsigned char last_sense[4];
    unsigned int retries;
    unsigned int cmd_errors;
    unsigned int io_errors;
};

struct mfmhd_ioc_sense {
    unsigned char bytes[4];
    unsigned char csb;
};

struct mfmhd_ioc_opts {
    unsigned char cmd_control;      /* command-block byte 5; reserved bits masked */
    unsigned char port_control;     /* port base+3 IRQ/DMA mask byte */
    unsigned char force_single;     /* non-zero forces single-sector I/O */
    unsigned char reserved;
};

struct mfmhd_ioc_chs {
    unsigned short cylinder;
    unsigned char head;
    unsigned char sector;
    unsigned char count;
    unsigned char cmd_control;      /* command-block byte 5; reserved bits masked */
    unsigned char csb;
    unsigned char sense[4];
};

struct mfmhd_ioc_long {
    unsigned short cylinder;
    unsigned char head;
    unsigned char sector;
    unsigned char count;
    unsigned char cmd_control;
    unsigned char csb;
    unsigned char sense[4];
    unsigned char data[516];
};

struct mfmhd_ioc_buffer {
    unsigned char data[512];
    unsigned char csb;
    unsigned char sense[4];
};

#endif /* _I86_DRIVERS_BLOCK_MFMHD_H */
