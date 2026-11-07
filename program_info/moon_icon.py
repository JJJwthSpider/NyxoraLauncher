#!/usr/bin/env python3
"""Generates the Lullaby Launcher icon, logos and the picture of Moony, the mod pack agent.

The icon is a big moon in the night sky with a see-through rainbow and stars, like the background of the
launcher. Moony is a little moon whose left half is night dark and right half moon white, with a sleepy
face (the animated Moony is drawn by launcher/ui/widgets/MoonyWidget.cpp the same way).

Run from the program_info directory:  python3 moon_icon.py
Then run genicons.sh (or the PNG/ICO/ICNS steps in it) to refresh the raster icons.
"""

import math
import pathlib
import random

HERE = pathlib.Path(__file__).resolve().parent

RAINBOW = ["#ff5c78", "#ffa84c", "#ffe666", "#6ce08c", "#5cb6ff", "#a880ff"]

DEFS = """<defs>
  <linearGradient id="sky" x1="0" y1="0" x2="0" y2="1">
   <stop offset="0" stop-color="#05060d"/>
   <stop offset="0.65" stop-color="#0d1020"/>
   <stop offset="1" stop-color="#1a1d36"/>
  </linearGradient>
  <radialGradient id="moonBody" cx="0.38" cy="0.36" r="0.75">
   <stop offset="0" stop-color="#ffffff"/>
   <stop offset="0.7" stop-color="#e6e8f1"/>
   <stop offset="1" stop-color="#b9bccc"/>
  </radialGradient>
  <radialGradient id="halo" cx="0.5" cy="0.5" r="0.5">
   <stop offset="0.45" stop-color="#ffffff" stop-opacity="0.28"/>
   <stop offset="1" stop-color="#dce4ff" stop-opacity="0"/>
  </radialGradient>
  <radialGradient id="nightHalf" cx="0.3" cy="0.3" r="0.9">
   <stop offset="0" stop-color="#2c3150"/>
   <stop offset="1" stop-color="#0e101c"/>
  </radialGradient>
  <radialGradient id="dayHalf" cx="0.7" cy="0.3" r="0.9">
   <stop offset="0" stop-color="#ffffff"/>
   <stop offset="1" stop-color="#c9ccdb"/>
  </radialGradient>
 </defs>"""


def sparkle(cx, cy, size, opacity=1.0, color="#ffffff"):
    s = size
    return (f'<path d="M{cx} {cy - s} Q{cx} {cy} {cx + s} {cy} Q{cx} {cy} {cx} {cy + s} Q{cx} {cy} {cx - s} {cy} '
            f'Q{cx} {cy} {cx} {cy - s}Z" fill="{color}" fill-opacity="{opacity}"/>')


def stars(x0, y0, w, h, count, seed):
    rnd = random.Random(seed)
    out = []
    for _ in range(count):
        x = x0 + rnd.random() * w
        y = y0 + rnd.random() * h
        r = rnd.choice([0.9, 1.1, 1.4, 1.8])
        out.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{r}" fill="#ffffff" fill-opacity="{0.35 + rnd.random() * 0.6:.2f}"/>')
    return "\n  ".join(out)


def rainbow(cx, cy, radius, band, opacity):
    """A see-through rainbow arc: six circles of decreasing radius, the outer one red."""
    out = []
    for i, color in enumerate(RAINBOW):
        r = radius - i * band
        out.append(f'<circle cx="{cx}" cy="{cy}" r="{r:.1f}" fill="none" stroke="{color}" stroke-opacity="{opacity}" '
                   f'stroke-width="{band:.1f}"/>')
    return "\n  ".join(out)


def big_moon(cx, cy, r):
    """The full moon of the background: soft glow, gray seas and craters."""
    seas = [(-0.35, -0.2, 0.32), (0.25, 0.25, 0.28), (0.3, -0.35, 0.18)]
    craters = [(-0.5, 0.35, 0.09), (0.45, -0.05, 0.07), (-0.15, -0.55, 0.06), (0.1, 0.6, 0.08), (-0.6, -0.1, 0.05)]
    parts = [f'<circle cx="{cx}" cy="{cy}" r="{r * 1.9:.1f}" fill="url(#halo)"/>',
             f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="url(#moonBody)"/>']
    for dx, dy, s in seas:
        parts.append(f'<circle cx="{cx + dx * r:.1f}" cy="{cy + dy * r:.1f}" r="{s * r:.1f}" fill="#9ca0b4" fill-opacity="0.28"/>')
    for dx, dy, s in craters:
        x, y = cx + dx * r, cy + dy * r
        parts.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{s * r:.1f}" fill="#8e92a6" fill-opacity="0.45"/>')
        parts.append(f'<circle cx="{x - s * r * 0.2:.1f}" cy="{y - s * r * 0.2:.1f}" r="{s * r * 0.7:.1f}" fill="#ffffff" fill-opacity="0.35"/>')
    return "\n  ".join(parts)


def moony(cx, cy, r, awake=False):
    """Moony: the left half night dark, the right half moon white, each half's face in the other color."""
    eye_x, eye_w, pen = r * 0.38, r * 0.2, max(2.0, r * 0.08)
    eye_y = cy - r * 0.05
    parts = [
        f'<circle cx="{cx}" cy="{cy}" r="{r * 1.5:.1f}" fill="url(#halo)"/>',
        f'<path d="M{cx} {cy - r} A{r} {r} 0 0 0 {cx} {cy + r}Z" fill="url(#nightHalf)"/>',
        f'<path d="M{cx} {cy - r} A{r} {r} 0 0 1 {cx} {cy + r}Z" fill="url(#dayHalf)"/>',
        f'<circle cx="{cx - 0.55 * r:.1f}" cy="{cy - 0.45 * r:.1f}" r="{0.13 * r:.1f}" fill="#ffffff" fill-opacity="0.1"/>',
        f'<circle cx="{cx + 0.6 * r:.1f}" cy="{cy - 0.5 * r:.1f}" r="{0.09 * r:.1f}" fill="#9a9eb4" fill-opacity="0.35"/>',
        f'<circle cx="{cx + 0.55 * r:.1f}" cy="{cy + 0.5 * r:.1f}" r="{0.12 * r:.1f}" fill="#9a9eb4" fill-opacity="0.3"/>',
        sparkle(cx - 0.35 * r, cy - 0.62 * r, 0.1 * r, 0.9),
        f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="none" stroke="#ffffff" stroke-opacity="0.5" stroke-width="{max(1.5, r * 0.04):.1f}"/>',
    ]
    for side, color in ((-1, "#f6f7fc"), (1, "#0e101c")):
        ex = cx + side * eye_x
        if awake:
            parts.append(f'<ellipse cx="{ex:.1f}" cy="{eye_y:.1f}" rx="{eye_w * 0.65:.1f}" ry="{eye_w * 0.8:.1f}" fill="{color}"/>')
        else:
            parts.append(f'<path d="M{ex - eye_w:.1f} {eye_y:.1f} Q{ex:.1f} {eye_y + eye_w * 0.9:.1f} {ex + eye_w:.1f} {eye_y:.1f}" '
                         f'fill="none" stroke="{color}" stroke-width="{pen:.1f}" stroke-linecap="round"/>')
        parts.append(f'<ellipse cx="{cx + side * r * 0.55:.1f}" cy="{eye_y + r * 0.28:.1f}" rx="{r * 0.13:.1f}" ry="{r * 0.07:.1f}" '
                     f'fill="#ff96af" fill-opacity="0.45"/>')
    mouth_y, mouth_w = cy + r * 0.3, r * 0.22
    parts.append(f'<path d="M{cx - mouth_w:.1f} {mouth_y:.1f} Q{cx - mouth_w * 0.5:.1f} {mouth_y + mouth_w * 0.6:.1f} {cx} {mouth_y + mouth_w * 0.4:.1f}" '
                 f'fill="none" stroke="#f6f7fc" stroke-width="{pen:.1f}" stroke-linecap="round"/>')
    parts.append(f'<path d="M{cx} {mouth_y + mouth_w * 0.4:.1f} Q{cx + mouth_w * 0.5:.1f} {mouth_y + mouth_w * 0.6:.1f} {cx + mouth_w:.1f} {mouth_y:.1f}" '
                 f'fill="none" stroke="#0e101c" stroke-width="{pen:.1f}" stroke-linecap="round"/>')
    for dx, dy, s, o in ((-1.3, -0.8, 0.14, 0.9), (1.3, -0.95, 0.11, 0.8), (1.4, 0.55, 0.12, 0.7), (-1.38, 0.6, 0.09, 0.6)):
        parts.append(sparkle(cx + dx * r, cy + dy * r, s * r, o, "#dce4ff"))
    return "\n  ".join(parts)


def scene(size, rx):
    """The app icon: night sky, a see-through rainbow, the big moon and sparkling stars."""
    s = size / 256
    return f"""<rect x="{8 * s}" y="{8 * s}" width="{240 * s}" height="{240 * s}" rx="{rx}" fill="url(#sky)" stroke="#ffffff" stroke-opacity="0.35" stroke-width="{3 * s}"/>
 <clipPath id="tile"><rect x="{8 * s}" y="{8 * s}" width="{240 * s}" height="{240 * s}" rx="{rx}"/></clipPath>
 <g clip-path="url(#tile)">
  {stars(12 * s, 12 * s, 232 * s, 232 * s, 26, 7)}
  {rainbow(100 * s, 300 * s, 190 * s, 9 * s, 0.38)}
  {big_moon(150 * s, 104 * s, 62 * s)}
  {sparkle(52 * s, 62 * s, 12 * s)}
  {sparkle(206 * s, 196 * s, 9 * s, 0.85)}
  {sparkle(78 * s, 160 * s, 6 * s, 0.7)}
 </g>"""


def icon_svg():
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<svg width="256" height="256" version="1.1" viewBox="0 0 256 256" xmlns="http://www.w3.org/2000/svg">
 <title>Lullaby Launcher</title>
 {DEFS}
 {scene(256, 56)}
</svg>
"""


def bigsur_svg():
    """macOS style icon: the tile is smaller and leaves the usual margin."""
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<svg width="1024" height="1024" version="1.1" viewBox="0 0 1024 1024" xmlns="http://www.w3.org/2000/svg">
 <title>Lullaby Launcher</title>
 {DEFS}
 <g transform="translate(100 100) scale(3.4333)">
  <g transform="translate(-8 -8)">{scene(256, 46)}</g>
 </g>
</svg>
"""


def moon_only_svg(size=256):
    """Just the moon without a tile (instance icon, icon composer layer)."""
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<svg width="{size}" height="{size}" version="1.1" viewBox="0 0 256 256" xmlns="http://www.w3.org/2000/svg">
 {DEFS}
 {rainbow(128, 250, 120, 7, 0.4)}
 {big_moon(128, 120, 84)}
</svg>
"""


def logo_svg(text_color):
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<svg width="1024" height="256" version="1.1" viewBox="0 0 1024 256" xmlns="http://www.w3.org/2000/svg">
 <title>Lullaby Launcher</title>
 {DEFS}
 {scene(256, 56)}
 <text x="290" y="150" font-family="sans-serif" font-size="104" font-weight="bold" fill="{text_color}">Lullaby</text>
 <text x="294" y="210" font-family="sans-serif" font-size="48" fill="#a880ff">Launcher</text>
</svg>
"""


def agent_svg():
    """Moony with a speech bubble, because the agent talks to you in a chat."""
    dots = "".join(f'<circle cx="{150 + i * 24}" cy="48" r="9" fill="{c}"/>' for i, c in enumerate(["#ffa8c4", "#ffeca0", "#a0d2ff"]))
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<svg width="256" height="256" version="1.1" viewBox="0 0 256 256" xmlns="http://www.w3.org/2000/svg">
 {DEFS}
 {moony(104, 150, 74)}
 <path d="M132 16h88a14 14 0 0 1 14 14v36a14 14 0 0 1-14 14h-54l-26 24 6-24h-14a14 14 0 0 1-14-14v-36a14 14 0 0 1 14-14z"
       fill="#14151f" stroke="#ffffff" stroke-opacity="0.85" stroke-width="5"/>
 {dots}
</svg>
"""


EMPTY = """<?xml version="1.0" encoding="UTF-8"?>
<svg width="256" height="256" version="1.1" viewBox="0 0 256 256" xmlns="http://www.w3.org/2000/svg"/>
"""


def main():
    files = {
        HERE / "org.prismlauncher.PrismLauncher.svg": icon_svg(),
        HERE / "org.prismlauncher.PrismLauncher.Source.svg": icon_svg(),
        HERE / "org.prismlauncher.PrismLauncher.bigsur.svg": bigsur_svg(),
        HERE / "org.prismlauncher.PrismLauncher.logo.svg": logo_svg("#14151f"),
        HERE / "org.prismlauncher.PrismLauncher.logo.source.svg": logo_svg("#14151f"),
        HERE / "org.prismlauncher.PrismLauncher.logo-darkmode.svg": logo_svg("#ffffff"),
        HERE / "org.prismlauncher.PrismLauncher.Social.svg": logo_svg("#ffffff"),
        HERE / "LullabyLauncher.icon/Assets/rainbow.svg": moon_only_svg(),
        HERE / "LullabyLauncher.icon/Assets/block.svg": EMPTY,
        HERE / "../launcher/resources/multimc/scalable/launcher.svg": icon_svg(),
        HERE / "../launcher/resources/multimc/scalable/instances/prismlauncher.svg": moon_only_svg(),
        HERE / "../launcher/resources/sculk_theme/agent.svg": agent_svg(),
    }
    for path, content in files.items():
        path.write_text(content, encoding="utf-8")
        print("wrote", path.resolve().relative_to(HERE.parent))


if __name__ == "__main__":
    main()
