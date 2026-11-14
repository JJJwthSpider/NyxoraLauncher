#!/usr/bin/env python3
"""Nyxora Launcher's pictures. The app icon is the picture the user sent (art/nyxora_icon_user.png: a glass cube inside a
hexagon with an orbit, blue and violet, in a glowing rounded frame). The tile "Готовые сборки" is object 2 of the earlier
picture (art/vellora_icons_source.png).
Writes the adaptive icon layers and the legacy icon, the logo used inside the app (res/drawable-nodpi/install_default.png),
the tiles "Новая установка" (tile_new.png, drawn here) and "Готовые сборки" (tile_packs.png and the desktop's
launcher/resources/sculk_theme/tile-packs-vellora.png: the earlier picture was crimson, its hues are turned to the icon's
violet), and a preview art/nyxora_preview.png.
Run:  python3 art/nyxora_art.py   (from android/)
"""
import pathlib

from PIL import Image, ImageDraw, ImageFilter

HERE = pathlib.Path(__file__).resolve().parent
RES = HERE.parent / "res"
S = 2160
DENSITIES = {"mdpi": 1, "hdpi": 1.5, "xhdpi": 2, "xxhdpi": 3, "xxxhdpi": 4}

ACCENT, ACCENT_LIGHT, ACCENT_DARK = (111, 62, 232), (161, 120, 255), (57, 24, 147)


def u(v):
    return v * S / 1080


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
    d.rounded_rectangle((24, 24, size - 24, size - 24), radius=150, fill=(9, 7, 30, 255), outline=(97, 63, 185, 255), width=18)
    if symbol == "plus":
        d.rounded_rectangle((size / 2 - 52, 190, size / 2 + 52, size - 190), radius=26, fill=ACCENT_LIGHT)
        d.rounded_rectangle((190, size / 2 - 52, size - 190, size / 2 + 52), radius=26, fill=ACCENT_LIGHT)
    else:  # a stack of three packs (boxes)
        for i, (y, c) in enumerate(((470, ACCENT_DARK), (360, ACCENT), (250, ACCENT_LIGHT))):
            d.rounded_rectangle((180 + i * 10, y, size - 180 - i * 10, y + 120), radius=28, fill=c, outline=(217, 200, 255), width=8)
            d.rectangle((size / 2 - 40, y + 30, size / 2 + 40, y + 50), fill=(234, 225, 255))
    return img


SOURCE = HERE / "vellora_icons_source.png"
ICON = HERE / "nyxora_icon_user.png"   # the icon exactly as the user sent it
FRAME = (134, 130, 1110, 1081)     # the glowing frame line in the icon picture
GLOW = 12                          # how much of the glow outside the line stays in the icon
CORNER = 232                       # the radius of the frame's corners
OBJECT = (232, 208, 1028, 1004)    # the cube with its orbit, without the frame: for the smallest sizes
PACKS_BOX = (977, 150, 1564, 737)  # the tile "Готовые сборки": object 2 with its frame, in SOURCE


def icon_picture():
    return Image.open(ICON).convert("RGBA")


def to_violet(img):
    """the crimson of the earlier picture turned to the violet of the icon: every hue moves by the same angle (red to
    violet), greys stay grey"""
    h, s, v = img.convert("RGB").convert("HSV").split()
    h = h.point(lambda x: (x - round(94 / 360 * 256)) % 256)
    out = Image.merge("HSV", (h, s, v)).convert("RGBA")
    out.putalpha(img.getchannel("A"))
    return out


def framed(box, size):
    """the icon with its glowing frame, the outside of the rounded square see-through (box None), or an object of
    SOURCE with its frame (the tile "Готовые сборки")"""
    if box is not None:
        img = to_violet(Image.open(SOURCE).convert("RGBA").crop(box)).resize((size, size), Image.LANCZOS)
        radius, inset = 0.205, 6
    else:
        # the frame is a little wider than it is tall: it is fitted to a square (the cube changes by 2.5%, unseen)
        x0, y0, x1, y1 = FRAME
        img = icon_picture().crop((x0 - GLOW, y0 - GLOW, x1 + GLOW, y1 + GLOW)).resize((size, size), Image.LANCZOS)
        radius, inset = (CORNER + GLOW) / (x1 - x0 + 2 * GLOW), 0
    mask = Image.new("L", (size * 4, size * 4), 0)
    ImageDraw.Draw(mask).rounded_rectangle((inset, inset, size * 4 - 1 - inset, size * 4 - 1 - inset), radius=int(size * 4 * radius), fill=255)
    img.putalpha(mask.resize((size, size), Image.LANCZOS))
    return img


def small(size):
    """the icon for 16 to 32 pixels: the cube with its orbit close up on the dark background, without the frame (its
    thin line would vanish), the corners rounded"""
    img = icon_picture().crop(OBJECT).resize((size * 4, size * 4), Image.LANCZOS)
    mask = Image.new("L", img.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, img.size[0] - 1, img.size[1] - 1), radius=int(img.size[0] * 0.22), fill=255)
    img.putalpha(mask)
    return img.resize((size, size), Image.LANCZOS)


def adaptive_layers(size):
    """the adaptive icon: the launcher of the phone cuts its own shape out of the middle 72 of 108dp, so the frame is
    left out; the foreground is the inside of the frame, the cube and its orbit within the visible part, and the
    background is the picture's dark colour"""
    picture = icon_picture()
    x0, y0, x1, y1 = FRAME
    line = 26  # the frame line with its inner glow, left out
    inner = picture.crop((x0 + line, y0 + line, x1 - line, y1 - line)).convert("RGB")
    edge = picture.convert("RGB").resize((16, 16), Image.BOX).getpixel((0, 0))
    bg = Image.new("RGB", (size, size), edge)
    part = int(size * 78 / 108)  # the orbit stays inside the round mask (66dp) of the phone's launcher
    scaled = inner.resize((part, part), Image.LANCZOS).convert("RGBA")
    # the edge of the picture fades into the background: no seam when the launcher moves the layers apart
    fade = Image.new("L", (part, part), 0)
    soft = max(2, part // 24)
    ImageDraw.Draw(fade).rectangle((soft, soft, part - 1 - soft, part - 1 - soft), fill=255)
    scaled.putalpha(fade.filter(ImageFilter.GaussianBlur(soft / 2)))
    fg = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    fg.alpha_composite(scaled, ((size - part) // 2, (size - part) // 2))
    return bg, fg


def main():
    for name, k in DENSITIES.items():
        folder = RES / f"mipmap-{name}"
        size = round(108 * k)
        bg, fg = adaptive_layers(size * 2)
        bg.resize((size, size), Image.LANCZOS).save(folder / "ic_launcher_background.png")
        fg.resize((size, size), Image.LANCZOS).save(folder / "ic_launcher_foreground.png")
        framed(None, round(48 * k)).save(folder / "ic_launcher.png")
    nodpi = RES / "drawable-nodpi"
    framed(None, 384).save(nodpi / "install_default.png")
    tile("plus").save(nodpi / "tile_new.png")
    framed(PACKS_BOX, 384).save(nodpi / "tile_packs.png")
    framed(PACKS_BOX, 384).save(HERE.parent.parent / "launcher" / "resources" / "sculk_theme" / "tile-packs-vellora.png")
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
    prev.save(HERE / "nyxora_preview.png")


if __name__ == "__main__":
    main()
