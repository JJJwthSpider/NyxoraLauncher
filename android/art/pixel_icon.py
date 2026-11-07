#!/usr/bin/env python3
"""Lullaby Launcher's app icon: a pixel-art sleeping crescent moon with "Zz" in a night sky, like a Minecraft item.

The picture is drawn on a 36x36 pixel grid (the 108dp adaptive icon, 3dp per pixel) and scaled up without
smoothing, so the pixels stay sharp at every density. Writes the adaptive layers and the legacy icon into res/.
Run:  python3 art/pixel_icon.py   (from android/)
"""
import math
import pathlib
import random

from PIL import Image, ImageDraw

RES = pathlib.Path(__file__).resolve().parent.parent / "res"
N = 36
DENSITIES = {"mdpi": 1, "hdpi": 1.5, "xhdpi": 2, "xxhdpi": 3, "xxxhdpi": 4}

SKY_TOP, SKY_BOTTOM = (16, 12, 38), (52, 30, 92)
OUTLINE = (40, 24, 82, 255)
MOON_LIGHT = (250, 246, 255, 255)
MOON = (220, 207, 255, 255)
MOON_SHADE = (168, 146, 232, 255)
CRATER = (190, 172, 245, 255)
EYE = (60, 40, 110, 255)
BLUSH = (255, 150, 190, 255)
ZZ = (255, 255, 255, 255)
STAR = (255, 236, 150, 255)


def lerp(a, b, t):
    return tuple(round(x + (y - x) * t) for x, y in zip(a, b))


def background():
    img = Image.new("RGB", (N, N))
    px = img.load()
    for y in range(N):
        band = (y // 3) / ((N - 1) // 3)  # steps of 3 pixels: a blocky gradient
        for x in range(N):
            px[x, y] = lerp(SKY_TOP, SKY_BOTTOM, band)
    rnd = random.Random(7)
    for _ in range(14):
        x, y = rnd.randrange(N), rnd.randrange(N)
        px[x, y] = lerp(px[x, y], (255, 255, 255), rnd.choice((0.35, 0.55, 0.8)))
    # a few bigger "+" stars near the edges (outside the moon)
    for cx, cy in ((5, 6), (30, 29), (6, 28)):
        for dx, dy in ((0, 0), (1, 0), (-1, 0), (0, 1), (0, -1)):
            px[cx + dx, cy + dy] = (255, 255, 255) if (dx, dy) == (0, 0) else (170, 160, 230)
    return img


def foreground():
    img = Image.new("RGBA", (N, N), (0, 0, 0, 0))
    px = img.load()
    cx, cy, r = 16.0, 19.0, 11.0       # the moon
    bx, by, br = 22.5, 14.0, 8.2       # the bite that makes the crescent
    inside = set()
    for y in range(N):
        for x in range(N):
            px_c, py_c = x + 0.5, y + 0.5
            if math.hypot(px_c - cx, py_c - cy) <= r and math.hypot(px_c - bx, py_c - by) > br:
                inside.add((x, y))
    for x, y in inside:
        d_out = r - math.hypot(x + 0.5 - cx, y + 0.5 - cy)
        d_in = math.hypot(x + 0.5 - bx, y + 0.5 - by) - br
        light = (x + y) < 28
        color = MOON_SHADE if d_in < 1.4 else (MOON_LIGHT if light and d_out < 2.2 else MOON)
        px[x, y] = color
    for x, y in inside:  # the dark outline around it
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            n = (x + dx, y + dy)
            if n not in inside and 0 <= n[0] < N and 0 <= n[1] < N:
                px[n] = OUTLINE
    for x, y in ((8, 15), (13, 27), (19, 26)):  # craters
        if (x, y) in inside:
            px[x, y] = CRATER
    # the sleeping face: a closed eye and a blush
    for x, y in ((9, 20), (10, 21), (11, 21), (12, 20)):
        px[x, y] = EYE
    for x, y in ((8, 23), (9, 23)):
        px[x, y] = BLUSH
    for x, y in ((12, 24), (13, 25), (14, 24)):  # a tiny smile
        px[x, y] = EYE
    # "Zz" above the moon's horn
    big_z = ["#####", "...#.", "..#..", ".#...", "#####"]
    small_z = ["####", "..#.", ".#..", "####"]
    for pattern, ox, oy in ((small_z, 19, 12), (big_z, 23, 5)):
        for j, row in enumerate(pattern):
            for i, c in enumerate(row):
                if c == "#":
                    px[ox + i, oy + j] = ZZ
    # a twinkle
    for dx, dy in ((0, 0), (1, 0), (-1, 0), (0, 1), (0, -1)):
        px[28 + dx, 22 + dy] = STAR
    return img


PAD = 6  # the adaptive layers: the picture in the middle of a 48 grid, so phone masks (circle, squircle) keep it whole
M = N + 2 * PAD


def adaptive_background():
    img = Image.new("RGB", (M, M))
    px = img.load()
    for y in range(M):
        band = (y // 3) / ((M - 1) // 3)
        for x in range(M):
            px[x, y] = lerp(SKY_TOP, SKY_BOTTOM, band)
    img.paste(background().crop((0, 0, N, N)), (PAD, PAD))
    rnd = random.Random(11)
    for _ in range(10):  # stars on the outer edge too
        x, y = rnd.randrange(M), rnd.randrange(M)
        if not (PAD <= x < PAD + N and PAD <= y < PAD + N):
            px[x, y] = lerp(px[x, y], (255, 255, 255), 0.5)
    return img


def adaptive_foreground():
    img = Image.new("RGBA", (M, M), (0, 0, 0, 0))
    img.paste(foreground(), (PAD, PAD))
    return img


def scaled(img, size):
    return img.resize((size, size), Image.NEAREST)


def main():
    bg, fg = adaptive_background(), adaptive_foreground()
    legacy = background().convert("RGBA")
    legacy.alpha_composite(foreground())
    for name, k in DENSITIES.items():
        folder = RES / f"mipmap-{name}"
        folder.mkdir(parents=True, exist_ok=True)
        size = round(108 * k)
        # sharp pixels: scale by a whole number first, then to the exact size
        big = max(1, math.ceil(size / M))
        scaled(scaled(bg, M * big), size).save(folder / "ic_launcher_background.png")
        scaled(scaled(fg, M * big), size).save(folder / "ic_launcher_foreground.png")
        # the legacy icon (Android 7 and older): the whole picture at 48dp
        scaled(scaled(legacy, N * max(1, math.ceil(48 * k / N))), round(48 * k)).save(folder / "ic_launcher.png")
    # a preview of how a phone shows it: the adaptive layers in a circle (the 72dp visible part)
    full = bg.convert("RGBA")
    full.alpha_composite(fg)
    view = scaled(full, M * 12)
    mask = Image.new("L", view.size, 0)
    cut = view.size[0] * 18 // 108
    ImageDraw.Draw(mask).ellipse((cut, cut, view.size[0] - cut, view.size[0] - cut), fill=255)
    preview = Image.new("RGBA", view.size, (255, 255, 255, 255))
    preview.paste(view, (0, 0), mask)
    preview.save(pathlib.Path(__file__).resolve().parent / "pixel_icon_preview.png")


if __name__ == "__main__":
    main()
