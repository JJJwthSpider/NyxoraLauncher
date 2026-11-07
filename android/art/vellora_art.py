#!/usr/bin/env python3
"""Vellora Client's pictures. The app icon is the user's picture (art/vellora_icons_source.png, object 1: a dark cube
with a crimson glow and an orbit with a star in a glowing frame); its second object is the tile "Готовые сборки".
Writes the adaptive icon layers and the legacy icon, the logo used inside the app (res/drawable-nodpi/install_default.png),
the tiles "Новая установка" (tile_new.png, drawn here) and "Готовые сборки" (tile_packs.png), and a preview
art/vellora_preview.png. (sky() and crystal_v() are the earlier drawn "V" icon, kept for reference.)
Run:  python3 art/vellora_art.py   (from android/)
"""
import math
import pathlib
import random

from PIL import Image, ImageDraw, ImageFilter

HERE = pathlib.Path(__file__).resolve().parent
RES = HERE.parent / "res"
S = 2160
DENSITIES = {"mdpi": 1, "hdpi": 1.5, "xhdpi": 2, "xxhdpi": 3, "xxxhdpi": 4}

DEEP, MID, LIGHT = (12, 3, 6), (42, 8, 16), (78, 14, 28)
CRIMSON, CRIMSON_LIGHT, CRIMSON_DARK = (220, 30, 58), (255, 96, 116), (120, 10, 28)


def u(v):
    return v * S / 1080


def sky(size=S, seed=4, drops=70):
    small = 216
    img = Image.new("RGB", (small, small))
    px = img.load()
    for y in range(small):
        for x in range(small):
            t = min(1, math.hypot(x - small * 0.5, y - small * 0.62) / (small * 0.75))
            px[x, y] = tuple(round(a + (b - a) * t) for a, b in zip(LIGHT, DEEP))
    img = img.resize((size, size), Image.BICUBIC).convert("RGBA")
    rain = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(rain)
    rnd = random.Random(seed)
    k = size / 1080
    for _ in range(drops):
        x, y, n = rnd.uniform(-100, 1080) * k, rnd.uniform(-60, 1080) * k, rnd.uniform(40, 110) * k
        a = rnd.randint(60, 170)
        d.line([(x, y), (x + n * 0.18, y + n)], fill=(255, 60, 80, a), width=max(1, round(rnd.choice((3, 4, 6)) * k)))
    img.alpha_composite(rain)
    return img


def crystal_v(size=S, scale=1.0, cx=None, cy=None):
    """the crimson crystal V with its glow, on a transparent layer"""
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    k = size / 1080 * scale
    cx = size / 2 if cx is None else cx
    cy = size / 2 if cy is None else cy
    def p(x, y):
        return (cx + x * k, cy + y * k)
    left = [p(-250, -200), p(-120, -200), p(0, 120), p(0, 250)]
    right = [p(250, -200), p(120, -200), p(0, 120), p(0, 250)]
    glow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    gd = ImageDraw.Draw(glow)
    gd.polygon(left + right[::-1], fill=CRIMSON + (200,))
    img.alpha_composite(glow.filter(ImageFilter.GaussianBlur(60 * k)))
    d = ImageDraw.Draw(img)
    # each arm in two facets: lit outer half, darker inner half
    d.polygon([p(-250, -200), p(-185, -200), p(-60, 185), p(0, 250)], fill=CRIMSON_LIGHT)
    d.polygon([p(-185, -200), p(-120, -200), p(0, 120), p(0, 250), p(-60, 185)], fill=CRIMSON)
    d.polygon([p(250, -200), p(185, -200), p(60, 185), p(0, 250)], fill=CRIMSON_DARK)
    d.polygon([p(185, -200), p(120, -200), p(0, 120), p(0, 250), p(60, 185)], fill=(176, 18, 44))
    # tops of the arms cut like gems
    d.polygon([p(-250, -200), p(-120, -200), p(-150, -250), p(-220, -250)], fill=(255, 150, 162))
    d.polygon([p(250, -200), p(120, -200), p(150, -250), p(220, -250)], fill=(200, 40, 66))
    for poly in ([p(-250, -200), p(-220, -250), p(-150, -250), p(-120, -200), p(0, 120), p(120, -200), p(150, -250), p(220, -250), p(250, -200), p(0, 250)],):
        d.line(poly + [poly[0]], fill=(255, 190, 198), width=max(1, round(5 * k)), joint="curve")
    # a sparkle on the left top
    sx, sy, r = cx - 200 * k, cy - 300 * k, 34 * k
    w = r * 0.2
    d.polygon([(sx, sy - r), (sx + w, sy - w), (sx + r, sy), (sx + w, sy + w), (sx, sy + r), (sx - w, sy + w), (sx - r, sy), (sx - w, sy - w)], fill=(255, 220, 225))
    return img


def rounded(img, radius_part=0.22):
    mask = Image.new("L", img.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, img.size[0] - 1, img.size[1] - 1), radius=int(img.size[0] * radius_part), fill=255)
    out = img.copy()
    out.putalpha(mask)
    return out


def tile(symbol):
    size = 768
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle((24, 24, size - 24, size - 24), radius=150, fill=(30, 7, 13, 255), outline=(150, 34, 52, 255), width=18)
    if symbol == "plus":
        d.rounded_rectangle((size / 2 - 52, 190, size / 2 + 52, size - 190), radius=26, fill=CRIMSON_LIGHT)
        d.rounded_rectangle((190, size / 2 - 52, size - 190, size / 2 + 52), radius=26, fill=CRIMSON_LIGHT)
    else:  # a stack of three packs (boxes)
        for i, (y, c) in enumerate(((470, CRIMSON_DARK), (360, CRIMSON), (250, CRIMSON_LIGHT))):
            d.rounded_rectangle((180 + i * 10, y, size - 180 - i * 10, y + 120), radius=28, fill=c, outline=(255, 190, 198), width=8)
            d.rectangle((size / 2 - 40, y + 30, size / 2 + 40, y + 50), fill=(255, 220, 225))
    return img


SOURCE = HERE / "vellora_icons_source.png"
ICON_BOX = (205, 150, 790, 735)    # object 1 with its frame, in the source picture
INNER_BOX = (232, 177, 763, 708)   # object 1 inside the frame line
PACKS_BOX = (977, 150, 1564, 737)  # object 2 with its frame


def framed(box, size):
    """an object of the source with its frame, the outside of the rounded square see-through"""
    img = Image.open(SOURCE).convert("RGBA").crop(box).resize((size, size), Image.LANCZOS)
    mask = Image.new("L", (size * 4, size * 4), 0)
    ImageDraw.Draw(mask).rounded_rectangle((6, 6, size * 4 - 7, size * 4 - 7), radius=int(size * 4 * 0.205), fill=255)
    img.putalpha(mask.resize((size, size), Image.LANCZOS))
    return img


def adaptive_layers(size):
    """the background: the dark crimson inside of the frame; the foreground: the cube with its orbit, feathered at the
    edges and small enough that every phone mask (circle, squircle) keeps it whole"""
    src = Image.open(SOURCE).convert("RGB")
    inner = src.crop(INNER_BOX)
    small = inner.resize((8, 8), Image.BOX)
    corner = small.getpixel((0, 7))
    bg = Image.new("RGB", (size, size), corner)
    glow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    ImageDraw.Draw(glow).ellipse((size * 0.18, size * 0.18, size * 0.82, size * 0.82), fill=(120, 14, 38, 110))
    bg = bg.convert("RGBA")
    bg.alpha_composite(glow.filter(ImageFilter.GaussianBlur(size * 0.08)))
    part = int(size * 68 / 108)
    fg_img = inner.resize((part, part), Image.LANCZOS).convert("RGBA")
    feather = Image.new("L", (part, part), 0)
    ImageDraw.Draw(feather).ellipse((part * 0.03, part * 0.03, part * 0.97, part * 0.97), fill=255)
    fg_img.putalpha(feather.filter(ImageFilter.GaussianBlur(part * 0.05)))
    fg = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    fg.alpha_composite(fg_img, ((size - part) // 2, (size - part) // 2))
    return bg.convert("RGB"), fg


def main():
    for name, k in DENSITIES.items():
        folder = RES / f"mipmap-{name}"
        size = round(108 * k)
        bg, fg = adaptive_layers(size * 2)
        bg.resize((size, size), Image.LANCZOS).save(folder / "ic_launcher_background.png")
        fg.resize((size, size), Image.LANCZOS).save(folder / "ic_launcher_foreground.png")
        framed(ICON_BOX, round(48 * k)).save(folder / "ic_launcher.png")
    nodpi = RES / "drawable-nodpi"
    framed(ICON_BOX, 384).save(nodpi / "install_default.png")
    tile("plus").save(nodpi / "tile_new.png")
    framed(PACKS_BOX, 384).save(nodpi / "tile_packs.png")
    # the preview: the adaptive icon in a circle, the logo and the tiles
    bg, fg = adaptive_layers(540)
    full = bg.convert("RGBA")
    full.alpha_composite(fg)
    prev = Image.new("RGBA", (4 * 300, 300), (40, 40, 40, 255))
    view = full.resize((300, 300), Image.LANCZOS)
    mask = Image.new("L", (300, 300), 0)
    ImageDraw.Draw(mask).ellipse((50, 50, 250, 250), fill=255)
    prev.paste(view, (0, 0), mask)
    for i, name in enumerate(("install_default.png", "tile_new.png", "tile_packs.png")):
        im = Image.open(nodpi / name).convert("RGBA").resize((260, 260), Image.LANCZOS)
        prev.alpha_composite(im, (300 * (i + 1) + 20, 20))
    prev.save(HERE / "vellora_preview.png")


if __name__ == "__main__":
    main()
