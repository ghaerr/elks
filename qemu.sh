# Helper to run ELKS in QEMU

# Specify your QEMU system emulator here:
# qemu=qemu-system-i386
qemu=qemu-system-x86_64

$qemu -nodefaults -name ELKS -monitor stdio -machine isapc -cpu 486 -m 1M -k fr -display sdl -vga cirrus -rtc base=utc -net nic,model=ne2k_isa,irq=10 -net user -net dump -fda elkscmd/full3 
