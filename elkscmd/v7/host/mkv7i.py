#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""
mkv7i - turn a picture into something v7show can put on a VEGA VGA.

    mkv7i.py -m 640x480 photo.jpg photo.v7i
    mkv7i.py -m 800x600 --pattern smpte.v7i

The 16 colour modes are planar, and the plane is the unit the card wants: a
byte written with the Map Mask selecting one plane lands in that plane alone.
So the file holds plane 0 whole, then plane 1, and v7show blits each in one
pass without shuffling bits on a 8086.  Mode 13h is chunky and goes out as it
is.

Pictures are fitted to the mode rather than stretched, because a mode with a
non-square pixel is normal here - 640x480 is square, 752x410 and 720x540 are
not - and stretching to fill would put a different distortion in every mode,
which is the opposite of what a test image is for.

Copyright (C) 2026 G Keet
Licensed under the GNU General Public License version 2, the same terms as the
ELKS kernel.
"""

import argparse
import struct
import sys

from PIL import Image, ImageDraw

MAGIC = b"V7I1"

# mode name -> (BIOS mode, width, height, bits per pixel)
MODES = {
    "640x480": (0x12, 640, 480, 4),     # standard VGA, always available
    "320x200": (0x13, 320, 200, 8),     # standard VGA, chunky
    "752x410": (0x60, 752, 410, 4),     # Video Seven
    "720x540": (0x61, 720, 540, 4),     # Video Seven
    "800x600": (0x62, 800, 600, 4),     # Video Seven
}

# The default 16 colour VGA palette, which is what a mode set leaves in the DAC.
EGA16 = [
    (0, 0, 0), (0, 0, 170), (0, 170, 0), (0, 170, 170),
    (170, 0, 0), (170, 0, 170), (170, 85, 0), (170, 170, 170),
    (85, 85, 85), (85, 85, 255), (85, 255, 85), (85, 255, 255),
    (255, 85, 85), (255, 85, 255), (255, 255, 85), (255, 255, 255),
]


def palette_image(colours):
    """A PIL palette image to quantise against."""
    pal = Image.new("P", (1, 1))
    flat = []
    for rgb in colours:
        flat += list(rgb)
    flat += [0, 0, 0] * (256 - len(colours))
    pal.putpalette(flat)
    return pal


def default_256():
    """
    The standard VGA DAC after a mode 13h set: 16 EGA colours, 16 greys, then
    a 216 entry hue/saturation/value block.  Only the first 16 and the greys
    are relied on here; the block is filled in the same order the BIOS does so
    a picture quantised against it looks right without loading a palette.
    """
    pal = list(EGA16)
    for i in range(16):
        v = i * 17
        pal.append((v, v, v))
    # 9 hues x 3 saturations x 8 values, the BIOS layout, approximated
    import colorsys
    for v in range(8):
        for s in range(3):
            for hstep in range(9):
                h = hstep / 9.0
                sat = 1.0 - s * 0.33
                val = 1.0 - v * 0.11
                r, g, b = colorsys.hsv_to_rgb(h, sat, val)
                pal.append((int(r * 255), int(g * 255), int(b * 255)))
    return (pal + [(0, 0, 0)] * 256)[:256]


def test_pattern(w, h, ncolours):
    """
    A pattern built to be judged by eye at arm's length: colour bars to show
    every entry of the palette is reaching the DAC, a one pixel border and
    corner marks to show the whole raster is addressed and nothing is clipped,
    a vertical wedge of single pixel lines to show the dot clock resolves
    them, and a horizontal ramp for the same on lines.
    """
    img = Image.new("P", (w, h), 0)
    img.putpalette(palette_image(EGA16 if ncolours == 16 else default_256())
                   .getpalette())
    d = ImageDraw.Draw(img)

    bars = 16
    bw = w // bars
    for i in range(bars):
        d.rectangle([i * bw, 0, (i + 1) * bw - 1, h // 3], fill=i)

    # single pixel vertical lines, then two, then three: a resolution wedge
    y0, y1 = h // 3 + 8, h * 2 // 3 - 8
    x = 8
    for period in (2, 3, 4, 6, 8):
        for _ in range(12):
            if x + period >= w - 8:
                break
            d.rectangle([x, y0, x, y1], fill=15)
            x += period
        x += 16

    # horizontal ramp: one pixel lines in the lower third
    y = h * 2 // 3 + 8
    step = 2
    while y < h - 12:
        d.line([8, y, w - 9, y], fill=15)
        y += step
        step += 1

    d.rectangle([0, 0, w - 1, h - 1], outline=15)
    for cx, cy in ((0, 0), (w - 16, 0), (0, h - 16), (w - 16, h - 16)):
        d.rectangle([cx, cy, cx + 15, cy + 15], outline=12)
    d.text((12, h // 3 - 14), "%dx%d" % (w, h), fill=15)
    return img


def fit(img, w, h, ncolours):
    """Scale to fit inside the mode and centre it on black."""
    src = img.convert("RGB")
    scale = min(w / src.width, h / src.height)
    new = (max(1, int(src.width * scale)), max(1, int(src.height * scale)))
    src = src.resize(new, Image.LANCZOS)
    canvas = Image.new("RGB", (w, h), (0, 0, 0))
    canvas.paste(src, ((w - new[0]) // 2, (h - new[1]) // 2))
    colours = EGA16 if ncolours == 16 else default_256()
    return canvas.quantize(palette=palette_image(colours),
                           dither=Image.FLOYDSTEINBERG)


def planar(img, w, h):
    """Split 4 bit pixels into four planes of packed bits, plane 0 first."""
    px = img.load()
    stride = (w + 7) // 8
    out = bytearray()
    for plane in range(4):
        bit = 1 << plane
        for y in range(h):
            acc = 0
            nbits = 0
            row = bytearray()
            for x in range(w):
                acc = (acc << 1) | (1 if px[x, y] & bit else 0)
                nbits += 1
                if nbits == 8:
                    row.append(acc)
                    acc = 0
                    nbits = 0
            if nbits:
                row.append(acc << (8 - nbits))
            out += row.ljust(stride, b"\0")
    return bytes(out), stride


def chunky(img, w, h):
    px = img.load()
    out = bytearray()
    for y in range(h):
        for x in range(w):
            out.append(px[x, y])
    return bytes(out), w


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-m", "--mode", default="640x480", choices=sorted(MODES))
    ap.add_argument("--pattern", action="store_true",
                    help="generate a test pattern instead of reading a file")
    ap.add_argument("source", nargs="?")
    ap.add_argument("out")
    a = ap.parse_args()

    mode, w, h, bpp = MODES[a.mode]
    ncolours = 1 << bpp

    if a.pattern:
        img = test_pattern(w, h, ncolours)
    else:
        if not a.source:
            ap.error("give a source image or --pattern")
        img = fit(Image.open(a.source), w, h, ncolours)

    if bpp == 4:
        data, stride = planar(img, w, h)
        planes = 4
    else:
        data, stride = chunky(img, w, h)
        planes = 1

    hdr = MAGIC + struct.pack("<BBHHHH", mode, planes, w, h, stride, 0)
    assert len(hdr) == 14
    with open(a.out, "wb") as f:
        f.write(hdr)
        f.write(data)
    print("%s: mode %02x %dx%d, %d plane%s, %d bytes"
          % (a.out, mode, w, h, planes, "" if planes == 1 else "s",
             len(hdr) + len(data)), file=sys.stderr)


if __name__ == "__main__":
    main()
