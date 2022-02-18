//------------------------------------------------------------------------------
// Code and data structures for booting ELKS from a FAT12/16 filesystem
//------------------------------------------------------------------------------

// boot_sect.S will use these macros if configured to be a FAT12/16 bootloader

.macro FAT_BPB
#if defined(CONFIG_IMG_FD360)
	.set FAT_SEC_PER_CLUS, 1
	.set FAT_ROOT_ENT_CNT, 224
	.set FAT_TOT_SEC, 720
	.set FAT_MEDIA_BYTE, 0xfd
	.set FAT_TABLE_SIZE, 3
	.set FAT_SEC_PER_TRK, 9
	.set FAT_NUM_HEADS, 2
#elif defined(CONFIG_IMG_FD720)
	.set FAT_SEC_PER_CLUS, 1
	.set FAT_ROOT_ENT_CNT, 224
	.set FAT_TOT_SEC, 1440
	.set FAT_MEDIA_BYTE, 0xf9
	.set FAT_TABLE_SIZE, 9
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
#elif defined(CONFIG_IMG_FD1232)
	.set FAT_SEC_PER_CLUS, 1
	.set FAT_ROOT_ENT_CNT, 192
	.set FAT_TOT_SEC, 1232
	.set FAT_MEDIA_BYTE, 0xfe
	.set FAT_TABLE_SIZE, 2
	.set FAT_SEC_PER_TRK, 8
	.set FAT_NUM_HEADS, 2
#elif defined(CONFIG_IMG_FD1440)
	.set FAT_SEC_PER_CLUS, 1
	.set FAT_ROOT_ENT_CNT, 224
	.set FAT_TOT_SEC, 2880
	.set FAT_MEDIA_BYTE, 0xf0
	.set FAT_TABLE_SIZE, 9
	.set FAT_SEC_PER_TRK, 18
	.set FAT_NUM_HEADS, 2
#elif defined(CONFIG_IMG_HD)
	.set FAT_ROOT_ENT_CNT, 512
	.set FAT_TOT_SEC, 2 * CONFIG_IMG_BLOCKS
// Default cluster sizes for various partition sizes --- see Microsoft's 2000
// white paper (reference below), p. 20
.if FAT_TOT_SEC < 262144
	.set FAT_SEC_PER_CLUS, 2
.elseif FAT_TOT_SEC < 524288
	.set FAT_SEC_PER_CLUS, 4
.elseif FAT_TOT_SEC < 1048576
	.set FAT_SEC_PER_CLUS, 8
.elseif FAT_TOT_SEC < 2097152
	.set FAT_SEC_PER_CLUS, 16
.elseif FAT_TOT_SEC < 4194304
	.set FAT_SEC_PER_CLUS, 32
.else
	.set FAT_SEC_PER_CLUS, 64
.endif
	.set FAT_MEDIA_BYTE, 0xf8
// Default file allocation table size --- see 2000 white paper (again), p. 21
	.set FAT_TMP_VAL_1, \
	     FAT_TOT_SEC - 1 - (FAT_ROOT_ENT_CNT * 32 + 511) / 512
	.set FAT_TMP_VAL_2, 256 * FAT_SEC_PER_CLUS + 2
	.set FAT_TABLE_SIZE, \
	     (FAT_TMP_VAL_1 + FAT_TMP_VAL_2 - 1) / FAT_TMP_VAL_2
	.set FAT_SEC_PER_TRK, CONFIG_IMG_SECT
	.set FAT_NUM_HEADS, CONFIG_IMG_HEAD
#else
	.warning "Unknown disk medium!"
	.set FAT_SEC_PER_CLUS, 0
	.set FAT_ROOT_ENT_CNT, 0
	.set FAT_TOT_SEC, 0
	.set FAT_MEDIA_BYTE, 0
	.set FAT_TABLE_SIZE, 0
	.set FAT_SEC_PER_TRK, 0
	.set FAT_NUM_HEADS, 0
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
	.ascii "ELKSFAT1"
bpb_byts_per_sec:			// Bytes per sector
	.word SECTOR_SIZE
bpb_sec_per_clus:			// Sectors per cluster
	.byte FAT_SEC_PER_CLUS
bpb_rsvd_sec_cnt:			// Reserved sectors, including this one
	.word 1
bpb_num_fats:				// Number of duplicate file allocation
	.byte 2				// tables
bpb_root_ent_cnt:			// Number of root directory entries
	.word FAT_ROOT_ENT_CNT
bpb_tot_sec_16:				// Total number of sectors, 16-bit
.if FAT_TOT_SEC <= 0xffff
	.word FAT_TOT_SEC
.else
	.word 0
.endif
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
	.global sect_offset
sect_offset:
	.long 0
bpb_tot_sec_32:				// Total number of sectors, 32-bit
.if FAT_TOT_SEC > 0xffff
	.long FAT_TOT_SEC
.else
	.long 0
.endif
bpb_drv_num:				// INT 13 drive number
	.byte 0
bpb_reserved1:				// Reserved
	.byte 0
bpb_bootsig:				// Extended boot signature
	.byte 0x29
bpb_vol_id:				// Volume serial number
	.long 0
bpb_vol_label:				// Volume label (11 bytes)
	.ascii "NO NAME    "
bpb_fil_sys_type:			// Filesystem type (8 bytes)
	.ascii "FAT12   "
.endm
	.set bpb_fat_sz_32,0x24		// FAT32 fat size, 32-bit

.macro FAT_LOAD_AND_RUN_KERNEL
	.set buf, entry + SECTOR_SIZE

	// Load the first sector of the root directory
	movb bpb_num_fats,%al
	xor %ah,%ah
	mov bpb_fat_sz_16,%bx		// check FAT16 fat size
	and %bx,%bx
	jnz 1f				// nonzero means FAT16 filesystem
	mov bpb_fat_sz_32,%bx		// get loword FAT32 fat size instead
1:	mulw %bx
	addw bpb_rsvd_sec_cnt,%ax
	push %ax
	inc %dx				// Assume DX was 0 (from mulw); if
					// DX != 0 then we are in trouble
					// anyway
	mov $buf,%cx
	push %ss
	call disk_read

	// See if /linux is in the first few root directory entries; bail out
	// if not
	// ROOT_ENTS_TO_CHECK says how many directory entries to check; it
	// should be in the range 1 to 16, & must also factor in entry slots
	// used for volume labels & long file names
	.set ROOT_ENTS_TO_CHECK, 8
	mov $buf-0x20,%si
find_system:
	addw $0x20,%si
	cmp $buf+ROOT_ENTS_TO_CHECK*0x20, %si
	jz no_system
	mov $kernel_name,%di
	mov $0xb,%cx
	push %si
	repz
	cmpsb
	lodsb
	pop %si
	jnz find_system
	test $0b11011000,%al		// check that any /linux entry we find
	jnz find_system			// is a normal file (not e.g. a volume
					// label or LFN portion)

	// Load consecutive sectors starting from the first file cluster
	// Calculate starting sector
	mov 0x1a(%si),%ax		// First cluster number
	dec %ax				// Compute no. of sectors from the
	dec %ax				// disk data area start
	mov bpb_sec_per_clus,%cl	// CH = 0
	mul %cx
	pop %bx				// BX = root directory logical sector
					// we calculated before
	add %ax,%bx
	mov bpb_root_ent_cnt,%ax	// Then account for the sectors
#if defined(CONFIG_IMG_FD1232)
	add $0x1f,%ax
	mov $5,%cl
#else
	add $0xf,%ax			// holding the root directory
	mov $4,%cl
#endif
	shr %cl,%ax
	add %bx,%ax

	// Load the file as one single blob at LOADSEG:0
	mov 0x1d(%si),%dx		// File size divided by 0x100
#if defined(CONFIG_IMG_FD1232)
	shr %dx
#endif
	shr %dx				// Now by 0x200 --- a sector count
	inc %dx				// Account for any incomplete sector
					// (this may overestimate a bit)
	// (According to MFLD (https://github.com/jbruchon/elks/issues/288
	// #issuecomment-581034966), BIOS reads may reportedly fail if %es
	// for read calls is not aligned to a large power of 2; to be on the
	// safe side, rewrite 0x100:0 into something like 0:0x1000...)
	mov $LOADSEG,%cx
	mov %cx,%es
.if (LOADSEG & 0xff) == 0
	mov $LOADSEG>>4,%ch
.else
	mov $LOADSEG<<4,%cx
.endif
.if (LOADSEG & 0xf000) == 0
	xor %bx,%bx
.else
	mov $LOADSEG&0xf000,%bx
.endif
	push %bx
	call disk_read
	mov sect_offset,%cx
	mov sect_offset+2,%si

	// Check for ELKS magic number
	mov $elks_magic,%di
	mov $0x4C45,%ax
	scasw
	jz boot_it

not_elks:
	mov $ERR_BAD_SYSTEM,%al
	jmp _except

boot_it:
	// w00t!
	mov drive_num,%al
	xor %ah,%ah
	RESTORE_DDPT
	push %es
	pop %ds
	// Signify that /linux was loaded as 1 blob
	orb $(EF_AS_BLOB|EF_BIOS_DEV_NUM),elks_flags
	mov %ax,root_dev
	mov %cx,part_offset  // save sector offset of booted partition
	mov %si,part_offset+2
	ljmp $LOADSEG+0x20,$0

kernel_name:
	.ascii "LINUX      "
.endm
