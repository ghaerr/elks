#!/bin/bash
# Test DHCP client against macOS built-in DHCP server (bootpd)
# via QEMU vmnet-shared. Requires sudo for vmnet framework.
# Usage: ./test-dhcp.sh
# After auto-login + net start, drops into interactive shell.

set -e
cd "$(dirname "$0")"

# Rebuild image with latest ktcp
. ./env.sh
make -C elkscmd/ktcp
make image

echo "=== Booting ELKS with vmnet-shared (macOS bootpd DHCP) ==="
echo "Press Ctrl-A X to exit QEMU"
echo ""

# Cache sudo credentials before piping stdin to QEMU
sudo -v

# Pipe through cat to forward terminal input to QEMU serial
cat | sudo qemu-system-i386 \
    -accel tcg,one-insn-per-tb=on \
    -nodefaults \
    -name ELKS \
    -machine isapc \
    -cpu 486,tsc \
    -m 8M \
    -vga std \
    -rtc base=utc \
    -netdev vmnet-shared,id=mynet \
    -device ne2k_isa,irq=12,netdev=mynet \
    -serial stdio \
    -nographic \
    -fda image/fd1440.img
