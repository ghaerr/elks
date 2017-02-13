# Helper to run ELKS in QEMU

# Specify your QEMU system emulator here:
# qemu=qemu-system-i386
qemu=qemu-system-x86_64

# Specify your keyboard mapping here:
# keyb=us
keyb=fr

# Configure QEMU as pure ISA system

$qemu -nodefaults -name ELKS -monitor stdio -machine isapc -cpu 486 -m 1M -k $keyb -display sdl -vga cirrus -rtc base=utc serial pty -net nic,model=ne2k_isa -net user -fda elkscmd/full3
