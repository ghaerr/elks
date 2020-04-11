# Helper to run ELKS in QEMU

# Auto-select QEMU system emulator variant
[ -x /usr/bin/qemu-system-i386 ] && QEMU="qemu-system-i386"
[ -x /usr/bin/qemu-system-x86_64 ] && QEMU="qemu-system-x86_64"
[ -x /usr/local/bin/qemu-system-i386 ] && QEMU="qemu-system-i386"
[ -x /usr/local/bin/qemu-system-x86_64 ] && QEMU="qemu-system-x86_64"
[ -z $QEMU ] && { echo 'QEMU system emulator not found!'; exit 1; }
echo "Using QEMU: $QEMU"

# Select disk image to use
# MINIX or FAT .config build
#IMAGE="-fda image/fd2880.bin"
IMAGE="-fda image/fd1440.bin"
#IMAGE="-fda image/fd720.bin"
#IMAGE="-fda image/fd360.bin"
#IMAGE="-hda image/hd.bin"
#IMAGE="-boot order=a -fda image/fd1440.bin -drive file=image/hd32-minix.bin,format=raw,if=ide"

# FAT package manager build
#IMAGE="-fda image/fd360-fat.bin"
#IMAGE="-fda image/fd720-fat.bin"
#IMAGE="-fda image/fd1440-fat.bin"
#IMAGE="-fda image/fd2880-fat.bin"
#IMAGE="-hda image/hd32-fat.bin"
#IMAGE="-hda image/hd32-fat.bin -fda image/fd1440-minix.bin"

# MINIX package manager build
#IMAGE="-fda image/fd360-minix.bin"
#IMAGE="-fda image/fd720-minix.bin"
#IMAGE="-fda image/fd1440-minix.bin"
#IMAGE="-fda image/fd2880-minix.bin"
#IMAGE="-hda image/hd32-minix.bin"

# Second disk for mount after boot
#DISK2="-fdb image/fd360-fat.bin"
#DISK2="-fdb image/fd720-fat.bin"
#DISK2="-fdb image/fd1440-fat.bin"
#DISK2="-fdb image/fd2880-fat.bin"
#DISK2="-fdb image/fd360-minix.bin"
#DISK2="-fdb image/fd720-minix.bin"
#DISK2="-fdb image/fd1440-minix.bin"
#DISK2="-fdb image/fd2880-minix.bin"

[ -z "$IMAGE" ] && { echo 'Disk image not found!'; exit 1; }
echo "Using disk image: $IMAGE"

# Select your keyboard mapping here:
# KEYBOARD="-k en-us"
# KEYBOARD="-k fr"
KEYBOARD=

# Select pty serial port or serial mouse driver
SERIAL="-chardev pty,id=chardev1 -device isa-serial,chardev=chardev1,id=serial1"
#SERIAL="-chardev msmouse,id=chardev1 -device isa-serial,chardev=chardev1,id=serial1"

# Uncomment this to route ELKS /dev/ttyS0 to host terminal
#CONSOLE="-serial stdio"
# Hides qemu window also
#CONSOLE="-serial stdio -nographic"

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

# Determine display type ("Darwin" = OSX)
[ `uname` != 'Darwin' ] && QDISPLAY="-display sdl"

# Configure QEMU as pure ISA system

exec $QEMU $CONSOLE -nodefaults -name ELKS -machine isapc -cpu 486,tsc -m 1M \
$KEYBOARD $QDISPLAY -vga std -rtc base=utc $SERIAL \
-net nic,model=ne2k_isa $HOSTFWD $NETDUMP $IMAGE $DISK2 $@
