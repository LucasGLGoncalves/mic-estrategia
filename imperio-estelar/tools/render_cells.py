#!/usr/bin/env python3
"""Render a .cells dump (from ./imperio --snapshot DIR) into a PNG, emulating a terminal."""
import sys
from PIL import Image, ImageDraw, ImageFont

FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
BOLD = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf"


def render(path, out, size=15):
    font = ImageFont.truetype(FONT, size)
    try:
        bold = ImageFont.truetype(BOLD, size)
    except OSError:
        bold = font
    cw = int(font.getlength("M") + 0.5)
    ch = int(size * 1.22)
    rows = [l.split() for l in open(path, encoding="utf-8").read().splitlines()]
    h = len(rows)
    w = max(len(r) for r in rows)
    img = Image.new("RGB", (w * cw, h * ch), (0, 0, 0))
    d = ImageDraw.Draw(img)
    for y, r in enumerate(rows):
        for x, tok in enumerate(r):
            c, fg, bg, at = tok.split(",")
            c = int(c, 16)
            fg = tuple(int(fg[i:i + 2], 16) for i in (0, 2, 4))
            bg = tuple(int(bg[i:i + 2], 16) for i in (0, 2, 4))
            d.rectangle([x * cw, y * ch, (x + 1) * cw - 1, (y + 1) * ch - 1], fill=bg)
            if c > 32:
                d.text((x * cw, y * ch), chr(c), font=bold if int(at) & 1 else font, fill=fg)
    img.save(out)


if __name__ == "__main__":
    render(sys.argv[1], sys.argv[2])
