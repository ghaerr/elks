//------------------------------------------------------------------------------
// Code and data structures for booting ELKS from a FAT12/16 filesystem
//------------------------------------------------------------------------------

// boot_sect.S will use these macros if configured to be a FAT12/16 bootloader

.macro FAT_BPB
#if defined(CONFIG_IMG_FD360)
	.set FAT_SEC_PER_CLUS, 2
	.set FAT_ROOT_ENT_CNT, 112
	.set FAT_TOT_SEC, 720
	.set FAT_MEDIA_BYTE, 0xfd
	.set FAT_TABLE_SIZE, 2
	.set FAT_SEC_PER_TRK, 9
	.set FAT_NUM_HEADS, 2
#elif defined(CONFIG_IMG_FD720)
	.set FAT_SEC_PER_CLUS, 2
	.set FAT_ROOT_ENT_CNT, 112
	.set FAT_TOT_SEC, 1440
	.set FAT_MEDIA_BYTE, 0xf9
	.set FAT_TABLE_SIZE, 3
	.set FAT_SEC_PER_TRK, 9
	.set FAT_NUM_HEADS, 2
#elif defined(CONFIG_IMG_FD1200)
	.set FAT_SEC_PER_CLUS, 1
	.set FAT_ROOT_ENT_CNT, 224
	.set FAT_TOT_SEC, 2400
	.set FAT_MEDIA_BYTE, 0xf9
	.set FAT_TABLE_SIZE, 7
	.set FAT_SEC_PER_TRK, 15
	.set FAT_NUM_HEADS, 2
#elif defined(CONFIG_IMG_FD1440)
	.set FAT_SEC_PER_CLUS, 1
	.set FAT_ROOT_ENT_CNT, 224
	.set FAT_TOT_SEC, 2880
	.set FAT_MEDIA_BYTE, 0xf0
	.set FAT_TABLE_SIZE, 9
	.set FAT_SEC_PER_TRK, 18
	.set FAT_NUM_HEADS, 2
#else
	.error "Unknown disk medium!"
#endif

// BIOS Parameter Block (BPB) for FAT filesystems, as laid out in
//
//	Microsoft Corporation.  _Microsoft Extensible Firmware Initiative
//	FAT32 File System Specification: FAT: General Overview of On-Disk
//	Format_.  2000.
//
// or alternatively
//
//	Microsoft Corporation.  _Microsoft FAT Specification_.  2004.

	.global sect_max
	.global head_max

bs_oem_name:				// OEM name
	.ascii "MSWIN4.1"
bpb_byts_per_sec:			// Bytes per sector
	.word 0x200
bpb_sec_per_clus:			// Sectors per cluster
	.byte FAT_SEC_PER_CLUS
bpb_rsvd_sec_cnt:			// Reserved sectors, including this one
	.word 1
bpb_num_fats:				// Number of duplicate file allocation
	.byte 2				// tables
bpb_root_ent_cnt:			// Number of root directory entries
	.word FAT_ROOT_ENT_CNT
bpb_tot_sec_16:				// Total number of sectors, 16-bit
	.word FAT_TOT_SEC
bpb_media:				// Media byte
	.byte FAT_MEDIA_BYTE
bpb_fat_sz_16:				// Sectors per FAT
	.word FAT_TABLE_SIZE
bpb_sec_per_trk:			// Sectors per track
sect_max:
	.word FAT_SEC_PER_TRK
bpb_num_heads:				// Number of heads
head_max:
	.word FAT_NUM_HEADS
bpb_hidd_sec:				// Hidden sectors
	.long 0
.endm

.macro FAT_LOAD_KERNEL
	.set buf, entry + 0x200

	// Load the first sector of the root directory
	movb bpb_num_fats,%al
	cbtw
	mulw bpb_fat_sz_16
	addw bpb_rsvd_sec_cnt,%ax
	push %ax
	inc %dx				// Assume DX = 0; if DX != 0 then we
					// are in trouble anyway
	mov $buf,%cx
	push %cx
	push %ss
	call disk_read

	// See if the first directory entry is /linux; bail out if not
	pop %si				// SI = buf
	mov $kernel_name,%di
	mov $0xb,%cx
	repz
	cmpsb
	jnz no_system

	// Load consecutive sectors starting from the first file cluster
	// Calculate starting sector
	mov (0x1a-0xb)(%si),%ax		// First cluster number
	dec %ax				// Compute no. of sectors from the
	dec %ax				// disk data area start
	mov bpb_sec_per_clus,%cl	// CH = 0
	mul %cx
	pop %bx				// BX = root directory logical sector
					// we calculated before
	add %ax,%bx
	mov bpb_root_ent_cnt,%ax	// Then account for the sectors
	add $0xf,%ax			// holding the root directory
	mov $4,%cl
	shr %cl,%ax
	add %bx,%ax

	// Calculate number of sectors to load; this may overestimate a bit :-|
	mov (0x1d-0xb)(%si),%dx
	inc %dx
	shr %dx

	// Load at LOADSEG:0
	xor %cx,%cx
	mov $LOADSEG,%bx
	push %bx
	call disk_read

	// w00t!
	call run_prog

kernel_name:
	.ascii "LINUX      "
.endm
