#!/usr/bin/env bash
# ELKS with tap networking (Linux)
# Full IP/ICMP forwarding — traceroute shows real hops
set -e
cd "$(dirname "$0")"

IMAGE="../../image/fd1440.img"
[ -f "$IMAGE" ] || IMAGE="../../image/fd2880.img"
[ -f "$IMAGE" ] || { echo "No ELKS image found!"; exit 1; }

TAP="${1:-tap0}"
NET="${2:-192.168.100}"
GATE="${NET}.1"
IP="${NET}.100"

echo "======================================================"
echo "  ELKS with Linux tap networking"
echo ""
echo "  Creates $TAP on $NET.0/24"
echo "  ELKS IP: $IP  Gateway: $GATE"
echo ""
echo "  Full IP/ICMP forwarding — real traceroute hops"
echo "  NOTE: needs sudo"
echo "======================================================"
echo ""

# Clean up any stale interface
sudo ip link delete "$TAP" 2>/dev/null || true

# Create tap and bridge
sudo ip tuntap add "$TAP" mode tap
sudo ip addr add "$GATE/24" dev "$TAP"
sudo ip link set "$TAP" up

# NAT to internet
sudo iptables -t nat -C POSTROUTING -s "$NET.0/24" -j MASQUERADE 2>/dev/null || \
    sudo iptables -t nat -A POSTROUTING -s "$NET.0/24" -j MASQUERADE
sudo sh -c "echo 1 > /proc/sys/net/ipv4/ip_forward"

cleanup() {
    sudo iptables -t nat -D POSTROUTING -s "$NET.0/24" -j MASQUERADE 2>/dev/null || true
    sudo ip link delete "$TAP" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

echo "Inside ELKS: ktcp -b $IP $GATE 255.255.255.0"
echo "     Or after ktcp starts: ifconfig $IP netmask 255.255.255.0 gateway $GATE"
echo ""
echo "NOTE: if you run 'net stop' later, re-run 'ifconfig' after 'net start'"
echo "      (net start resets ktcp to /etc/net.cfg IPs)"
echo ""

exec sudo qemu-system-x86_64 -nodefaults -machine pc -cpu 486,tsc -m 8M \
    -serial stdio \
    -vga std \
    -netdev tap,id=net0,ifname="$TAP",script=no,downscript=no \
    -device ne2k_isa,irq=12,netdev=net0 \
    -snapshot -fda "$IMAGE"
