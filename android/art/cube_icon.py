#!/usr/bin/env python3
"""Lullaby Launcher's app icon: a Minecraft-like grass block at night, with a glowing crescent moon and stars.

The block is drawn in isometric view with pixel textures on its faces (an 8x8 grid like a game block), on a deep
purple night sky. Everything is drawn 2 times bigger and scaled down, so the edges are smooth. Writes the adaptive
layers (background, foreground) and the legacy icon for every density into res/, and art/cube_icon_preview.png.
Run:  python3 art/cube_icon.py   (from android/)
"""
import math
import pathlib
import random

from PIL import Image, ImageDraw, ImageFilter

HERE = pathlib.Path(__file__).resolve().parent
RES = HERE.parent / "res"
S = 1080 * 2          # the drawing size of the 108dp adaptive icon
K = S / 1080          # 1 unit of the layout below = 1/10 dp
DENSITIES = {"mdpi": 1, "hdpi": 1.5, "xhdpi": 2, "xxhdpi": 3, "xxxhdpi": 4}

GRASS = [(86, 200, 120), (74, 182, 106), (98, 214, 132), (66, 168, 98), (110, 222, 140)]
DIRT = [(134, 92, 62), (120, 82, 54), (148, 104, 70), (108, 72, 48), (126, 88, 60)]
SKY_INNER, SKY_OUTER = (74, 48, 140), (14, 10, 34)
MOON, MOON_GLOW = (246, 240, 255), (200, 180, 255)


def u(v):
    return v * K


def background():
    small = 270  # the gradient is smooth, so it is computed small and scaled up
    img = Image.new("RGB", (small, small))
    px = img.load()
    cx, cy = small * 0.5, small * 0.42
    far = math.hypot(small * 0.5, small * 0.6)
    for y in range(small):
        for x in range(small):
            t = min(1.0, math.hypot(x - cx, y - cy) / far)
            px[x, y] = tuple(round(a + (b - a) * t ** 0.8) for a, b in zip(SKY_INNER, SKY_OUTER))
    img = img.resize((S, S), Image.BICUBIC)
    draw = ImageDraw.Draw(img, "RGBA")
    rnd = random.Random(5)
    for _ in range(40):
        x, y, r = rnd.uniform(0, S), rnd.uniform(0, S * 0.75), u(rnd.choice((2.5, 3.5, 5)))
        draw.ellipse((x - r, y - r, x + r, y + r), fill=(255, 255, 255, round(255 * rnd.uniform(0.45, 1))))
    return img


def sparkle(draw, x, y, r, color):
    w = r * 0.22
    draw.polygon([(x, y - r), (x + w, y - w), (x + r, y), (x + w, y + w), (x, y + r), (x - w, y + w), (x - r, y), (x - w, y - w)], fill=color)


def face(draw, origin, du, dv, palette, rnd, shade, grass_rows=0):
    """an 8x8 pixel-textured face: the cell (i, j) starts at origin + du*i + dv*j"""
    def p(a, b):
        return (origin[0] + du[0] * a + dv[0] * b, origin[1] + du[1] * a + dv[1] * b)

    for j in range(8):
        for i in range(8):
            if grass_rows and (j < grass_rows or (j == grass_rows and rnd.random() < 0.45)):
                color = rnd.choice(GRASS)
            else:
                color = rnd.choice(palette)
            color = tuple(round(c * shade) for c in color)
            draw.polygon([p(i, j), p(i + 1, j), p(i + 1, j + 1), p(i, j + 1)], fill=color, outline=color)


def foreground():
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))

    # the moon's glow, then the moon (a circle with a bite), upper right
    mx, my, mr = u(690), u(330), u(92)
    glow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    ImageDraw.Draw(glow).ellipse((mx - mr * 1.7, my - mr * 1.7, mx + mr * 1.7, my + mr * 1.7), fill=MOON_GLOW + (110,))
    img.alpha_composite(glow.filter(ImageFilter.GaussianBlur(u(38))))
    moon = Image.new("L", (S, S), 0)
    md = ImageDraw.Draw(moon)
    md.ellipse((mx - mr, my - mr, mx + mr, my + mr), fill=255)
    md.ellipse((mx - mr * 0.35, my - mr * 1.15, mx + mr * 1.45, my + mr * 0.6), fill=0)
    img.paste(Image.new("RGBA", (S, S), MOON + (255,)), (0, 0), moon)

    # the grass block and its soft shadow
    cx, cy, s = u(500), u(640), u(215)
    c30 = math.cos(math.radians(30))
    top, right, center, left = (cx, cy - s), (cx + s * c30, cy - s / 2), (cx, cy), (cx - s * c30, cy - s / 2)
    shadow = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    ImageDraw.Draw(shadow).ellipse((cx - s * 1.05, cy + s * 0.75, cx + s * 1.05, cy + s * 1.2), fill=(0, 0, 0, 120))
    img.alpha_composite(shadow.filter(ImageFilter.GaussianBlur(u(18))))
    draw = ImageDraw.Draw(img)
    rnd = random.Random(3)
    eighth = lambda a, b: ((b[0] - a[0]) / 8, (b[1] - a[1]) / 8)
    face(draw, left, eighth(left, top), eighth(left, center), GRASS, rnd, 1.0)
    down = (0, s / 8)
    face(draw, left, eighth(left, center), down, DIRT, rnd, 0.82, grass_rows=2)
    face(draw, center, eighth(center, right), down, DIRT, rnd, 0.62, grass_rows=2)
    # moonlight on the top edges
    draw.line([left, top, right], fill=(235, 228, 255), width=round(u(5)))
    draw.line([left, center, right], fill=(170, 230, 180), width=round(u(3)))
    draw.line([center, (cx, cy + s)], fill=(60, 40, 30), width=round(u(3)))

    sparkle(draw, u(300), u(300), u(26), (255, 236, 160))
    sparkle(draw, u(790), u(520), u(16), (255, 255, 255))
    return img


def main():
    bg, fg = background(), foreground()
    full = bg.convert("RGBA")
    full.alpha_composite(fg)
    for name, k in DENSITIES.items():
        folder = RES / f"mipmap-{name}"
        size = round(108 * k)
        bg.resize((size, size), Image.LANCZOS).save(folder / "ic_launcher_background.png")
        fg.resize((size, size), Image.LANCZOS).save(folder / "ic_launcher_foreground.png")
        # the legacy icon (Android 7 and older): the visible middle (72 of 108dp) in a rounded square
        cut = S * 18 // 108
        legacy = full.crop((cut, cut, S - cut, S - cut)).resize((round(48 * k),) * 2, Image.LANCZOS)
        mask = Image.new("L", (legacy.size[0] * 4,) * 2, 0)
        ImageDraw.Draw(mask).rounded_rectangle((0, 0, mask.size[0] - 1, mask.size[1] - 1), radius=mask.size[0] // 5, fill=255)
        legacy.putalpha(mask.resize(legacy.size, Image.LANCZOS))
        legacy.save(folder / "ic_launcher.png")
    # a preview of how a phone shows it (the 72dp visible part in a circle)
    view = full.resize((576, 576), Image.LANCZOS)
    mask = Image.new("L", (576 * 4,) * 2, 0)
    ImageDraw.Draw(mask).ellipse((576 * 4 // 6, 576 * 4 // 6, 576 * 4 * 5 // 6, 576 * 4 * 5 // 6), fill=255)
    preview = Image.new("RGBA", (576, 576), (255, 255, 255, 255))
    preview.paste(view, (0, 0), mask.resize((576, 576), Image.LANCZOS))
    preview.save(HERE / "cube_icon_preview.png")


if __name__ == "__main__":
    main()
