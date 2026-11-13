#!/usr/bin/env python3
"""Nyxora Launcher's desktop icon: the picture the user sent (android/art/nyxora_icon_user.png: a glass cube inside a
hexagon with an orbit, blue and violet, in a glowing rounded frame), the same as the Android app (android/art/nyxora_art.py).
Writes the icon used inside the launcher (org.prismlauncher.PrismLauncher.svg: the picture inside an SVG), the 256 px
PNG, nyxora.ico (Windows) and nyxora.icns (macOS). The sizes of 16 to 32 pixels show the cube close up, without the
frame: its thin line would vanish there.
Run from the program_info directory:  python3 nyxora_icon.py
"""
import base64
import io
import pathlib
import sys

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "android" / "art"))
import nyxora_art  # noqa: E402

from PIL import Image  # noqa: E402


def main():
    big = nyxora_art.framed(None, 512)  # the icon picture the user sent
    big.resize((256, 256), Image.LANCZOS).save(HERE / "org.prismlauncher.PrismLauncher_256.png")
    sizes = [16, 24, 32, 48, 64, 128, 256]
    pictures = [nyxora_art.small(s) if s <= 32 else big.resize((s, s), Image.LANCZOS) for s in sizes]
    pictures[-1].save(HERE / "nyxora.ico", sizes=[(s, s) for s in sizes], append_images=pictures[:-1])
    big.save(HERE / "nyxora.icns")
    data = io.BytesIO()
    big.save(data, "PNG", optimize=True)
    encoded = base64.b64encode(data.getvalue()).decode("ascii")
    (HERE / "org.prismlauncher.PrismLauncher.svg").write_text(
        '<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="256" height="256" '
        'viewBox="0 0 256 256">\n'
        f' <image width="256" height="256" xlink:href="data:image/png;base64,{encoded}"/>\n</svg>\n', encoding="utf-8")


if __name__ == "__main__":
    main()
