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
HAND = (53.5 / N, 19.0 / N)   # the middle of the tesseract: it floats over the raised right hand
SIZE = 28                     # the size of the tesseract in grid pixels

# burgundy and gold: the wizard is mostly in shadow, lit from the right by the red tesseract over his hand
HOOD_DARK, HOOD, HOOD_MID, HOOD_LIGHT, RIM = (26, 4, 10), (52, 8, 18), (72, 12, 26), (98, 17, 34), (150, 30, 50)
ROBE_DARK, ROBE, ROBE_MID, ROBE_LIGHT = (20, 3, 8), (44, 6, 15), (64, 10, 22), (90, 15, 30)
SHADOW, SHADOW_2 = (7, 1, 4), (16, 3, 8)
GOLD_DARK, GOLD, GOLD_LIGHT = (112, 74, 28), (190, 140, 56), (244, 204, 116)
BEARD_SHADOW, BEARD_DARK, BEARD, BEARD_LIGHT = (86, 80, 92), (138, 132, 142), (196, 192, 196), (236, 232, 230)
SKIN_DARK, SKIN, SKIN_LIGHT = (92, 44, 42), (140, 74, 64), (196, 108, 96)
GEM, GEM_LIGHT, GEM_DARK = (220, 28, 48), (255, 150, 160), (120, 10, 24)
EYE, EYE_CORE = (255, 54, 70), (255, 214, 218)
OUTLINE = (4, 0, 2)
# where the eyes are (left pixel of each, two pixels wide): the views draw them glowing and blinking over the picture
EYES = ((26, 24), (35, 24))


def body(eyes=True):
    """a portrait: the hood with a gold-stitched edge, the face in shadow with a silver beard, the robe with a gold trim,
    a ruby brooch and runes, and the right hand raised with the palm up, under the place of the tesseract"""
    img = Image.new("RGBA", (N, N), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    # the robe: shoulders cut by the bottom of the picture, folds, the front edges with a gold trim
    d.polygon([(2, 64), (5, 52), (14, 44), (50, 44), (59, 52), (62, 64)], fill=ROBE)
    d.polygon([(2, 64), (5, 52), (14, 44), (21, 44), (17, 64)], fill=ROBE_DARK)
    d.polygon([(44, 44), (50, 44), (59, 52), (62, 64), (55, 64)], fill=ROBE_MID)
    for x0 in (9, 13):  # the folds on the left
        d.line([(x0, 64), (x0 + 3, 52)], fill=ROBE_DARK)
    d.line([(51, 64), (49, 54)], fill=ROBE_DARK)
    # the cloak's front edges, gold
    d.line([(24, 64), (25, 46)], fill=GOLD_DARK)
    d.line([(25, 64), (26, 46)], fill=GOLD)
    d.line([(40, 64), (39, 46)], fill=GOLD)
    d.line([(41, 64), (40, 46)], fill=GOLD_DARK)
    # runes stitched on the left side of the robe
    for (x, y) in ((10, 57), (14, 60), (9, 61)):
        d.point([(x, y), (x + 1, y), (x, y + 1), (x + 1, y + 2)], fill=GOLD_DARK)
    d.point([(12, 55), (13, 56), (12, 57)], fill=GOLD_DARK)

    # the hood: a pointed top falling back to the left, folds, a lit edge on the right
    d.polygon([(33, 2), (23, 1), (13, 6), (18, 12), (13, 30), (11, 45), (53, 45), (51, 30), (46, 11), (40, 5)], fill=HOOD)
    d.polygon([(33, 2), (23, 1), (13, 6), (18, 12), (13, 30), (11, 45), (19, 45), (21, 28), (24, 12)], fill=HOOD_DARK)
    d.polygon([(40, 5), (46, 11), (51, 30), (53, 45), (48, 45), (47, 29), (43, 12)], fill=HOOD_MID)
    d.line([(46, 12), (50, 30), (52, 44)], fill=HOOD_LIGHT)
    d.line([(17, 18), (15, 34)], fill=SHADOW_2)  # a fold
    d.line([(27, 4), (36, 4)], fill=HOOD_MID)   # light on the top
    d.point([(13, 6), (14, 6), (14, 7)], fill=GOLD)  # the gold tip

    # the opening of the hood: a dark face framed by a crimson rim stitched with gold
    face = [(25, 12), (39, 12), (43, 20), (43, 33), (38, 40), (26, 40), (21, 33), (21, 20)]
    d.polygon([(24, 10), (40, 10), (45, 19), (45, 34), (39, 42), (25, 42), (19, 34), (19, 19)], fill=RIM)
    d.polygon(face, fill=SHADOW)
    for i, (x, y) in enumerate([(24, 10), (27, 10), (30, 10), (33, 10), (36, 10), (39, 10), (42, 14), (44, 18), (45, 22), (45, 26), (45, 30),
                                (44, 34), (42, 37), (40, 40), (20, 18), (19, 22), (19, 26), (19, 30), (20, 34), (22, 37), (24, 40)]):
        d.point((x, y), fill=GOLD if i % 2 == 0 else GOLD_DARK)
    # inside the face: a hint of the nose and the cheeks lit red by the tesseract
    d.polygon([(23, 20), (41, 20), (40, 28), (24, 28)], fill=SHADOW_2)
    d.point([(31, 28), (32, 28)], fill=SKIN_DARK)
    d.point([(32, 27)], fill=(70, 30, 30))
    d.point([(39, 27), (40, 26)], fill=(60, 14, 22))
    if eyes:
        for (x, y) in EYES:
            d.point([(x, y), (x + 1, y)], fill=EYE)
            d.point([(x, y)], fill=EYE_CORE)

    # the silver beard and moustache, falling over the robe to a point
    beard = [(24, 30), (27, 29), (32, 31), (37, 29), (40, 30), (41, 36), (39, 44), (36, 50), (33, 55), (31, 55), (28, 50), (25, 44), (23, 36)]
    d.polygon(beard, fill=BEARD)
    d.polygon([(24, 30), (27, 29), (29, 31), (27, 40), (28, 50), (25, 44), (23, 36)], fill=BEARD_DARK)
    d.polygon([(37, 29), (40, 30), (41, 36), (39, 44), (37, 47), (38, 38)], fill=BEARD_LIGHT)
    d.line([(25, 30), (30, 32)], fill=BEARD_LIGHT)   # the moustache
    d.line([(34, 32), (39, 30)], fill=BEARD_LIGHT)
    d.point([(31, 33), (32, 33), (33, 33)], fill=SHADOW)  # the mouth under it
    for (x0, y0, x1, y1) in ((29, 36, 30, 46), (33, 36, 33, 50), (36, 35, 35, 45)):  # strands
        d.line([(x0, y0), (x1, y1)], fill=BEARD_SHADOW)
    d.point([(32, 55), (31, 54)], fill=BEARD_DARK)

    # the brooch that holds the cloak: a ruby in gold, on the left of the chest
    d.ellipse((15, 47, 21, 53), fill=GOLD)
    d.ellipse((16, 48, 20, 52), fill=GEM)
    d.point([(17, 49)], fill=GEM_LIGHT)
    d.point([(19, 51)], fill=GEM_DARK)
    d.point([(15, 50), (21, 50)], fill=GOLD_DARK)

    # the right arm raised: the sleeve from the shoulder, the hand with the palm up under the tesseract
    d.polygon([(47, 64), (45, 52), (47, 42), (52, 36), (57, 37), (58, 44), (56, 53), (58, 64)], fill=ROBE_MID)
    d.polygon([(47, 64), (45, 52), (47, 42), (50, 39), (51, 46), (50, 64)], fill=ROBE)
    d.line([(52, 36), (57, 37)], fill=GOLD)       # the cuff
    d.line([(51, 37), (58, 38)], fill=GOLD_DARK)
    d.line([(57, 39), (57, 50)], fill=(150, 30, 50))  # the sleeve lit by the tesseract
    # the hand: an open palm turned up, the fingers a little curled, lit red from the tesseract above
    hand = ["....l..l....",
            "..l.Sl.Sl.l.",
            "..SlSS.SS.Sl",
            ".lSSSSSSSSSS",
            "lSSSSSSSSSSd",
            ".dSSSSSSSSd.",
            "..ddSSSSdd..",
            "....dddd...."]
    colors = {"l": SKIN_LIGHT, "S": SKIN, "d": SKIN_DARK}
    for j, row in enumerate(hand):
        for i, ch in enumerate(row):
            if ch in colors:
                img.putpixel((47 + i, 28 + j), colors[ch] + (255,))

    # the outline around the whole figure
    alpha = img.getchannel("A")
    grown = alpha.filter(ImageFilter.MaxFilter(3))
    px, a, g = img.load(), alpha.load(), grown.load()
    for y in range(N):
        for x in range(N):
            if g[x, y] and not a[x, y]:
                px[x, y] = OUTLINE + (255,)
    # a red light from the tesseract on the right edges of the hood and the shoulder
    for y in range(N):
        for x in range(N - 1, 0, -1):
            if a[x, y] and not a[min(N - 1, x + 1), y]:
                if 6 <= y <= 60:
                    px[x, y] = (160, 34, 54, 255)
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
    base = body(eyes=False)  # the views draw the eyes: they glow and blink
    OUT.mkdir(parents=True, exist_ok=True)
    base.resize((N * SCALE, N * SCALE), Image.NEAREST).save(OUT / "wizard_body.png")
    desktop = HERE.parent.parent / "launcher" / "resources" / "sculk_theme" / "wizard_body.png"
    if desktop.parent.exists():
        base.resize((N * SCALE, N * SCALE), Image.NEAREST).save(desktop)
    still = with_tesseract(body())
    still.save(OUT / "wizard.png")
    preview = Image.new("RGBA", still.size, (30, 6, 12, 255))
    preview.alpha_composite(still)
    preview.save(HERE / "wizard_preview.png")


if __name__ == "__main__":
    main()
