#!/usr/bin/env python3
"""Turns a pixel-art picture of the Warden head into the grid used by the AI agent.

usage: python3 warden_from_picture.py picture.ppm [columns rows left top right]
  (convert a PNG or JPG with ImageMagick first: convert picture.jpg picture.ppm)

The picture is cut into columns x rows cells between the left and right edge (in picture pixels); every cell gets the
average color of its middle part, white becomes transparent, and nearly equal colors share one letter. Paste the
printed rows and colors into WARDEN / COLORS in warden_icon.py and into s_head / colorOf() in
launcher/ui/widgets/WardenWidget.cpp. The defaults fit the picture the current head was taken from.
"""
import sys


def main():
    path = sys.argv[1]
    cols, rows, left, top, right = (int(v) for v in (sys.argv[2:7] if len(sys.argv) >= 7 else (36, 18, 2, 10, 1257)))
    data = open(path, "rb").read()
    header = data.split(b"\n", 3)
    width, height = map(int, header[1].split())
    pixels = header[3]

    def at(x, y):
        i = (y * width + x) * 3
        return pixels[i], pixels[i + 1], pixels[i + 2]

    cell = (right - left) / cols
    palette, grid = [], []
    for r in range(rows):
        line = ""
        for c in range(cols):
            cx, cy = left + (c + 0.5) * cell, top + (r + 0.5) * cell
            samples = [at(int(cx + dx), int(cy + dy)) for dx in range(-10, 11, 5) for dy in range(-10, 11, 5)
                       if 0 <= int(cx + dx) < width and 0 <= int(cy + dy) < height]
            color = tuple(round(sum(s[i] for s in samples) / len(samples)) for i in range(3))
            if min(color) > 225:
                line += "."
                continue
            near = [(sum(abs(a - b) for a, b in zip(p, color)), i) for i, p in enumerate(palette)]
            near = [n for n in near if n[0] <= 6]
            if near:
                index = min(near)[1]
            else:
                palette.append(color)
                index = len(palette) - 1
            line += chr(65 + index)
        grid.append(line)
    for line in grid:
        print(f'    "{line}",')
    for i, color in enumerate(palette):
        print(f'{chr(65 + i)} #%02x%02x%02x' % color)


if __name__ == "__main__":
    main()
