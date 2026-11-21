#!/usr/bin/env python3
"""Arkan, Nyxora's AI helper, as a 3D mob in the style of a Minecraft mod: a model of boxes and its texture. He is a
person in a spacesuit (he was a hooded wizard before): a white helmet with a wide glass of the icon's blue and violet,
a violet stripe over its top, round caps on its sides and an antenna whose light blinks; a white suit with a control
panel on the chest (a small lit screen, buttons), a grey collar ring, a life-support pack on the back, a dark belt,
violet bands on the shoulders, grey gloves with metal wrist rings, grey boots. The hand on the right of the picture is
raised, the palm up, under Nyxora's cube.

The model (arkan_model.json) is a list of boxes in Minecraft units (y up, the feet at 0, the face towards +z), each
with its part (head, body, rarm, larm), its place and size and its texture corner (u, v) in the usual Minecraft box
layout: top and bottom on the first row, then right, front, left and back. The texture (arkan.png) is painted here
pixel by pixel and the boxes are packed on it in rows. Pixels with alpha 254 glow (the antenna light, the screen): the
views draw them at full brightness; the glowing pixels of the head go dark for a moment now and then (the views call it
a blink: it was the wizard's eyes, it is the antenna light now). "magicHand" is the hand under the cube, "eyes" are the
points of the head that shine (the antenna light), "glows" are the soft lights the views draw over the model (the
screen).

WizardView.java (phone) and MoonyWidget.cpp (PC) draw it in 3D: he floats over a turning ring, turns a little, looks
around, holds the turning cube (a tesseract, as in the icon: a cube in a cube) over his raised hand. The small pictures
of him (res/drawable-nodpi/wizard.png and launcher/resources/sculk_theme/wizard.png: chat avatars, list icons) are
stills of that render, a close-up of his head and shoulders: QT_SCALE_FACTOR=8 VELLORA_SHOTS=<dir> ./MoonyWidget closeUp
writes arkan_small.png, 512x512.

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

# the suit: a cool white, from its shadow to its light; the 3D views shade the faces themselves
SUIT = [(150, 158, 182), (184, 192, 214), (212, 218, 234), (234, 238, 248), (250, 251, 255)]
METAL = [(52, 58, 78), (80, 88, 112), (112, 120, 146), (150, 158, 184)]
DARK, DARK_L = (16, 18, 36), (30, 32, 58)
VIOLET_D, VIOLET, VIOLET_L = (75, 43, 169), (129, 84, 240), (176, 140, 255)
BLUE_D, BLUE, BLUE_L = (36, 96, 190), (72, 150, 250), (124, 208, 255)
GLINT = (228, 244, 255)
RED = (232, 54, 79)
LAMP, LAMP_CORE = (150, 110, 255, 254), (236, 226, 255, 254)   # glowing: the antenna light
SCREEN, SCREEN_L = (72, 180, 255, 254), (190, 236, 255, 254)   # glowing: the screen on the chest
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



# ---------------------------------------------------------------- the helmet: a white shell with a wide glass
helmet = box("head", [-5, 24, -5], [10, 10, 10])


def helmet_paint(n, i, j, w, h):
    if n == "front":
        if 1 <= i <= 8 and 2 <= j <= 7:
            return DARK  # behind the glass
        if (i in (0, 9) and j in (0, 9)):
            return shade(SUIT, 1)  # the corners rounded by shadow
        if i in (0, 9) or j in (1, 8):
            return METAL[2] if (1 <= i <= 8) else shade(SUIT, 2)  # the metal rim of the glass
        return shade(SUIT, 3 if j == 0 else 2)
    if n == "top":
        if i in (4, 5):
            return VIOLET if j % 4 != 3 else VIOLET_D  # the stripe over the top, front to back
        return shade(SUIT, 4 if 2 <= i <= 7 and 2 <= j <= 7 else 3)
    if n == "bottom":
        return shade(SUIT, 0)
    if n == "back":
        if i in (4, 5) and j <= 3:
            return VIOLET  # the stripe comes down the back a little
        if j == 6 and 2 <= i <= 7:
            return shade(SUIT, 0)  # a seam
        return shade(SUIT, 2 if j < 8 else 1)
    # the sides: the left of the picture in shade
    base = 2 if n == "right" else 3
    if j in (0, 9) and i in (0, 9):
        return shade(SUIT, 1)
    return shade(SUIT, base - (1 if j >= 8 else 0))


paint_all(helmet, helmet_paint)
# the glass stands out of the shell: the icon's blue at the top left to its violet at the bottom right, a glint
glass = box("head", [-4, 26, 4.6], [8, 6, 1])


def glass_paint(n, i, j, w, h):
    if n != "front":
        return DARK_L if n in ("top", "left") else DARK
    if (i, j) in ((0, 0), (7, 0), (0, 5), (7, 5)):
        return METAL[1]  # rounded corners
    t = (i + j * 1.3) / (7 + 5 * 1.3)
    deep = tuple(round((BLUE_D[k] + (VIOLET_D[k] - BLUE_D[k]) * t) * 0.62) for k in range(3))
    if (i, j) in ((1, 1), (2, 1), (1, 2)):
        return GLINT
    if (i, j) in ((3, 1), (2, 2), (1, 3)):
        return BLUE_L
    if i - j == 4 and 1 <= j <= 3:
        return tuple(min(255, c + 46) for c in deep)  # a faint second reflection
    return deep


paint_all(glass, glass_paint)
# the round caps over the ears
for x in (-6, 5):
    cap = box("head", [x, 27, -1.5], [1, 4, 3])
    outer = "right" if x < 0 else "left"
    paint_all(cap, lambda n, i, j, w, h, outer=outer: (VIOLET if (i, j) == (1, 1) or (i, j) == (1, 2) else METAL[2]) if n == outer
              else METAL[3] if n == "top" else METAL[1])
# the antenna on the right of the picture, its light at the top
mast = box("head", [3.5, 34, -1], [1, 3, 1])
paint_all(mast, lambda n, i, j, w, h: METAL[3] if n in ("front", "left", "top") else METAL[1])
light = box("head", [3.5, 37, -1], [1, 1, 1], inflate=0.25)
paint_all(light, lambda n, i, j, w, h: LAMP_CORE if n in ("front", "top") else LAMP)

# ---------------------------------------------------------------- the suit
collar = box("body", [-4, 22, -3], [8, 2, 6], inflate=0.5)
paint_all(collar, lambda n, i, j, w, h: METAL[3] if n == "top" else METAL[2] if j == 0 else METAL[1])

torso = box("body", [-4, 12, -2.5], [8, 10, 5])


def torso_paint(n, i, j, w, h):
    if n == "front":
        if j == 0:
            return shade(SUIT, 2)
        if i in (0, 7):
            return shade(SUIT, 1 if i == 0 else 3)  # the left of the picture in shade
        if j == 8 and 1 <= i <= 6:
            return shade(SUIT, 1)  # a seam over the belt
        return shade(SUIT, 3)
    if n == "back":
        return shade(SUIT, 1)
    if n == "top":
        return shade(SUIT, 3)
    if n == "bottom":
        return shade(SUIT, 0)
    return shade(SUIT, (1 if n == "right" else 3) - (1 if i == 0 else 0))


paint_all(torso, torso_paint)
# the control panel on the chest: a lit screen, three buttons
panel = box("body", [-2.5, 15, 2.5], [5, 5, 1])


def panel_paint(n, i, j, w, h):
    if n != "front":
        return METAL[2] if n in ("top", "left") else METAL[0]
    if i in (0, 4) or j in (0, 4):
        return METAL[2] if (i == 0 or j == 0) else METAL[0]  # a bevelled frame
    if j == 1:
        return SCREEN_L if i == 1 else SCREEN  # the screen
    if j == 2:
        return SCREEN if i < 3 else (16, 40, 78)
    return (BLUE, VIOLET, RED)[i - 1]  # the buttons


paint_all(panel, panel_paint)
# the life-support pack on the back, two tanks on its top
pack = box("body", [-3.5, 12.5, -5.5], [7, 9, 3])


def pack_paint(n, i, j, w, h):
    if n == "back":
        if i in (0, 6) or j in (0, 8):
            return METAL[1]
        if j in (3, 5) and 2 <= i <= 4:
            return METAL[0]  # the grille
        if (i, j) == (5, 1):
            return SCREEN
        return METAL[2]
    if n == "top":
        return METAL[3]
    if n == "bottom":
        return METAL[0]
    return METAL[2] if n == "left" else METAL[1]


paint_all(pack, pack_paint)
for x in (-3, 1):
    tank = box("body", [x, 21.5, -5], [2, 2, 2])
    paint_all(tank, lambda n, i, j, w, h: SUIT[4] if n == "top" else SUIT[2] if n in ("left", "back") else SUIT[1])
belt = box("body", [-4, 11, -2.5], [8, 2, 5], inflate=0.3)
paint_all(belt, lambda n, i, j, w, h: DARK_L if j == 0 else DARK)
fill((belt["front"][0] + 3, belt["front"][1], 2, 2), lambda i, j, w, h: VIOLET_L if (i, j) == (0, 0) else VIOLET)


# ---------------------------------------------------------------- the legs and the boots
def leg(x, lit):
    part = box("body", [x, 0, -2], [4, 11, 4])

    def p(n, i, j, w, h):
        if n == "top":
            return shade(SUIT, 1)
        if n == "bottom":
            return METAL[0]
        if j in (4, 5) and n == "front" and 1 <= i <= 2:
            return METAL[2]  # the knee pad
        if j == 7:
            return shade(SUIT, 1)  # a fold
        tone = 3 if n == "front" else 2 if n == ("left" if lit else "right") else 1
        return shade(SUIT, tone if lit or n != "front" else 2)
    paint_all(part, p)
    boot = box("body", [x, 0, -2], [4, 3, 4], inflate=0.4)
    paint_all(boot, lambda n, i, j, w, h: VIOLET if j == h - 1 and n not in ("top", "bottom") else METAL[0] if n == "bottom" else
              METAL[2] if n in ("top", "front") else METAL[1])


leg(-4, False)
leg(0, True)


# ---------------------------------------------------------------- the arms: violet shoulder bands, wrist rings, gloves
def arm(part, x, ring_x):
    limb = box(part, [x, 12, -2], [4, 12, 4])

    def p(n, i, j, w, h):
        if n == "bottom":
            return METAL[2] if (i + j) % 3 else METAL[3]  # the palm of the glove
        if n == "top":
            return shade(SUIT, 3)
        if j >= h - 3:
            return METAL[1] if n == "back" else METAL[2]  # the glove
        if j == 2:
            return VIOLET  # the band on the shoulder
        if j == 6:
            return shade(SUIT, 1)  # the fold of the elbow
        return shade(SUIT, (3 if n in ("front", "left") else 2) - (1 if i == 0 else 0))
    paint_all(limb, p)
    ring = box(part, [ring_x, 15, -2.5], [5, 1, 5])
    paint_all(ring, lambda n, i, j, w, h: CLEAR if n in ("top", "bottom") and 1 <= i <= 3 and 1 <= j <= 3 else
              METAL[3] if n in ("top", "front", "left") else METAL[1])


arm("rarm", -8, -8.5)
arm("larm", 4, 3.5)

TH = shelf["y"] + shelf["h"]
TH = (TH + 15) // 16 * 16
img = img.crop((0, 0, TW, TH))

model = {
    "texture": [TW, TH],
    # the pivots of the parts: the head turns about the neck, the arms about the shoulders
    "pivots": {"head": [0, 24, 0], "body": [0, 0, 0], "rarm": [-6, 22, 0], "larm": [6, 22, 0]},
    "hands": {"rarm": [-6, 11, 0], "larm": [6, 11, 0]},
    # the raised hand under the cube: the one on the right of the picture
    "magicHand": "larm",
    # what shines on the head and goes dark in a blink: the antenna light
    "eyes": [[4, 37.5, -0.5]],
    # the soft light over the screen on the chest
    "glows": [{"part": "body", "at": [-0.5, 18, 3.7], "radius": 1.4}],
    "boxes": BOXES,
}
for target in (ANDROID, DESKTOP):
    if target.exists():
        img.save(target / "arkan.png")
        (target / "arkan_model.json").write_text(json.dumps(model, indent=1), encoding="utf-8")
img.resize((TW * 6, TH * 6), Image.NEAREST).save(HERE / "arkan_texture_preview.png")
print(f"{len(BOXES)} boxes, texture {TW}x{TH}")
