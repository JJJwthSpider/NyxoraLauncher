#!/usr/bin/env python3
"""Arkan, Nyxora's AI wizard, as a 3D mob in the style of a Minecraft mod: a model of boxes and its texture.

It is the old pixel-art Arkan made of boxes: a deep indigo hood with its pointed top falling back to the left (a gold
tip at the end) and a violet rim stitched with gold around the opening, the face lost in the shadow of the hood with
only the violet eyes glowing in it, a silver beard falling to a point, a cloak with gold edges, gold runes and a ruby
brooch, a black belt with a gold buckle, a gold hem, and the right hand of the picture raised, the palm up, under the
red tesseract.

The model (arkan_model.json) is a list of boxes in Minecraft units (y up, the feet at 0, the face towards +z), each
with its part (head, body, rarm, larm), its place and size and its texture corner (u, v) in the usual Minecraft box
layout: top and bottom on the first row, then right, front, left and back. The texture (arkan.png) is painted here
pixel by pixel and the boxes are packed on it in rows. Pixels with alpha 254 glow (the eyes, the ruby): the views draw
them at full brightness; the glowing pixels of the head are the eyes, they close when he blinks. "magicHand" is the
hand under the tesseract, "glows" are the soft lights the views draw over the model (the ruby).

WizardView.java (phone) and MoonyWidget.cpp (PC) draw it in 3D: he floats over a magic circle, turns a little, looks
around, holds the violet tesseract over his raised hand, and the tesseract lights the cloth near it violet. The small
pictures of him (res/drawable-nodpi/wizard.png and launcher/resources/sculk_theme/wizard.png: chat avatars, list icons)
are stills of that render, a close-up of his head and shoulders.

Run:  python3 art/arkan3d.py   (from android/)
"""
import json
import pathlib
import random

from PIL import Image

HERE = pathlib.Path(__file__).resolve().parent
ANDROID = HERE.parent / "assets"
DESKTOP = HERE.parent.parent / "launcher" / "resources" / "sculk_theme"
TW = 128

# the palette of the old Arkan, a little lighter: the 3D views shade the faces and the cloth must not turn black
HOOD = [(7, 5, 30), (12, 8, 46), (17, 10, 60), (24, 13, 76), (40, 21, 107)]
ROBE = [(4, 3, 22), (9, 6, 36), (13, 8, 50), (19, 11, 66), (30, 16, 86)]
RIM_D, RIM, RIM_L = (61, 36, 141), (92, 58, 185), (106, 70, 204)
GOLD_D, GOLD, GOLD_L = (120, 82, 30), (192, 142, 56), (240, 202, 112)
BEARD_S, BEARD_D, BEARD, BEARD_L = (96, 90, 102), (146, 140, 150), (200, 196, 200), (238, 234, 232)
SKIN_D, SKIN, SKIN_L = (92, 60, 123), (150, 101, 185), (178, 125, 217)
SHADOW, SHADOW_2 = (1, 2, 7), (4, 4, 18)
LEATHER, LEATHER_L = (7, 6, 14), (19, 14, 30)
EYE, EYE_CORE = (145, 84, 255, 254), (232, 220, 255, 254)
GEM_D, GEM, GEM_L = (80, 37, 185, 254), (117, 60, 232, 254), (196, 166, 255, 254)
CLEAR = (0, 0, 0, 0)

rnd = random.Random(7)
img = Image.new("RGBA", (TW, 128), CLEAR)
px = img.load()

BOXES = []  # the model
shelf = {"x": 0, "y": 0, "h": 0}  # the packing of the boxes on the texture, row by row


def faces(u, v, w, h, d):
    """the rectangles of a box's texture: name -> (x, y, width, height)"""
    return {"top": (u + d, v, w, d), "bottom": (u + d + w, v, w, d), "right": (u, v + d, d, h), "front": (u + d, v + d, w, h),
            "left": (u + d + w, v + d, d, h), "back": (u + 2 * d + w, v + d, w, h)}


def put(x, y, c):
    if 0 <= x < TW and 0 <= y < img.height:
        px[x, y] = c if len(c) == 4 else c + (255,)


def shade(palette, s):
    """a cloth colour with a little grain"""
    return palette[max(0, min(len(palette) - 1, s + rnd.choice((-1, 0, 0, 0, 1))))]


def box(part, frm, size, inflate=0.0):
    """a box of the model; its place on the texture is the next free one"""
    w, h, d = (int(v) for v in size)
    fw, fh = 2 * (d + w), d + h
    if shelf["x"] + fw > TW:
        shelf["x"], shelf["y"], shelf["h"] = 0, shelf["y"] + shelf["h"], 0
    uv = [shelf["x"], shelf["y"]]
    shelf["x"] += fw
    shelf["h"] = max(shelf["h"], fh)
    BOXES.append({"part": part, "from": frm, "size": size, "uv": uv, "inflate": inflate})
    return faces(uv[0], uv[1], w, h, d)


def fill(rect, painter):
    x0, y0, w, h = rect
    for j in range(h):
        for i in range(w):
            c = painter(i, j, w, h)
            if c is not None:
                put(x0 + i, y0 + j, c)


def paint_all(b, painter, skip=()):
    for name, r in b.items():
        if name not in skip:
            fill(r, lambda i, j, w, h, name=name: painter(name, i, j, w, h))


def stud(k):
    """the gold stitches on the violet rim, every other pixel"""
    return (GOLD if k % 4 == 0 else GOLD_D) if k % 2 == 0 else None


# ---------------------------------------------------------------- the head: no face, only the shadow and the violet eyes
head = box("head", [-4, 24, -4], [8, 8, 8])
paint_all(head, lambda n, i, j, w, h: SHADOW)


def face_paint(i, j, w, h):
    if 2 <= j <= 4 and 1 <= i <= 6:
        return SHADOW_2  # the eyes catch a little light
    if j == 5 and i in (3, 4):
        return (24, 16, 44)  # a hint of the nose
    return SHADOW


fill(head["front"], face_paint)
for x, c in ((1, EYE_CORE), (2, EYE), (5, EYE_CORE), (6, EYE)):  # the eyes, two pixels each, as in the old picture
    put(head["front"][0] + x, head["front"][1] + 3, c)

# ---------------------------------------------------------------- the hood: deep, with the opening for the shadow
hood = box("head", [-4, 24, -4], [8, 9, 8], inflate=0.6)


def hood_paint(n, i, j, w, h):
    if n == "front":
        if 1 <= i <= 6 and j >= 2:
            return CLEAR  # the opening
        return RIM if j == 1 or i in (0, 7) else shade(HOOD, 3)
    if n == "bottom":
        return CLEAR if 1 <= i <= 6 and j >= 3 else shade(HOOD, 1)
    if n == "top":
        return shade(HOOD, 3 if j > 3 else 2)
    if n == "back":
        return shade(HOOD, 1 if i % 3 == 0 else 2)
    # the sides: folds; the left of the picture (the right side of the hood) is in shadow
    base = 1 if n == "right" else 3
    return shade(HOOD, base - (1 if j % 4 == 3 else 0))


paint_all(hood, hood_paint)
# the brim: the violet rim stitched with gold stands out around the opening, so the shadow inside is deep
brim_top = box("head", [-4.5, 30.6, 4], [9, 2, 2])
paint_all(brim_top, lambda n, i, j, w, h: (stud(i) or RIM) if n == "front" and j == 1 else
          (RIM_L if j == 0 else RIM) if n == "front" else SHADOW_2 if n == "bottom" else shade(HOOD, 3) if n == "top" else RIM_D)
for side, x in (("r", -4.5), ("l", 3.5)):
    brim = box("head", [x, 24.6, 4], [1, 6, 2])
    inner = "left" if side == "r" else "right"
    paint_all(brim, lambda n, i, j, w, h, inner=inner: (stud(j + 1) or RIM) if n == "front" else SHADOW_2 if n == inner else
              RIM_D if n in ("top", "bottom") else shade(HOOD, 2))
# the pointed top falling back to the left, a gold tip at the end
for frm, size in (([-2.5, 33.6, -3], [5, 2, 5]), ([-4.2, 35.2, -2.6], [3, 2, 3]), ([-6.2, 35.8, -2.2], [2, 2, 2])):
    tip = box("head", frm, size)
    paint_all(tip, lambda n, i, j, w, h: shade(HOOD, 3 if n in ("top", "left", "front") else 1 if n == "bottom" else 2))
gold_tip = box("head", [-7.2, 35.2, -1.7], [1, 1, 1])
paint_all(gold_tip, lambda n, i, j, w, h: GOLD_L if n == "top" else GOLD)

# ---------------------------------------------------------------- the silver beard, falling to a point
beard_parts = (([-3, 19, 3.5], [6, 7, 2]), ([-2.5, 15, 3.2], [5, 4, 2]), ([-1.5, 12, 3.4], [3, 3, 1]), ([-0.5, 10, 3.3], [1, 2, 1]))
for k, (frm, size) in enumerate(beard_parts):
    beard = box("head", frm, size)

    def beard_paint(n, i, j, w, h, k=k):
        if n != "front":
            return BEARD_D if n in ("right", "bottom") else BEARD
        if k == 0 and j == 0:
            return BEARD_L if i not in (2, 3) else BEARD  # the moustache
        if k == 0 and j == 1 and i in (2, 3):
            return BEARD_S  # the mouth under it
        if k == 0 and j == h - 1 and i in (0, w - 1):
            return CLEAR
        if i == 0:
            return BEARD_D  # the left of the beard in shadow, the right lit
        if i == w - 1 and w > 2:
            return BEARD_L
        return BEARD_D if (i + k) % 2 == 1 and j % 3 == 1 else BEARD  # strands
    paint_all(beard, beard_paint)

# ---------------------------------------------------------------- the cloak: the hood falls on the shoulders, gold edges
cowl = box("body", [-6, 20, -3.5], [12, 4, 7])


def cowl_paint(n, i, j, w, h):
    if n == "front" and i in (2, 9):
        return stud(j + i) or RIM  # the rim goes on down around the beard
    if n in ("front", "right", "left", "back") and j == h - 1:
        return shade(HOOD, 0)
    return shade(HOOD, 3 if n in ("top", "front", "left") else 2)


paint_all(cowl, cowl_paint)


def robe_paint(edges, hem):
    def p(n, i, j, w, h):
        if hem and j == h - 1 and n not in ("top", "bottom"):
            return GOLD if i % 2 == 0 else GOLD_D  # the gold hem
        if n == "front":
            if i in edges:
                return GOLD  # the gold edges of the cloak
            if edges[0] < i < edges[1]:
                return shade(ROBE, 1)  # the robe under the cloak, darker
            return shade(ROBE, 3 - (1 if i % 3 == 2 else 0))
        if n == "top":
            return shade(ROBE, 2)
        if n == "bottom":
            return SHADOW
        return shade(ROBE, (3 if n == "left" else 2) - (1 if i % 3 == 2 else 0))  # folds
    return p


torso = box("body", [-4, 12, -3], [8, 12, 6])
paint_all(torso, robe_paint((1, 6), False))
skirt = box("body", [-4.5, 0, -3.5], [9, 12, 7])
paint_all(skirt, robe_paint((1, 7), True))
# runes stitched in gold on the left of the robe, as in the old picture
for (x, y) in ((0, 3), (0, 4), (0, 6), (0, 9)):
    put(skirt["front"][0] + x, skirt["front"][1] + y, GOLD_D)
for (x, y) in ((1, 2), (2, 2), (1, 3), (2, 4), (4, 5), (5, 5), (4, 6), (5, 7), (1, 8), (2, 9), (1, 10), (4, 9), (5, 10)):
    put(skirt["right"][0] + x, skirt["right"][1] + y, GOLD_D)
# the black belt with the gold buckle
belt = box("body", [-4, 11, -3], [8, 2, 6], inflate=0.3)
paint_all(belt, lambda n, i, j, w, h: LEATHER_L if j == 0 else LEATHER)
fill((belt["front"][0] + 3, belt["front"][1], 2, 2), lambda i, j, w, h: GOLD_L if (i, j) == (0, 0) else GOLD)
# the brooch that holds the cloak: a ruby in gold, on the left of the chest
brooch = box("body", [-6, 19.5, 3.3], [3, 3, 1])
paint_all(brooch, lambda n, i, j, w, h: (GOLD_D if (i in (0, 2) and j in (0, 2)) else GEM_L if (i, j) == (1, 1) else GEM)
          if n == "front" else GOLD)

# ---------------------------------------------------------------- the arms: wide sleeves, gold cuffs, the hands
def sleeve(part, x, cuff_x):
    arm = box(part, [x, 12, -2], [4, 12, 4])

    def p(n, i, j, w, h):
        if n == "bottom":
            return SKIN if (i + j) % 3 else SKIN_L  # the palm
        if n == "top":
            return shade(ROBE, 2)
        if j == h - 1:
            return SKIN_D if n == "back" else SKIN
        if j == h - 2:
            return SKIN_L if n == "front" else SKIN
        return shade(ROBE, (3 if n in ("front", "left") else 2) - (1 if i == 0 else 0))
    paint_all(arm, p)
    cuff = box(part, [cuff_x, 14, -2.5], [5, 3, 5])
    paint_all(cuff, lambda n, i, j, w, h: CLEAR if n in ("top", "bottom") and 1 <= i <= 3 and 1 <= j <= 3 else
              GOLD if j == h - 1 or n in ("top", "bottom") else GOLD_D if j == 0 else shade(ROBE, 3))


sleeve("rarm", -8, -8.5)
sleeve("larm", 4, 3.5)

TH = shelf["y"] + shelf["h"]
TH = (TH + 15) // 16 * 16
img = img.crop((0, 0, TW, TH))

model = {
    "texture": [TW, TH],
    # the pivots of the parts: the head turns about the neck, the arms about the shoulders
    "pivots": {"head": [0, 24, 0], "body": [0, 0, 0], "rarm": [-6, 22, 0], "larm": [6, 22, 0]},
    "hands": {"rarm": [-6, 11, 0], "larm": [6, 11, 0]},
    # the raised hand under the tesseract: the one on the right of the picture, as in the old Arkan
    "magicHand": "larm",
    "eyes": [[-2, 28.5, 4.1], [2, 28.5, 4.1]],
    "glows": [{"part": "body", "at": [-4.5, 21, 4.4], "radius": 1.5}],
    "boxes": BOXES,
}
for target in (ANDROID, DESKTOP):
    if target.exists():
        img.save(target / "arkan.png")
        (target / "arkan_model.json").write_text(json.dumps(model, indent=1), encoding="utf-8")
img.resize((TW * 6, TH * 6), Image.NEAREST).save(HERE / "arkan_texture_preview.png")
print(f"{len(BOXES)} boxes, texture {TW}x{TH}")
