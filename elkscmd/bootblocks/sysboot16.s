
! The master boot sector will have setup a stack,
! this is normally at 0:7c00 down.
! DS, SS, CS and ES will all have value 0 so the execution address is 0:7c00
! On entry the register SI will be pointer to the partition entry that
! this sector was loaded from, DL is the drive.

! Also if it's a standard Master boot DH will be the head, CX will be the
! sector and cylinder, BX=7C00, AX=1, DI=7DFE, BP=SI. There's a reasonable
! chance that this isn't true though.

! The Master boot itself will have been loaded and run at $07c00
! The BIOS must have setup a stack because interrupts are enabled
! Little else can be assumed because DOS doesn`t assume anything either

sysboot_start:
j codestart
nop		! DOS appears to _require_ this to identify an MSDOS disk!!

.blkb sysboot_start+3-*
public dosfs_stat
dos_sysid:	.ascii "LINUX"	! System ID
		.byte 0,0,0
dosfs_stat:
dos_sect:	.blkw 1		! Sector size
dos_clust:	.blkb 1		! Cluster size
dos_resv:	.blkw 1		! Res-sector
dos_nfat:	.blkb 1		! FAT count
dos_nroot:	.blkw 1		! Root dir entries
dos_maxsect:	.blkw 1		! Sector count (=0 if large FS)
dos_media:	.blkb 1		! Media code
dos_fatlen:	.blkw 1		! FAT length
dos_spt:	.blkw 1		! Sect/Track
dos_heads:	.blkw 1		! Heads
dos_hidden:	.blkw 2		! Hidden sectors

! Here down is DOS 4+ and probably not needed for floppy boots.

dos4_maxsect:	.blkw 2		! Large FS sector count
dos4_phy_drive:	.blkb 1		! Phys drive
.blkb 1		! Reserved
.blkb 1		! DOS 4

floppy_temp:
dos4_serial:	.blkw 2		! Serial number
dos4_label:	.blkb 11	! Disk Label (DOS 4+)
dos4_fattype:	.blkb 8		! FAT type

!
! This is where the code will be overlaid, the default is a hang
.blkb sysboot_start+0x3E-*
public codestart
codestart:
  j	codestart

! Partition table
public bootblock_magic

.blkb sysboot_start+0x1BE-*
partition_1:
.blkb 8			! IN,SH,SS,ST,OS,EH,ES,ET
.blkw 2			! Linear position (0 based)
.blkw 2			! Linear length
.blkb sysboot_start+0x1CE-*
partition_2:
.blkb 8			! IN,SH,SS,ST,OS,EH,ES,ET
.blkw 2			! Linear position (0 based)
.blkw 2			! Linear length
.blkb sysboot_start+0x1DE-*
partition_3:
.blkb 8			! IN,SH,SS,ST,OS,EH,ES,ET
.blkw 2			! Linear position (0 based)
.blkw 2			! Linear length
.blkb sysboot_start+0x1EE-*
partition_4:
.blkb 8			! IN,SH,SS,ST,OS,EH,ES,ET
.blkw 2			! Linear position (0 based)
.blkw 2			! Linear length

.blkb sysboot_start+0x1FE-*
bootblock_magic:
.word 0xAA55
