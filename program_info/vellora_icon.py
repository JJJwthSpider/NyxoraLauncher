#!/usr/bin/env python3
"""Vellora Client's desktop icon: the crimson crystal "V" in red rain on a burgundy rounded square, the same picture
as the Android app (android/art/vellora_art.py). Writes the vector icon (org.prismlauncher.PrismLauncher.svg, used
inside the launcher), the 256 px PNG, vellora.ico (Windows) and vellora.icns (macOS).
Run from the program_info directory:  python3 vellora_icon.py
"""
import pathlib
import random
import sys

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "android" / "art"))
import vellora_art  # noqa: E402

from PIL import Image  # noqa: E402


def svg():
    rnd = random.Random(4)
    rain = []
    def inside(x, y, r=56):
        # in the rounded square (Qt's SVG renderer has no clip paths, so the rain stays inside by itself)
        cx, cy = min(max(x, r), 256 - r), min(max(y, r), 256 - r)
        return (x - cx) ** 2 + (y - cy) ** 2 <= (r - 4) ** 2 and 4 <= x <= 252 and 4 <= y <= 252

    while len(rain) < 46:
        x, y, n = rnd.uniform(0, 256), rnd.uniform(0, 256), rnd.uniform(10, 26)
        if not (inside(x, y) and inside(x + n * 0.18, y + n)):
            continue
        a = rnd.uniform(0.25, 0.65)
        rain.append(f'<line x1="{x:.1f}" y1="{y:.1f}" x2="{x + n * 0.18:.1f}" y2="{y + n:.1f}" stroke="#ff3c50" '
                    f'stroke-opacity="{a:.2f}" stroke-width="{rnd.choice((1, 1.2, 1.6))}" stroke-linecap="round"/>')

    def pt(x, y, k=0.19, cx=128, cy=133):
        return f"{cx + x * k:.1f},{cy + y * k:.1f}"

    def poly(points, fill):
        return f'<polygon points="{" ".join(pt(*p) for p in points)}" fill="{fill}"/>'

    shape = [(-250, -200), (-220, -250), (-150, -250), (-120, -200), (0, 120), (120, -200), (150, -250), (220, -250), (250, -200), (0, 250)]
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="256" height="256" viewBox="0 0 256 256">
 <defs>
  <radialGradient id="sky" cx="0.5" cy="0.62" r="0.75">
   <stop offset="0" stop-color="#4e0e1c"/>
   <stop offset="1" stop-color="#0c0306"/>
  </radialGradient>
  <radialGradient id="glow" cx="0.5" cy="0.5" r="0.5">
   <stop offset="0" stop-color="#dc1e3a" stop-opacity="0.75"/>
   <stop offset="1" stop-color="#dc1e3a" stop-opacity="0"/>
  </radialGradient>
 </defs>
 <g>
  <rect width="256" height="256" rx="56" ry="56" fill="url(#sky)"/>
  {"".join(rain)}
  <circle cx="128" cy="135" r="92" fill="url(#glow)"/>
  {poly([(-250, -200), (-185, -200), (-60, 185), (0, 250)], "#ff6074")}
  {poly([(-185, -200), (-120, -200), (0, 120), (0, 250), (-60, 185)], "#dc1e3a")}
  {poly([(250, -200), (185, -200), (60, 185), (0, 250)], "#780a1c")}
  {poly([(185, -200), (120, -200), (0, 120), (0, 250), (60, 185)], "#b0122c")}
  {poly([(-250, -200), (-120, -200), (-150, -250), (-220, -250)], "#ff96a2")}
  {poly([(250, -200), (120, -200), (150, -250), (220, -250)], "#c82842")}
  <polygon points="{" ".join(pt(*p) for p in shape)}" fill="none" stroke="#ffbec6" stroke-width="1.2" stroke-linejoin="round"/>
  <path d="M90 66 l2.2 6.8 6.8 2.2 -6.8 2.2 -2.2 6.8 -2.2 -6.8 -6.8 -2.2 6.8 -2.2z" fill="#ffdce1"/>
 </g>
</svg>
'''


def main():
    (HERE / "org.prismlauncher.PrismLauncher.svg").write_text(svg(), encoding="utf-8")
    big = vellora_art.sky(1024, seed=9, drops=60)
    big.alpha_composite(vellora_art.crystal_v(1024, scale=1.15, cy=1024 * 0.53))
    big = vellora_art.rounded(big)
    big.resize((256, 256), Image.LANCZOS).save(HERE / "org.prismlauncher.PrismLauncher_256.png")
    big.save(HERE / "vellora.ico", sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    big.save(HERE / "vellora.icns")


if __name__ == "__main__":
    main()
