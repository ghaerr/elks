#ifndef __LINUXMT_MINIX_FS_SB_H
#define __LINUXMT_MINIX_FS_SB_H

/*
 * minix super-block data in memory
 */
#include <linuxmt/minix_fs.h>

struct minix_sb_info {
    unsigned short		s_ninodes;
    unsigned short		s_nzones;
    unsigned short		s_imap_blocks;
    unsigned short		s_zmap_blocks;
    unsigned short		s_firstdatazone;
    unsigned short		s_log_zone_size;
    unsigned long		s_max_size;
    block_t                     s_imap[MINIX_I_MAP_SLOTS];
    block_t                     s_zmap[MINIX_Z_MAP_SLOTS];
    unsigned short		s_dirsize;
    unsigned short		s_namelen;
    unsigned short		s_mount_state;
};

#endif
