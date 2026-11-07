#!/usr/bin/env python3
"""Vellora's AI pack builder: a burgundy wizard in a hood, in pixel art, holding a red tesseract.

Writes res/drawable-nodpi/wizard.png (with a still tesseract, for small pictures) and wizard_body.png (without it:
WizardView draws the turning 4D tesseract over the hand), and art/wizard_preview.png. The hand point for the
tesseract is HAND (in 0..1 of the picture), the same numbers are in WizardView.java.
Run:  python3 art/wizard.py   (from android/)
"""
import math
import pathlib

from PIL import Image, ImageDraw, ImageFilter

HERE = pathlib.Path(__file__).resolve().parent
OUT = HERE.parent / "res" / "drawable-nodpi"
N = 64
SCALE = 8
HAND = (53.0 / N, 16.0 / N)   # the middle of the tesseract: it floats by the right shoulder
SIZE = 30                     # the size of the tesseract in grid pixels

# dark burgundy: the wizard is mostly shadow, lit a little by the red tesseract
HOOD_DARK, HOOD, HOOD_LIGHT = (22, 3, 8), (44, 7, 15), (68, 12, 24)
ROBE_DARK, ROBE, ROBE_LIGHT = (18, 2, 6), (36, 5, 12), (56, 9, 19)
SHADOW = (6, 1, 3)
GOLD, GOLD_DARK = (150, 104, 44), (96, 62, 26)
EYE = (255, 54, 70)
OUTLINE = (4, 0, 2)


def body():
    """only the head in a big hood and the shoulders, like a portrait"""
    img = Image.new("RGBA", (N, N), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    # the shoulders and the top of the robe, cut by the bottom of the picture
    d.polygon([(4, 64), (8, 50), (18, 43), (46, 43), (56, 50), (60, 64)], fill=ROBE)
    d.polygon([(4, 64), (8, 50), (18, 43), (24, 43), (20, 64)], fill=ROBE_DARK)
    d.polygon([(42, 43), (46, 43), (56, 50), (60, 64), (50, 64)], fill=ROBE_LIGHT)
    # the collar with a gold edge
    d.polygon([(22, 42), (32, 54), (42, 42)], fill=HOOD_DARK)
    d.line([(22, 42), (32, 54), (42, 42)], fill=GOLD)
    d.point([(32, 55), (31, 54), (33, 54)], fill=GOLD_DARK)
    # the big hood with a pointed top falling back to the left
    d.polygon([(33, 3), (24, 2), (15, 7), (20, 13), (15, 30), (13, 44), (51, 44), (49, 30), (44, 12)], fill=HOOD)
    d.polygon([(33, 3), (24, 2), (15, 7), (20, 13), (15, 30), (13, 44), (22, 44), (24, 26), (26, 12)], fill=HOOD_DARK)
    d.polygon([(41, 10), (44, 12), (49, 30), (51, 44), (46, 44), (45, 28)], fill=HOOD_LIGHT)
    d.point([(15, 7), (16, 7)], fill=GOLD)
    # the dark face with two glowing red eyes
    d.polygon([(25, 14), (39, 14), (42, 22), (41, 34), (36, 39), (28, 39), (23, 34), (22, 22)], fill=SHADOW)
    for x in (27, 28, 29, 35, 36, 37):
        d.point((x, 25), fill=EYE)
    d.point([(28, 24), (36, 24)], fill=(255, 140, 150))
    d.point([(28, 26), (36, 26)], fill=(150, 20, 34))
    # the outline around the whole figure
    alpha = img.getchannel("A")
    grown = alpha.filter(ImageFilter.MaxFilter(3))
    px, a, g = img.load(), alpha.load(), grown.load()
    for y in range(N):
        for x in range(N):
            if g[x, y] and not a[x, y]:
                px[x, y] = OUTLINE + (255,)
    # a faint red light from the tesseract on the right edges of the hood and the shoulder
    for y in range(N):
        for x in range(N - 1, 0, -1):
            if a[x, y] and not a[min(N - 1, x + 1), y]:
                if 8 <= y <= 56:
                    px[x, y] = (120, 24, 38, 255)
                break
    return img


def tesseract_points(t, size):
    """the 16 corners of a tesseract turned by t (radians), seen in 2D; and its 32 edges"""
    pts = []
    for i in range(16):
        v = [1 if i >> k & 1 else -1 for k in range(4)]
        x, y, z, w = v
        a, b = t, t * 0.7
        x, w = x * math.cos(a) - w * math.sin(a), x * math.sin(a) + w * math.cos(a)
        y, z = y * math.cos(b) - z * math.sin(b), y * math.sin(b) + z * math.cos(b)
        x, z = x * math.cos(0.5) - z * math.sin(0.5), x * math.sin(0.5) + z * math.cos(0.5)
        k4 = 1 / (3 - w)
        x, y, z = x * k4, y * k4, z * k4
        k3 = 1 / (4 - z)
        pts.append((x * k3 * size, y * k3 * size))
    edges = [(i, i ^ (1 << k)) for i in range(16) for k in range(4) if i < i ^ (1 << k)]
    return pts, edges


def with_tesseract(base, t=0.6):
    big = base.resize((N * SCALE, N * SCALE), Image.NEAREST)
    glow = Image.new("RGBA", big.size, (0, 0, 0, 0))
    gd = ImageDraw.Draw(glow)
    cx, cy = HAND[0] * big.size[0], HAND[1] * big.size[1]
    r = 10 * SCALE
    gd.ellipse((cx - r, cy - r, cx + r, cy + r), fill=(255, 40, 60, 90))
    big.alpha_composite(glow.filter(ImageFilter.GaussianBlur(6 * SCALE)))
    d = ImageDraw.Draw(big)
    pts, edges = tesseract_points(t, SIZE * SCALE)
    for a, b in edges:
        d.line([(cx + pts[a][0], cy + pts[a][1]), (cx + pts[b][0], cy + pts[b][1])], fill=(255, 70, 84, 255), width=SCALE // 2 + 1)
    for p in pts:
        d.ellipse((cx + p[0] - SCALE * 0.6, cy + p[1] - SCALE * 0.6, cx + p[0] + SCALE * 0.6, cy + p[1] + SCALE * 0.6), fill=(255, 200, 205, 255))
    return big


def main():
    base = body()
    OUT.mkdir(parents=True, exist_ok=True)
    base.resize((N * SCALE, N * SCALE), Image.NEAREST).save(OUT / "wizard_body.png")
    still = with_tesseract(base)
    still.save(OUT / "wizard.png")
    preview = Image.new("RGBA", still.size, (30, 6, 12, 255))
    preview.alpha_composite(still)
    preview.save(HERE / "wizard_preview.png")


if __name__ == "__main__":
    main()
