#!/usr/bin/env python3
"""Creative 4-bit ADPCM encoder, bit-exact mirror of the DOSBox/86Box decoder."""
import sys
scaleMap4 = [
    0,  1,  2,  3,  4,  5,  6,  7,  0,  -1,  -2,  -3,  -4,  -5,  -6,  -7,
    1,  3,  5,  7,  9, 11, 13, 15, -1,  -3,  -5,  -7,  -9, -11, -13, -15,
    2,  6, 10, 14, 18, 22, 26, 30, -2,  -6, -10, -14, -18, -22, -26, -30,
    4, 12, 20, 28, 36, 44, 52, 60, -4, -12, -20, -28, -36, -44, -52, -60]
adjustMap4 = [
      0, 0, 0, 0, 0, 16, 16, 16,
      0, 0, 0, 0, 0, 16, 16, 16,
    240, 0, 0, 0, 0, 16, 16, 16,
    240, 0, 0, 0, 0, 16, 16, 16,
    240, 0, 0, 0, 0, 16, 16, 16,
    240, 0, 0, 0, 0, 16, 16, 16,
    240, 0, 0, 0, 0,  0,  0,  0,
    240, 0, 0, 0, 0,  0,  0,  0]
def s8(x): x &= 0xff; return x-256 if x>127 else x
def encode(data):
    out = bytearray([data[0]])          # reference sample
    ref, step = data[0], 0
    nibbles = []
    for want in data[1:]:
        best, bestn = None, 0
        for nib in range(16):
            t = nib + step
            t = 0 if t < 0 else (63 if t > 63 else t)
            r = ref + scaleMap4[t]
            r = 0 if r < 0 else (255 if r > 255 else r)
            d = abs(r - want)
            if best is None or d < best:
                best, bestn = d, nib
        t = bestn + step
        t = 0 if t < 0 else (63 if t > 63 else t)
        ref += scaleMap4[t]
        ref = 0 if ref < 0 else (255 if ref > 255 else ref)
        step = s8(step + adjustMap4[t])
        nibbles.append(bestn)
    for i in range(0, len(nibbles)-1, 2):
        out.append((nibbles[i] << 4) | nibbles[i+1])   # high nibble first
    return bytes(out)


# ---- fast streaming encoder (table lookup per sample) ----
_BEST = {}
def _build():
    for step in range(-128,128):
        row = bytearray(511)
        for d in range(-255,256):
            bn, be = 0, None
            for n in range(16):
                t = n + step
                t = 0 if t < 0 else (63 if t > 63 else t)
                e = abs(scaleMap4[t] - d)
                if be is None or e < be: be, bn = e, n
            row[d+255] = bn
        _BEST[step] = row
_build()

class Streamer:
    def __init__(self):
        self.ref = None; self.step = 0; self.spare = None
    def feed(self, data):
        out = bytearray(); nibbles = []
        if self.ref is None and data:
            self.ref = data[0]; self.step = 0; out.append(data[0]); data = data[1:]
        for want in data:
            d = want - self.ref
            if d < -255: d = -255
            elif d > 255: d = 255
            nib = _BEST[self.step][d+255]
            t = nib + self.step
            t = 0 if t < 0 else (63 if t > 63 else t)
            self.ref += scaleMap4[t]
            self.ref = 0 if self.ref < 0 else (255 if self.ref > 255 else self.ref)
            self.step = s8(self.step + adjustMap4[t])
            nibbles.append(nib)
        if self.spare is not None:
            nibbles.insert(0, self.spare); self.spare = None
        if len(nibbles) & 1: self.spare = nibbles.pop()
        for i in range(0, len(nibbles), 2):
            out.append((nibbles[i] << 4) | nibbles[i+1])
        return bytes(out)

if __name__ == "__main__":
    if "--stream" in sys.argv:
        st = Streamer()
        while True:
            chunk = sys.stdin.buffer.read(2048)
            if not chunk: break
            sys.stdout.buffer.write(st.feed(chunk)); sys.stdout.buffer.flush()
    else:
        sys.stdout.buffer.write(encode(sys.stdin.buffer.read()))
