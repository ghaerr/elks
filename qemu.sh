# Helper to run ELKS in QEMU

# Specify your QEMU system emulator here:
qemu=qemu-system-i386
#qemu=qemu-system-x86_64

# Specify your keyboard mapping here:
# keyb=-k en-us
# keyb=-k fr
keyb=
# Specify network debugging here:
netdebug="-net dump"
#netdebug=
# Specify host forwarding here:
# no forwarding, just ELKS to host
hostfwd="-net user"
# telnet port forwarding
#hostfwd="-net user,hostfwd=tcp:127.0.0.1:2323-10.0.2.15:2323"
# telnet and http forwarding, call ELKS web server with 'http://localhost:8080'
#hostfwd="-net user,hostfwd=tcp:127.0.0.1:2323-10.0.2.15:23,hostfwd=tcp::8080-:80"
serialdev="-chardev pty,id=chardev1 -device isa-serial,chardev=chardev1,id=serial1"

# Configure QEMU as pure ISA system

$qemu -nodefaults -name ELKS -monitor stdio -machine isapc -cpu 486 \
-m 1M $keyb -display sdl -vga cirrus -rtc base=utc $serialdev \
-net nic,model=ne2k_isa $hostfwd $netdebug -fda elkscmd/full3
