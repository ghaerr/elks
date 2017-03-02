# Helper to run ELKS in QEMU

# Specify your QEMU system emulator here:
qemu=qemu-system-i386
#qemu=qemu-system-x86_64

# Specify your keyboard mapping here:
# keyb="-k en-us"
# keyb="-k fr"
keyb=

# Specify network dump option here:
#netdump="-net dump"
netdump=

# Specify host forwarding here:
# no forwarding - only outgoing from ELKS to host
hostfwd="-net user"
# incoming telnet forwarding - example: connect to ELKS with telnet localhost:2323
#hostfwd="-net user,hostfwd=tcp:127.0.0.1:2323-10.0.2.15:23"
# telnet and http forwarding - example: connect to ELKS httpd with 'http://localhost:8080'
#hostfwd="-net user,hostfwd=tcp:127.0.0.1:2323-10.0.2.15:23,hostfwd=tcp:127.0.0.1:8080-10.0.2.15:80"

serialdev="-chardev pty,id=chardev1 -device isa-serial,chardev=chardev1,id=serial1"

# Configure QEMU as pure ISA system

$qemu -nodefaults -name ELKS -monitor stdio -machine isapc -cpu 486 \
-m 1M $keyb -display sdl -vga cirrus -rtc base=utc $serialdev \
-net nic,model=ne2k_isa $hostfwd $netdump -fda elkscmd/full3
