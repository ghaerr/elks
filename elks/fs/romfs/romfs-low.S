; ROMFS - A tiny filesystem in ROM

; This module implements low-level functions
; to directly access the filesystem in ROM,
; and used by the ROMFS high-level module.

#define __ASSEMBLY__
#include <linuxmt/config.h>

	.text

; Get superblock
; int romfs_super_get (byte_t *);
; TODO: add argument with size of destination buffer

_romfs_super_get:

	push  bp
	mov   bp,sp
	push  ds
	push  di
	push  si
	push  cx

	mov   ax,#CONFIG_ROMFS_BASE
	mov   ds,ax

	; TODO: check super block magic

	xor   si,si
	mov   cx,[si+6]  ; size of superblock
	mov   di,[bp+4]  ; arg0

	cld
	rep
	movsb

	xor   ax,ax

	pop   cx
	pop   si
	pop   di
	pop   ds
	pop   bp
	ret


; Get inode from index
; int romfs_inode_get (word_t index, byte_t *);
; TODO: add argument with size of destination buffer

_romfs_inode_get:

	push  bp
	mov   bp,sp
	push  ds
	push  di
	push  si
	push  dx
	push  cx

	mov   ax,#CONFIG_ROMFS_BASE
	mov   ds,ax

	; Check inode index

	xor   si,si

	mov   ax,[bp+4]   ; arg0
	cmp   ax,[si+10]  ; inodes count
	jb    inode_get_next1

	mov   ax,#1
	jmp   inode_get_exit

inode_get_next1:

	; Compute inode base from index

	xor   dx,dx
	mov   cx,[si+8]  ; inode size
	mul   cx
	add   ax,[si+6]  ; superblock size
	adc   dx,#0
	or    dx,dx
	jz    inode_get_next2

	mov   ax,#2
	jmp   inode_get_exit

inode_get_next2:

	mov   si,ax
	mov   di,[bp+6]  ; arg1

	cld
	rep
	movsb

	xor   ax, ax

inode_get_exit:

	pop   cx
	pop   dx
	pop   si
	pop   di
	pop   ds
	pop   bp
	ret


; Get data from block offset and position
; int romfs_file_read (word_t offset, word_t pos, word_t seg, byte_t * buf, word_t len);

_romfs_file_read:

	push  bp
	mov   bp,sp
	push  es
	push  ds
	push  di
	push  si
	push  cx

	mov   ax,[bp+4]   ; arg0: offset of the block in FS (in paragraphs)
	add   ax,#CONFIG_ROMFS_BASE
	mov   ds,ax

	mov   ax,[bp+8]   ; arg2: destination segment (or 0 for current ES)
	test  ax,ax
	jz    file_read_next1
	mov   es,ax

file_read_next1:

	mov   si,[bp+6]   ; arg1: position in the block (in bytes)
	mov   di,[bp+10]  ; arg3: destination offset
	mov   cx,[bp+12]  ; arg4: bytes count

	cld
	rep
	movsb

	xor   ax,ax  ; no error

	pop   cx
	pop   si
	pop   di
	pop   ds
	pop   es
	pop   bp
	ret


; Lookup inode name in directory block
; int romfs_dir_lookup (word_t offset, word_t seg, char * name, byte_t len, word_t * index);

_romfs_dir_lookup:

	push  bp
	mov   bp,sp
	push  ds
	push  di
	push  si
	push  cx

	mov   ax,[bp+4]   ; arg0: offset of the block in FS (in paragraphs)
	add   ax,#CONFIG_ROMFS_BASE
	mov   ds,ax

	xor   si,si
	lodsw             ; directory entry count
	or    ax,ax
	jnz   dir_lookup_next1
	mov   ax,#1
	jmp   dir_lookup_exit

dir_lookup_next1:

	mov   cx,ax

dir_lookup_loop1:

	lodsb             ; entry size
	xor   ah,ah
	cmp   ax,[bp+10]   ; arg3: name length
	jz    dir_lookup_next2
	add   si,ax
	jmp   dir_lookup_next3

dir_lookup_next2:

	push  cx
	mov   cx,ax

	push  cx
	push  si
	push  es
	mov   es,[bp+6]   ; arg1: name segment
	mov   di,[bp+8]   ; arg2: name offset

	cld
	repz
	cmpsb
	jz    dir_lookup_next4

	pop   es
	pop   si
	pop   cx
	add   si,cx
	pop   cx

dir_lookup_next3:

	add   si,#2
	loop  dir_lookup_loop1

	mov   ax,#2
	jmp   dir_lookup_exit

dir_lookup_next4:

	lodsw             ; inode index
	pop   es
	pop   si
	pop   cx
	pop   cx
	mov   di,[bp+12]  ; arg4: pointer to index
	seg   es
	mov   [di],ax

	xor   ax,ax

dir_lookup_exit:

	pop   cx
	pop   si
	pop   di
	pop   ds
	pop   bp
	ret


	export _romfs_super_get
	export _romfs_inode_get
	export _romfs_file_read
	export _romfs_dir_lookup

	end
