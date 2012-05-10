
CFILES1 = be1/basename.c be1/cat.c be1/chgrp.c be1/chmod.c be1/chown.c be1/clock.c be1/cmp.c be1/cp.c be1/date.c be1/dd.c be1/dirname.c be1/du.c be1/echo.c be1/getty.c be1/grep.c be1/head.c be1/init.c be1/kill.c be1/l.c be1/ln.c be1/login.c be1/ls.c be1/meminfo.c be1/mkdir.c be1/mkfifo.c be1/mknod.c be1/more.c be1/mount.c be1/mv.c be1/passwd.c be1/printenv.c be1/ps.c be1/pwd.c be1/reboot.c be1/rm.c be1/rmdir.c be1/setenv.c be1/sh.c be1/tail.c be1/tar.c be1/tee.c be1/test.c be1/touch.c be1/tr.c be1/umount.c be1/uniq.c be1/which.c be1/who.c be1/whoami.c be1/yes.c

CFILES2 = be2/banner.c be2/cal.c be2/cksum.c be2/compress.c be2/cut.c be2/diff.c be2/ed.c be2/fdisk.c be2/partype.c be2/ramdisk.c be2/sort.c be2/stty.c be2/swapon.c be2/wc.c be2/xargs.c

CFILES3 = be3/find.c be3/logname.c be3/mesg.c be3/mkfs.c be3/sed.c be3/write.c

L1CFILES = lib1/buildname.c lib1/copyfile.c lib1/getpass.c lib1/isadir.c lib1/sashlib.c lib1/timestring.c

LIBCFILES = lib/chunks.c lib/fsbitops.c lib/match.c lib/modestring.c lib/namesort.c lib/stripslash.c lib/usage.c lib/utc_mktime.c lib/wildcards.c

OBJS1 = be1/basename.o be1/cat.o be1/chgrp.o be1/chmod.o be1/chown.o be1/clock.o be1/cmp.o be1/cp.o be1/date.o be1/dd.o be1/dirname.o be1/du.o be1/echo.o be1/getty.o be1/grep.o be1/head.o be1/init.o be1/kill.o be1/l.o be1/ln.o be1/login.o be1/ls.o be1/meminfo.o be1/mkdir.o be1/mkfifo.o be1/mknod.o be1/more.o be1/mount.o be1/mv.o be1/passwd.o be1/printenv.o be1/ps.o be1/pwd.o be1/reboot.o be1/rm.o be1/rmdir.o be1/setenv.o be1/sh.o be1/tail.o be1/tar.o be1/tee.o be1/test.o be1/touch.o be1/tr.o be1/umount.o be1/uniq.o be1/which.o be1/who.o be1/whoami.o be1/yes.o 
OBJS2 = be2/banner.o be2/cal.o be2/cksum.o be2/compress.o be2/cut.o be2/diff.o be2/ed.o be2/fdisk.o be2/partype.o be2/ramdisk.o be2/sort.o be2/stty.o be2/swapon.o be2/wc.o be2/xargs.o 
OBJS3 = be3/find.o be3/logname.o be3/mesg.o be3/mkfs.o be3/sed.o be3/write.o 
L1OBJS = lib1/buildname.o lib1/copyfile.o lib1/getpass.o lib1/isadir.o lib1/sashlib.o lib1/timestring.o 
LIBOBJS = lib/chunks.o lib/fsbitops.o lib/match.o lib/modestring.o lib/namesort.o lib/stripslash.o lib/usage.o lib/utc_mktime.o lib/wildcards.o 
CC = bcc
CFLAGS = -ansi -O -0

all: busyelks1 busyelks2 busyelks3

busyelks1: $(OBJS1) $(LIBOBJS) $(L1OBJS) busyelks1.o
	$(CC) $(CFLAGS) -s -o busyelks1 $(OBJS1) $(LIBOBJS) $(L1OBJS) busyelks1.o

busyelks2: $(OBJS2) $(LIBOBJS) busyelks2.o
	$(CC) $(CFLAGS) -s -o busyelks2 $(OBJS2) $(LIBOBJS) busyelks2.o

busyelks3: $(OBJS3) $(LIBOBJS) busyelks3.o
	$(CC) $(CFLAGS) -s -o busyelks3 $(OBJS3) $(LIBOBJS) busyelks3.o

busyelks: busyelks1 busyelks2 busyelks3

$(OBJS1): $(CFILES1) busyelks1.c

$(OBJS2): $(CFILES2) busyelks2.c

$(OBJS3): $(CFILES3) busyelks3.c

$(L1OBJS): $(L1CFILES)

$(LIBOBJS): $(LIBCFILES)

clean:
	rm -rf a.out core *~ *.bak *.o */*.o busyelks1 busyelks2 busyelks3 _install

stats:
	objdump86 */*.o | grep -A 1 text | grep '\/.*\.o' | sort -g -k 1
	objdump86 busyelks? | grep -v '\:' | grep -A 1 text
