# Helper to run ELKS in QEMU

# QEMU emulator binary to use
QEMU=qemu-system-i386
#QEMU=qemu-system-x86_64

# Floppy image to use
IMAGE=elkscmd/full3

# Keyboard mapping
# KEYBOARD="-k en-us"
# KEYBOARD="-k fr"
KEYBOARD=

# Network dump
#NETDUMP="-net dump"
NETDUMP=

# Host forwarding for networking
# no forwarding - only outgoing from ELKS to host
HOSTFWD="-net user"
# incoming telnet forwarding - example: connect to ELKS with telnet localhost:2323
#HOSTFWD="-net user,hostfwd=tcp:127.0.0.1:2323-10.0.2.15:23"
# telnet and http forwarding - example: connect to ELKS httpd with 'http://localhost:8080'
#HOSTFWD="-net user,hostfwd=tcp:127.0.0.1:2323-10.0.2.15:23,hostfwd=tcp:127.0.0.1:8080-10.0.2.15:80"

SERIALDEV="-chardev pty,id=chardev1 -device isa-serial,chardev=chardev1,id=serial1"

if [ ! -e "$IMAGE" ]
	then echo "The disk image file '$IMAGE' is missing"
	exit 1
fi

# Configure QEMU as pure ISA system
$QEMU -nodefaults -name ELKS -monitor stdio -machine isapc -cpu 486 \
-m 1M $KEYBOARD -display sdl -vga cirrus -rtc base=utc $SERIALDEV \
-net nic,model=ne2k_isa $HOSTFWD $NETDUMP -fda $IMAGE $@
