# Helper to run ELKS in QEMU

# Specify your QEMU system emulator here:
# qemu=qemu-system-i386
# qemu=qemu-system-x86_64
qemu=qemu-system-i386

# Specify your keyboard mapping here:
# keyb="-k en-us"
# keyb="-k fr"
keyb=

# Specify network debugging here:
# netdebug="-net dump"
netdebug=

# Configure QEMU as pure ISA system

$qemu -nodefaults -name ELKS -monitor stdio -machine isapc -cpu 486 -m 1M \
$keyb -display sdl -vga cirrus -rtc base=utc -serial pty \
-net nic,model=ne2k_isa -net user $netdebug -fda elkscmd/full3
