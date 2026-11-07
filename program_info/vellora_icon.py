#!/usr/bin/env python3
"""Vellora Client's desktop icon: the user's picture (android/art/vellora_icons_source.png, object 1: a dark cube with a
crimson glow and an orbit with a star in a glowing rounded frame), the same as the Android app (android/art/vellora_art.py).
Writes the icon used inside the launcher (org.prismlauncher.PrismLauncher.svg: the picture inside an SVG), the 256 px
PNG, vellora.ico (Windows) and vellora.icns (macOS).
Run from the program_info directory:  python3 vellora_icon.py
"""
import base64
import io
import pathlib
import sys

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "android" / "art"))
import vellora_art  # noqa: E402

from PIL import Image  # noqa: E402


def main():
    big = vellora_art.framed(vellora_art.ICON_BOX, 512)
    big.resize((256, 256), Image.LANCZOS).save(HERE / "org.prismlauncher.PrismLauncher_256.png")
    big.save(HERE / "vellora.ico", sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    big.save(HERE / "vellora.icns")
    data = io.BytesIO()
    big.save(data, "PNG", optimize=True)
    encoded = base64.b64encode(data.getvalue()).decode("ascii")
    (HERE / "org.prismlauncher.PrismLauncher.svg").write_text(
        '<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="256" height="256" '
        'viewBox="0 0 256 256">\n'
        f' <image width="256" height="256" xlink:href="data:image/png;base64,{encoded}"/>\n</svg>\n', encoding="utf-8")


if __name__ == "__main__":
    main()
