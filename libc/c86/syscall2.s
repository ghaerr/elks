; ELKS system call library for C86 - AS86 version
; Must be kept synchronized with elks/arch/i86/kernel/syscall.dat
; PART II
;
; 23 Nov 24 Greg Haerr
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        use16   86

        .text
        .extern callsys

        .global _fork
_fork:
        mov     ax,#2
        jmp     near callsys

        .global _wait4
_wait4:
        mov     ax,#7
        jmp     near callsys

        .global _link
_link:
        mov     ax,#9
        jmp     near callsys

        .global _unlink
_unlink:
        mov     ax,#10
        jmp     near callsys

        .global __execve
__execve:
        mov     ax,#11
        jmp     near callsys

        .global _chdir
_chdir:
        mov     ax,#12
        jmp     near callsys

        .global _mknod
_mknod:
        mov     ax,#14
        jmp     near callsys

        .global _chmod
_chmod:
        mov     ax,#15
        jmp     near callsys

        .global _chown
_chown:
        mov     ax,#16
        jmp     near callsys

        .global __brk
__brk:
        mov     ax,#17
        jmp     near callsys

        .global _stat
_stat:
        mov     ax,#18
        jmp     near callsys

        .global __lseek
__lseek:
        mov     ax,#19
        jmp     near callsys

        .global __getpid
__getpid:
        mov     ax,#20
        jmp     near callsys

        .global _mount
_mount:
        mov     ax,#21
        jmp     near callsys

        .global _umount
_umount:
        mov     ax,#22
        jmp     near callsys

        .global _setuid
_setuid:
        mov     ax,#23
        jmp     near callsys

        .global __getuid
__getuid:
        mov     ax,#24
        jmp     near callsys

        .global _alarm
_alarm:
        mov     ax,#27
        jmp     near callsys

        .global _fstat
_fstat:
        mov     ax,#28
        jmp     near callsys

        .global _access
_access:
        mov     ax,#33
        jmp     near callsys

        .global _sync
_sync:
        mov     ax,#36
        jmp     near callsys

        .global _kill
_kill:
        mov     ax,#37
        jmp     near callsys

        .global _rename
_rename:
        mov     ax,#38
        jmp     near callsys

        .global _mkdir
_mkdir:
        mov     ax,#39
        jmp     near callsys

        .global _rmdir
_rmdir:
        mov     ax,#40
        jmp     near callsys

        .global _dup
_dup:
        mov     ax,#41
        jmp     near callsys

        .global _pipe
_pipe:
        mov     ax,#42
        jmp     near callsys

        .global _dup2
_dup2:
        mov     ax,#45
        jmp     near callsys

        .global _setgid
_setgid:
        mov     ax,#46
        jmp     near callsys

        .global __getgid
__getgid:
        mov     ax,#47
        jmp     near callsys

        .global __signal
__signal:
        mov     ax,#48
        jmp     near callsys

        .global _fcntl
_fcntl:
        mov     ax,#50
        jmp     near callsys

        .global _ioctl
_ioctl:
        mov     ax,#54
        jmp     near callsys

        .global _reboot
_reboot:
        mov     ax,#55
        jmp     near callsys

        .global _lstat
_lstat:
        mov     ax,#57
        jmp     near callsys

        .global _symlink
_symlink:
        mov     ax,#58
        jmp     near callsys

        .global _readlink
_readlink:
        mov     ax,#59
        jmp     near callsys

        .global _umask
_umask:
        mov     ax,#60
        jmp     near callsys

        .global _settimeofday
_settimeofday:
        mov     ax,#61
        jmp     near callsys

        .global _gettimeofday
_gettimeofday:
        mov     ax,#62
        jmp     near callsys

        .global _select
_select:
        mov     ax,#63
        jmp     near callsys

        .global __readdir
__readdir:
        mov     ax,#64
        jmp     near callsys

        .global _fchown
_fchown:
        mov     ax,#66
        jmp     near callsys

        .global _setsid
_setsid:
        mov     ax,#68
        jmp     near callsys

        .global __sbrk
__sbrk:
        mov     ax,#69
        jmp     near callsys

        .global _ustatfs
_ustatfs:
        mov     ax,#70
        jmp     near callsys

        .global _setitimer
_setitimer:
        mov     ax,#71
        jmp     near callsys

        .global _sysctl
_sysctl:
        mov     ax,#72
        jmp     near callsys

        .global _uname
_uname:
        mov     ax,#73
        jmp     near callsys

        .global _socket
_socket:
        mov     ax,#198
        jmp     near callsys

        .global _bind
_bind:
        mov     ax,#200
        jmp     near callsys

        .global _listen
_listen:
        mov     ax,#201
        jmp     near callsys

        .global _accept
_accept:
        mov     ax,#202
        jmp     near callsys

        .global _connect
_connect:
        mov     ax,#203
        jmp     near callsys

        .global _setsockopt
_setsockopt:
        mov     ax,#204
        jmp     near callsys

        .global _getsocknam
_getsocknam:
        mov     ax,#205
        jmp     near callsys

        .global __fmemalloc
__fmemalloc:
        mov     ax,#206
        jmp     near callsys

        .global __fmemfree
__fmemfree:
        mov     ax,#207
        jmp     near callsys
