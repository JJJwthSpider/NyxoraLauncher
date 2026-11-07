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
HAND = (47.5 / N, 20.5 / N)   # the middle of the tesseract

HOOD_DARK, HOOD, HOOD_LIGHT = (58, 10, 22), (110, 22, 38), (150, 38, 56)
ROBE_DARK, ROBE, ROBE_LIGHT = (50, 8, 18), (96, 18, 32), (128, 30, 46)
SHADOW = (14, 3, 7)
GOLD, GOLD_DARK = (226, 172, 74), (160, 108, 40)
EYE = (255, 70, 84)
SKIN, SKIN_DARK = (232, 190, 160), (196, 146, 118)


def body():
    img = Image.new("RGBA", (N, N), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    # the robe: a wide cone to the ground
    d.polygon([(22, 28), (38, 28), (47, 62), (13, 62)], fill=ROBE)
    d.polygon([(22, 28), (27, 28), (20, 62), (13, 62)], fill=ROBE_DARK)
    d.polygon([(35, 28), (38, 28), (47, 62), (42, 62)], fill=ROBE_LIGHT)
    # gold trim down the middle and on the hem
    d.line([(30, 30), (30, 61)], fill=GOLD)
    d.line([(31, 30), (31, 61)], fill=GOLD_DARK)
    d.line([(14, 61), (46, 61)], fill=GOLD)
    d.line([(14, 62), (46, 62)], fill=GOLD_DARK)
    # a belt
    d.rectangle((23, 40, 38, 41), fill=SHADOW)
    d.rectangle((29, 39, 32, 42), fill=GOLD)
    # the hood: a pointed cap falling back to the left, wide around the face
    d.polygon([(30, 4), (22, 2), (14, 6), (20, 12), (17, 30), (43, 30), (40, 12)], fill=HOOD)
    d.polygon([(30, 4), (22, 2), (14, 6), (20, 12), (17, 30), (24, 30), (24, 12)], fill=HOOD_DARK)
    d.polygon([(37, 10), (40, 12), (43, 30), (39, 30)], fill=HOOD_LIGHT)
    d.point([(14, 6), (15, 6)], fill=GOLD)  # a gold tip
    # the dark opening and the glowing eyes
    d.polygon([(25, 12), (35, 12), (37, 18), (36, 27), (24, 27), (23, 18)], fill=SHADOW)
    for x in (27, 28, 32, 33):
        d.point((x, 19), fill=EYE)
    d.point([(27, 18), (33, 18)], fill=(255, 150, 160))
    # the left arm hanging down (dark side)
    d.polygon([(19, 30), (23, 30), (22, 46), (17, 46)], fill=ROBE_DARK)
    d.rectangle((17, 46, 21, 48), fill=SKIN_DARK)
    # the right arm reaching out and up to hold the tesseract
    d.polygon([(36, 30), (40, 29), (47, 33), (45, 37), (37, 35)], fill=ROBE_LIGHT)
    d.polygon([(44, 32), (48, 30), (50, 34), (46, 36)], fill=HOOD)  # the wide sleeve end
    d.line([(44, 32), (48, 30)], fill=GOLD)
    # the open hand, palm up
    d.rectangle((45, 31, 49, 32), fill=SKIN)
    d.point([(45, 30), (47, 30), (49, 30)], fill=SKIN)
    d.point([(46, 32), (48, 32)], fill=SKIN_DARK)
    # a dark pixel outline around the whole figure, so it stands out on any background
    alpha = img.getchannel("A")
    grown = alpha.filter(ImageFilter.MaxFilter(3))
    px, a, g = img.load(), alpha.load(), grown.load()
    for y in range(N):
        for x in range(N):
            if g[x, y] and not a[x, y]:
                px[x, y] = (12, 2, 5, 255)
    # a soft red rim light on the right edges (the light of the tesseract)
    for y in range(N):
        for x in range(N - 1, 0, -1):
            if a[x, y] and not a[min(N - 1, x + 1), y] and y < 40:
                px[x, y] = (196, 60, 78, 255)
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
    r = 12 * SCALE
    gd.ellipse((cx - r, cy - r, cx + r, cy + r), fill=(255, 40, 60, 90))
    big.alpha_composite(glow.filter(ImageFilter.GaussianBlur(6 * SCALE)))
    d = ImageDraw.Draw(big)
    pts, edges = tesseract_points(t, 38 * SCALE)
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
