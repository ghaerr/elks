# Helper to run ELKS in QEMU

# Auto-select QEMU system emulator variant
[ -x /usr/bin/qemu-system-i386 ] && QEMU="qemu-system-i386"
[ -x /usr/bin/qemu-system-x86_64 ] && QEMU="qemu-system-x86_64"
[ -x /usr/local/bin/qemu-system-i386 ] && QEMU="qemu-system-i386"
[ -x /usr/local/bin/qemu-system-x86_64 ] && QEMU="qemu-system-x86_64"
[ -x /opt/homebrew/bin/qemu-system-i386 ] && QEMU="qemu-system-i386"
[ -x /opt/homebrew/bin/qemu-system-x86_64 ] && QEMU="qemu-system-x86_64"
[ -z $QEMU ] && { echo 'QEMU system emulator not found!'; exit 1; }
echo "Using QEMU: $QEMU"

# Select disk image to use
# MINIX or FAT .config build
#IMAGE="-fda image/fd2880.img"
IMAGE="-fda image/fd1440.img"
#IMAGE="-fda image/fd1200.img"
#IMAGE="-fda image/fd720.img"
#IMAGE="-fda image/fd360.img"
#IMAGE="-hda image/hd.img"
#IMAGE="-boot order=a -fda image/fd1440.img -drive file=image/hd32-minix.img,format=raw,if=ide"

# FAT package manager build
#IMAGE="-fda image/fd360-fat.img"
#IMAGE="-fda image/fd720-fat.img"
#IMAGE="-fda image/fd1200-fat.img"
#IMAGE="-fda image/fd1440-fat.img"
#IMAGE="-fda image/fd2880-fat.img"
#IMAGE="-hda image/hd32-fat.img"
#IMAGE="-hda image/hd32-fat.img -fda image/fd1440-minix.img"

# MINIX package manager build
#IMAGE="-fda image/fd360-minix.img"
#IMAGE="-fda image/fd720-minix.img"
#IMAGE="-fda image/fd1200-minix.img"
#IMAGE="-fda image/fd1440-minix.img"
#IMAGE="-fda image/fd2880-minix.img"
#IMAGE="-hda image/hd32-minix.img"

# MBR builds
#IMAGE="-hda image/hd32-minix.img"
#IMAGE="-hda image/hd32mbr-minix.img"
#IMAGE="-hda image/hd32mbr-minix.img -hdb image/hd32-minix.img"
#IMAGE="-hda image/hd32mbr-fat.img"
#IMAGE="-fda image/fd1440.img -drive file=image/hd32mbr-minix.img,if=ide"
#IMAGE="-boot order=a -fda image/fd1440.img -drive file=image/hd32-minix.img,if=ide"
#IMAGE="-boot order=a -fda image/fd1440.img -drive file=image/hd32mbr-minix.img,if=ide"
#IMAGE="-boot order=a -fda image/fd1440.img -drive file=test.img,if=ide"

# Second disk for mount after boot
#DISK2="-fdb image/fd360-fat.img"
#DISK2="-fdb image/fd720-fat.img"
#DISK2="-fdb image/fd1200-fat.img"
#DISK2="-fdb image/fd1440-fat.img"
#DISK2="-fdb image/fd2880-fat.img"
#DISK2="-fdb image/fd360-minix.img"
#DISK2="-fdb image/fd720-minix.img"
#DISK2="-fdb image/fd1200-minix.img"
#DISK2="-fdb image/fd1440-minix.img"
#DISK2="-fdb image/fd2880-minix.img"
#DISK2="-hdb image/hd32-minix.img"
#DISK2="-hdb image/hd32-fat.img"

[ -z "$IMAGE" ] && { echo 'Disk image not found!'; exit 1; }
echo "Using disk image: $IMAGE"

# Select your keyboard mapping here:
# KEYBOARD="-k en-us"
# KEYBOARD="-k fr"
KEYBOARD=

# Select pty serial port or serial mouse driver
#SERIAL="-chardev pty,id=chardev1 -device isa-serial,chardev=chardev1,id=serial1"
#SERIAL="-chardev msmouse,id=chardev1 -device isa-serial,chardev=chardev1,id=serial1"

# Uncomment this to route ELKS /dev/ttyS0 to host terminal
CONSOLE="-serial stdio"
# Hides qemu window also
#CONSOLE="-serial stdio -nographic"

# Host forwarding for networking
# No forwarding: only outgoing from ELKS to host
# HOSTFWD="-net user"
# Incoming telnet forwarding: example: connect to ELKS with telnet localhost 2323
# HOSTFWD="-net user,hostfwd=tcp:127.0.0.1:2323-10.0.2.15:23"
# Incoming http forwarding: example: connect to ELKS httpd with 'http://localhost:8080'
# HOSTFWD="-net user,hostfwd=tcp:127.0.0.1:8080-10.0.2.15:80"

# Simultaneous telnet, http and ftp forwarding
FWD="\
hostfwd=tcp:127.0.0.1:8080-10.0.2.15:80,\
hostfwd=tcp:127.0.0.1:2323-10.0.2.15:23,\
hostfwd=tcp::8020-:20,\
hostfwd=tcp::8021-:21,\
hostfwd=tcp::8041-:49821,\
hostfwd=tcp::8042-:49822,\
hostfwd=tcp::8043-:49823,\
hostfwd=tcp::8044-:49824,\
hostfwd=tcp::8045-:49825,\
hostfwd=tcp::8046-:49826,\
hostfwd=tcp::8047-:49827,\
hostfwd=tcp::8048-:49828,\
hostfwd=tcp::8049-:49829"

# new style
#NET="-net nic,model=ne2k_isa -net user,$FWD"
# old style, with configurable interrupt line
NET="-netdev user,id=mynet,$FWD -device ne2k_isa,irq=12,netdev=mynet"

# Enable network dump here:
# NETDUMP="-net dump"

# Enable PC-Speaker here:
AUDIO="-audiodev pa,id=speaker -machine pcspk-audiodev=speaker"

# Determine display type ("Darwin" = OSX)
[ `uname` != 'Darwin' ] && QDISPLAY="-display sdl"

# Configure QEMU as pure ISA system

exec $QEMU $AUDIO $CONSOLE -nodefaults -name ELKS -machine isapc -cpu 486,tsc -m 4M \
$KEYBOARD $QDISPLAY -vga std -rtc base=utc $SERIAL \
$NET $NETDUMP $IMAGE $DISK2 $@
