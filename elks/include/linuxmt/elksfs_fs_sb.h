#ifndef LX86_LINUXMT_FS_SB_H
#define LX86_LINUXMT_FS_SB_H

#include <linuxmt/types.h>

/*
 * elksfs super-block data in memory
 */

struct elksfs_sb_info {
    ino_t			s_ninodes;
    block_t			s_nzones;
    unsigned long int		s_imap_blocks;
    unsigned long int		s_zmap_blocks;
    block_t			s_firstdatazone;
    unsigned long int		s_log_zone_size;
    unsigned long int		s_max_size;
    struct buffer_head		*s_imap[8];
    struct buffer_head		*s_zmap[64];
    unsigned long int		s_dirsize;
    unsigned long int		s_namelen;
    struct buffer_head		*s_sbh;
    struct elksfs_super_block	*s_ms;
    unsigned short int		s_mount_state;
    unsigned short int		s_version;
};

#endif
