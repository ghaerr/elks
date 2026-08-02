#!/usr/bin/env bash
# Send audio from a PulseAudio sink to audiorecv -4 on the ELKS box.
#
# Same idea as pa2elks.sh, but the stream is packed as Creative 4-bit ADPCM
# before it goes on the wire: 16000 Hz then costs 8000 bytes a second, which
# the link carries, where the same rate as plain PCM would need 16000 and
# would not fit.  audiorecv expands it on the target - the card's own ADPCM
# playback commands decode to something other than Creative's encoding on an
# OPTi 82C929, so the expansion is done in software there.
#
# usage: ./pa2elks-adpcm.sh [host] [port]
set -u
HOST=${1:-${HOST:-192.168.10.201}}
PORT=${2:-${PORT:-4950}}
RATE=${RATE:-16000}
SINK=${SINK:-elksc}
ENC=${ENC:-$(dirname "$0")/sb4enc.py}

for t in pactl parec ffmpeg socat python3; do
    command -v "$t" >/dev/null 2>&1 || { echo "$0: need $t" >&2; exit 1; }
done
[ -r "$ENC" ] || { echo "$0: encoder not found at $ENC" >&2; exit 1; }

OURMOD=
if ! pactl list short sinks 2>/dev/null | grep -qE "[[:space:]]${SINK}[[:space:]]"; then
    WAS=$(pactl info | sed -n 's/^Default Sink: //p')
    OURMOD=$(pactl load-module module-null-sink \
        sink_name="$SINK" \
        sink_properties=device.description=ELKS_PC1640_ADPCM) || {
        echo "$0: could not create sink '$SINK'" >&2; exit 1; }
    echo "$0: created sink '$SINK' (module $OURMOD)"
    [ -n "$WAS" ] && [ "$WAS" != "$SINK" ] && pactl set-default-sink "$WAS" 2>/dev/null
fi
NOWDEF=$(pactl info | sed -n 's/^Default Sink: //p')
if [ "$NOWDEF" = "$SINK" ]; then
    REAL=$(pactl list short sinks | awk '$2 != "'"$SINK"'" { print $2; exit }')
    [ -n "$REAL" ] && pactl set-default-sink "$REAL" 2>/dev/null
fi
cleanup() {
    trap - EXIT INT TERM
    [ -n "$OURMOD" ] && pactl unload-module "$OURMOD" 2>/dev/null
}
trap cleanup EXIT INT TERM

echo "$0: ${SINK}.monitor -> adpcm4 mono ${RATE}Hz -> ${HOST}:${PORT}"
echo "$0: route audio with:  pactl move-sink-input <id> ${SINK}"

parec --device="${SINK}.monitor" --format=s16le --rate=44100 --channels=2 2>/dev/null \
  | ffmpeg -hide_banner -loglevel error \
        -f s16le -ar 44100 -ac 2 -i - \
        -ac 1 -ar "$RATE" \
        -af "aresample=out_sample_fmt=u8:dither_method=triangular" \
        -f u8 - \
  | python3 "$ENC" --stream \
  | socat -u - "TCP:${HOST}:${PORT}"
