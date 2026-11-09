#!/usr/bin/env python3
"""Arkan, Vellora's AI wizard, as a 3D mob in the style of a Minecraft mod: a model of boxes and its texture.

The model (arkan_model.json) is a list of boxes in Minecraft units (y up, the feet at 0, the face towards +z), each
with its part (head, body, rarm, larm, staff), its place and size and its texture corner (u, v) in the usual Minecraft
box layout: top and bottom on the first row, then right, front, left and back. The texture (arkan.png, 128x64) is
painted here pixel by pixel. Pixels with alpha 254 glow (the eyes, the ruby of the staff and the belt): the views draw
them at full brightness.

WizardView.java (phone) and MoonyWidget.cpp (PC) draw it in 3D: he floats over a magic circle, turns a little, looks
around, holds the red tesseract over his right hand and a staff in the left. The small pictures of him
(res/drawable-nodpi/wizard.png and launcher/resources/sculk_theme/wizard.png: chat avatars, list icons) are stills of
that render, a close-up of his head and shoulders.

Run:  python3 art/arkan3d.py   (from android/)
"""
import json
import pathlib
import random

from PIL import Image

HERE = pathlib.Path(__file__).resolve().parent
ANDROID = HERE.parent / "assets"
DESKTOP = HERE.parent.parent / "launcher" / "resources" / "sculk_theme"
TW, TH = 128, 64

# the palette: burgundy cloth, gold trim, a pale face in the shadow of the hood, a silver beard, dark wood
CLOTH = [(40, 6, 14), (54, 8, 19), (68, 11, 24), (84, 14, 30), (104, 18, 36)]
GOLD_D, GOLD, GOLD_L = (120, 82, 30), (192, 142, 56), (240, 202, 112)
SKIN_D, SKIN, SKIN_L = (150, 98, 80), (190, 132, 108), (214, 160, 134)
BEARD = [(120, 116, 124), (170, 166, 172), (210, 206, 210), (238, 234, 234)]
WOOD_D, WOOD, WOOD_L = (58, 34, 20), (86, 52, 30), (112, 72, 42)
SHADOW = (14, 3, 7)
EYE, EYE_CORE = (255, 54, 70, 254), (255, 214, 218, 254)
GEM_D, GEM, GEM_L = (150, 10, 30, 254), (232, 34, 56, 254), (255, 150, 160, 254)

rnd = random.Random(7)
img = Image.new("RGBA", (TW, TH), (0, 0, 0, 0))
px = img.load()

BOXES = []  # the model


def faces(u, v, w, h, d):
    """the rectangles of a box's texture: name -> (x, y, width, height)"""
    return {"top": (u + d, v, w, d), "bottom": (u + d + w, v, w, d), "right": (u, v + d, d, h), "front": (u + d, v + d, w, h),
            "left": (u + d + w, v + d, d, h), "back": (u + 2 * d + w, v + d, w, h)}


def put(x, y, c):
    if 0 <= x < TW and 0 <= y < TH:
        px[x, y] = c if len(c) == 4 else c + (255,)


def cloth(x, y, shade=2):
    """burgundy cloth with a little grain"""
    return CLOTH[max(0, min(4, shade + rnd.choice((-1, 0, 0, 0, 1))))]


def box(part, frm, size, uv, inflate=0.0):
    BOXES.append({"part": part, "from": frm, "size": size, "uv": uv, "inflate": inflate})
    return faces(uv[0], uv[1], int(size[0]), int(size[1]), int(size[2]))


def fill(rect, painter):
    x0, y0, w, h = rect
    for j in range(h):
        for i in range(w):
            c = painter(i, j, w, h)
            if c is not None:
                put(x0 + i, y0 + j, c)


def robe_face(shade=2, hem=False, folds=True):
    def p(i, j, w, h):
        s = shade - (1 if j > h * 0.7 else 0)
        if folds and i % 3 == 2:
            s -= 1
        if hem and j == h - 1:
            return GOLD if i % 2 == 0 else GOLD_D
        return cloth(i, j, s)
    return p


# ---------------------------------------------------------------- the head: the face in the shadow of the hood
head = box("head", [-4, 24, -4], [8, 8, 8], [0, 0])
fill(head["front"], lambda i, j, w, h: SHADOW if j < 2 else SKIN_D if j < 3 or i in (0, 7) else SKIN)
for x, y, c in ((1, 3, EYE_CORE), (2, 3, EYE), (5, 3, EYE), (6, 3, EYE_CORE)):  # glowing eyes
    put(head["front"][0] + x, head["front"][1] + y, c)
for x in (1, 2, 5, 6):  # bushy grey brows
    put(head["front"][0] + x, head["front"][1] + 2, BEARD[1])
for name in ("right", "left", "back", "top", "bottom"):
    fill(head[name], lambda i, j, w, h: SHADOW)

# ---------------------------------------------------------------- the hood: an opening for the face with a gold-stitched rim
hood = box("head", [-4, 24, -4], [8, 9, 8], [32, 0], inflate=0.6)


def hood_front(i, j, w, h):
    if 1 <= i <= 6 and 2 <= j <= 8:
        return (0, 0, 0, 0)  # the face shows through
    if (i in (0, 7) and j >= 2) or j == 1:
        return GOLD if (i + j) % 2 == 0 else GOLD_D
    return cloth(i, j, 3)


fill(hood["front"], hood_front)
fill(hood["top"], lambda i, j, w, h: cloth(i, j, 3 if j < 4 else 2))
fill(hood["back"], lambda i, j, w, h: cloth(i, j, 1 if i % 3 == 0 else 2))
fill(hood["right"], lambda i, j, w, h: GOLD_D if i == w - 1 and j > 1 else cloth(i, j, 2 if j % 4 else 1))
fill(hood["left"], lambda i, j, w, h: GOLD_D if i == 0 and j > 1 else cloth(i, j, 3 if j % 4 else 2))
fill(hood["bottom"], lambda i, j, w, h: (0, 0, 0, 0) if 1 <= i <= 6 and j >= 4 else cloth(i, j, 1))
# the pointed top of the hood, falling back in two steps
tip1 = box("head", [-2.5, 32.4, -3.5], [5, 3, 5], [64, 0])
for r in tip1.values():
    fill(r, lambda i, j, w, h: cloth(i, j, 2 if j else 3))
tip2 = box("head", [-1, 35, -5], [2, 2, 3], [84, 0])
for r in tip2.values():
    fill(r, lambda i, j, w, h: GOLD if (i + j) % 3 == 0 else cloth(i, j, 2))
# the nose
nose = box("head", [-1, 25, 4], [2, 3, 1], [96, 0])
for r in nose.values():
    fill(r, lambda i, j, w, h: SKIN_L if j == 0 else SKIN if j == 1 else SKIN_D)
# the moustache and the long silver beard
moustache = box("head", [-3.5, 24, 4], [7, 2, 1], [104, 0])
for r in moustache.values():
    fill(r, lambda i, j, w, h: BEARD[3] if j == 0 and 1 <= i <= w - 2 else BEARD[2])
beard = box("head", [-3, 15, 3.6], [6, 9, 1], [64, 8])
fill(beard["front"], lambda i, j, w, h: (0, 0, 0, 0) if j >= h - 2 and i in (0, w - 1) else
     (0, 0, 0, 0) if j == h - 1 and i in (1, w - 2) else BEARD[3 if i in (2, 3) and j < 5 else 2 if (i + j) % 3 else 1])
for name in ("right", "left", "top", "bottom", "back"):
    fill(beard[name], lambda i, j, w, h: BEARD[1])

# ---------------------------------------------------------------- the body: the robe down to the ground, a mantle, a belt
torso = box("body", [-4, 12, -3], [8, 12, 6], [0, 18])
fill(torso["front"], lambda i, j, w, h: GOLD if i in (3, 4) and j % 3 == 1 else robe_face(3, folds=False)(i, j, w, h))
fill(torso["back"], robe_face(2))
fill(torso["right"], robe_face(2))
fill(torso["left"], robe_face(3))
fill(torso["top"], lambda i, j, w, h: cloth(i, j, 2))
fill(torso["bottom"], lambda i, j, w, h: cloth(i, j, 1))
mantle = box("body", [-5, 21, -3.5], [10, 3, 7], [28, 18])
for name, r in mantle.items():
    fill(r, lambda i, j, w, h, name=name: GOLD if (name not in ("top", "bottom") and j == h - 1 and i % 2 == 0) else
         GOLD_D if (name not in ("top", "bottom") and j == h - 1) else cloth(i, j, 1 if name == "back" else 2))
skirt = box("body", [-4.5, 0, -3.5], [9, 12, 7], [0, 36])
fill(skirt["front"], lambda i, j, w, h: GOLD if i in (3, 5) else (40, 6, 14) if i == 4 else robe_face(3, hem=True)(i, j, w, h))
fill(skirt["back"], robe_face(2, hem=True))
fill(skirt["right"], robe_face(2, hem=True))
fill(skirt["left"], robe_face(3, hem=True))
fill(skirt["top"], lambda i, j, w, h: cloth(i, j, 2))
fill(skirt["bottom"], lambda i, j, w, h: SHADOW)
# runes stitched on the skirt, gold
for (x, y) in ((1, 4), (1, 5), (2, 6), (7, 3), (6, 4), (7, 5), (1, 9), (2, 8), (7, 8), (6, 9)):
    put(skirt["front"][0] + x, skirt["front"][1] + y, GOLD_D)
belt = box("body", [-4, 11, -3], [8, 2, 6], [32, 28], inflate=0.3)
for name, r in belt.items():
    fill(r, lambda i, j, w, h: GOLD_D if j == 0 else WOOD)
fill((belt["front"][0] + 3, belt["front"][1], 2, 2), lambda i, j, w, h: GEM_L if (i, j) == (0, 0) else GEM)

# ---------------------------------------------------------------- the arms: wide sleeves, gold cuffs, the hands
def sleeve(part, frm, uv, cuff_uv, cuff_x):
    arm = box(part, frm, [4, 12, 4], uv)
    for name, r in arm.items():
        if name == "bottom":
            fill(r, lambda i, j, w, h: SKIN)  # the palm
        elif name == "top":
            fill(r, lambda i, j, w, h: cloth(i, j, 2))
        else:
            fill(r, lambda i, j, w, h, name=name: SKIN_L if j == h - 2 else SKIN if j == h - 1 else
                 cloth(i, j, 3 if name in ("front", "left") else 2))
    cuff = box(part, [cuff_x, 14, frm[2] - 0.5], [5, 3, 5], cuff_uv)
    for name, r in cuff.items():
        fill(r, lambda i, j, w, h, name=name: (0, 0, 0, 0) if name in ("top", "bottom") and 1 <= i <= 3 and 1 <= j <= 3 else
             GOLD if j == h - 1 or name in ("top", "bottom") else cloth(i, j, 3))


sleeve("rarm", [-8, 12, -2], [64, 20], [80, 20], -8.5)
sleeve("larm", [4, 12, -2], [64, 40], [80, 40], 3.5)

# ---------------------------------------------------------------- the staff: dark wood, a gold cap, a glowing ruby
staff = box("staff", [-1, -2, -1], [2, 32, 2], [104, 18])
for name, r in staff.items():
    fill(r, lambda i, j, w, h: GOLD_D if (j + i) % 7 == 0 else WOOD_L if i == 0 else WOOD if (j // 2) % 2 else WOOD_D)
cap = box("staff", [-2, 30, -2], [4, 1, 4], [112, 18])
for r in cap.values():
    fill(r, lambda i, j, w, h: GOLD_L if (i + j) % 2 else GOLD)
gem = box("staff", [-1.5, 31, -1.5], [3, 3, 3], [112, 24])
for r in gem.values():
    fill(r, lambda i, j, w, h: GEM_L if (i, j) == (0, 0) else GEM_D if j == h - 1 else GEM)

model = {
    "texture": [TW, TH],
    # the pivots of the parts (the head turns about the neck, the arms about the shoulders); the staff is placed in the
    # left hand by the views
    "pivots": {"head": [0, 24, 0], "body": [0, 0, 0], "rarm": [-6, 22, 0], "larm": [6, 22, 0], "staff": [0, 0, 0]},
    "hands": {"rarm": [-6, 11, 0], "larm": [6, 11, 0]},
    "eyes": [[-2.5, 28.5, 4.1], [2.5, 28.5, 4.1]],
    "boxes": BOXES,
}
for target in (ANDROID, DESKTOP):
    if target.exists():
        img.save(target / "arkan.png")
        (target / "arkan_model.json").write_text(json.dumps(model, indent=1), encoding="utf-8")
img.resize((TW * 6, TH * 6), Image.NEAREST).save(HERE / "arkan_texture_preview.png")
print(f"{len(BOXES)} boxes")
