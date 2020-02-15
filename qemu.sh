# Helper to run ELKS in QEMU

# Auto-select QEMU system emulator variant
[ -x /usr/bin/qemu-system-i386 ] && QEMU="qemu-system-i386"
[ -x /usr/bin/qemu-system-x86_64 ] && QEMU="qemu-system-x86_64"
[ -x /usr/local/bin/qemu-system-i386 ] && QEMU="qemu-system-i386"
[ -x /usr/local/bin/qemu-system-x86_64 ] && QEMU="qemu-system-x86_64"
[ -z $QEMU ] && { echo 'QEMU system emulator not found!'; exit 1; }

# Select floppy disk image set(s) to use

# MINIX or FAT .config build
IMAGE=image/fd1440.bin

# FAT package manager build
#IMAGE=image/fd360-fat.bin
#IMAGE=image/fd720-fat.bin
#IMAGE=image/fd1440-fat.bin
#IMAGE=image/fd2880-fat.bin

# MINIX package manager build
#IMAGE=image/fd360-minix.bin
#IMAGE=image/fd720-minix.bin
#IMAGE=image/fd1440-minix.bin

# Second disk for mount after boot
#DISK2="-fdb image/fd360-fat.bin"
#DISK2="-fdb image/fd720-fat.bin"
#DISK2="-fdb image/fd1440-fat.bin"
#DISK2="-fdb image/fd2880-fat.bin"
#DISK2="-fdb image/fd1440-minix.bin"

# Select your keyboard mapping here:
# KEYBOARD="-k en-us"
# KEYBOARD="-k fr"
KEYBOARD=

# Host forwarding for networking
# No forwarding: only outgoing from ELKS to host
# HOSTFWD="-net user"
# Incoming telnet forwarding: example: connect to ELKS with telnet localhost 2323
# HOSTFWD="-net user,hostfwd=tcp:127.0.0.1:2323-10.0.2.15:23"
# Incoming http forwarding: example: connect to ELKS httpd with 'http://localhost:8080'
# HOSTFWD="-net user,hostfwd=tcp:127.0.0.1:8080-10.0.2.15:80"
HOSTFWD="-net user"

# Enable network dump here:
# NETDUMP="-net dump"

# Determine display type
[ `uname` != 'Darwin' ] && QDISPLAY="-display sdl"

# Configure QEMU as pure ISA system

SERIAL="-chardev pty,id=chardev1 -device isa-serial,chardev=chardev1,id=serial1"

$QEMU -nodefaults -name ELKS -machine isapc -cpu 486,tsc -m 1M \
$KEYBOARD $QDISPLAY -vga std -rtc base=utc $SERIAL \
-net nic,model=ne2k_isa $HOSTFWD $NETDUMP -fda $IMAGE $DISK2 $@
