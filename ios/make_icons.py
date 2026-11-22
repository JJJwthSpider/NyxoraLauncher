#!/usr/bin/env python3
"""Nyxora's icons for the iPhone app, made from the icon picture the user sent (android/art/nyxora_icon_user.png, the same
as the PC and Android icons): iOS cuts its own rounded shape out of a full square, so the icon is the inside of the
glowing frame (the glass cube with its orbit) on the picture's dark navy, without the frame line.

Writes ios/art/:
  icon.png             1024x1024, the home screen icon (also the dark and the alternative icons)
  icon-clear.png       1024x1024 with see-through dark parts: the icon for the dark appearance, the logo in the app
  icon-tinted.png      1024x1024 white on see-through: the tinted appearance (iOS 18+ colours it)
ios/prepare.py puts them into Amethyst's asset catalog. Run:  python3 ios/make_icons.py
"""
import pathlib
import sys

from PIL import Image, ImageDraw, ImageFilter

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "android" / "art"))
import nyxora_art  # noqa: E402

SIZE = 1024


def square():
    picture = nyxora_art.icon_picture().convert("RGB")
    inner = picture.crop(nyxora_art.OBJECT)  # the cube with its orbit, away from the frame and its glowing corners
    background = picture.resize((16, 16), Image.BOX).getpixel((0, 0))
    out = Image.new("RGB", (SIZE, SIZE), background)
    part = int(SIZE * 0.92)  # a margin, so the ends of the orbit stay clear of the rounded corners
    scaled = inner.resize((part, part), Image.LANCZOS).convert("RGBA")
    # the edges of the crop fade into the background in a soft round shape: no seam, no corner left over
    fade = Image.new("L", (part, part), 0)
    soft = part // 10
    ImageDraw.Draw(fade).ellipse((-soft, -soft, part - 1 + soft, part - 1 + soft), fill=255)
    scaled.putalpha(fade.filter(ImageFilter.GaussianBlur(soft / 2)))
    out.paste(scaled, ((SIZE - part) // 2, (SIZE - part) // 2), scaled)
    return out


def keyed(img, white=False):
    """the dark background made see-through: the brighter a pixel, the more it shows"""
    rgba = img.convert("RGBA")
    px = rgba.load()
    for y in range(SIZE):
        for x in range(SIZE):
            r, g, b, _ = px[x, y]
            # the white one keeps only the bright lines: a soft glow would be a grey cloud once iOS tints it
            a = max(0, min(255, (max(r, g, b) - 90) * 2)) if white else max(0, min(255, (max(r, g, b) - 28) * 3))
            px[x, y] = (255, 255, 255, a) if white else (r, g, b, a)
    return rgba


def main():
    out = HERE / "art"
    out.mkdir(exist_ok=True)
    icon = square()
    icon.save(out / "icon.png")
    keyed(icon).save(out / "icon-clear.png")
    keyed(icon, white=True).save(out / "icon-tinted.png")


if __name__ == "__main__":
    main()
