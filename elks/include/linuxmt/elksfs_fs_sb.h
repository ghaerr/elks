#ifndef __ELKSFS_FS_SB__
#define __ELKSFS_FS_SB__

/*
 * elksfs super-block data in memory
 */

struct elksfs_sb_info
{
    unsigned long s_ninodes;
    unsigned long s_nzones;
    unsigned long s_imap_blocks;
    unsigned long s_zmap_blocks;
    unsigned long s_firstdatazone;
    unsigned long s_log_zone_size;
    unsigned long s_max_size;
    struct buffer_head *s_imap[8];
    struct buffer_head *s_zmap[64];
    unsigned long s_dirsize;
    unsigned long s_namelen;
    struct buffer_head *s_sbh;
    struct elksfs_super_block *s_ms;
    unsigned short s_mount_state;
    unsigned short s_version;
};

#endif
