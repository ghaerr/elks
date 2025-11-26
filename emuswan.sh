#!/usr/bin/env bash

# Run ELKS in wf-mednafen (WonderSwan emulator)
# wf-mednafen can be built as part of the cross tools:
# make -C tools swan
# See https://github.com/WonderfulToolchain/wf-mednafen

# For ELKS ROM Configuration:
# ELKS must be configured minimally with 'cp swan.config .config'

# Environment setup

set -e

SCRIPTDIR="$(dirname "$0")"

. "$SCRIPTDIR/env.sh"

# Create temporary directory housing the fake serial port and wrapper script
temp=$(mktemp -d /tmp/emuswan.XXXXXXXX)

# Create wrapper script
cat <<EOF >$temp/serial.sh
#!/usr/bin/env bash
socat PTY,link=$temp/ttyS0,echo=0 STDIO
EOF
chmod +x $temp/serial.sh

# Print banner
echo ====================================================================
echo
echo Use the following file to access the emulated console\'s serial port:
echo
echo $temp/ttyS0
echo
echo ====================================================================
echo

# Run emulator
export MEDNAFEN_HOME="$CROSSDIR/etc/mednafen"
mkdir -p "$MEDNAFEN_HOME"
exec wf-mednafen -wswan.excomm 1 -wswan.excomm.path $temp/serial.sh image/rom.wsc ${1+"$@"}

# Clean up
rm -r $temp
