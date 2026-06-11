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
echo "Auto-login as root, runs net start, then interactive."
echo "Press Ctrl-A X to exit QEMU"

# Auto-login, run net start, then hand off to interactive stdin
( sleep 15; echo "root"; sleep 3; echo "net start"; sleep 5; cat ) | \
sudo qemu-system-i386 \
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
