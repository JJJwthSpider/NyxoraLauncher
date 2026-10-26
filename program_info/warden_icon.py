#!/usr/bin/env python3
"""Generates the Warden launcher icon and logos of the Sculk edition.

The head is pixel art (20x17) so it looks like Minecraft; the same grid is used by the
animated Warden in the launcher (launcher/ui/widgets/WardenWidget.cpp).

Run from the program_info directory:  python3 warden_icon.py
Then run genicons.sh (or the PNG/ICO/ICNS steps in it) to refresh the raster icons.
"""

import pathlib

# . transparent   K outline       D dark skin    M skin         L light skin
# E ear (horn)    G ear glow      S eye socket   J mouth        T teeth
# C glowing sculk spot on the cheek
WARDEN = [
    "EE................EE",
    "EGE..............EGE",
    "EGGE............EGGE",
    ".EGGE..........EGGE.",
    "..EGEE........EEGE..",
    "...EEKKKKKKKKKKEE...",
    "...KDDDMMMMMMDDDK...",
    "...KDMMLLLLLLMMDK...",
    "...KDSSSMLLMSSSDK...",
    "...KDSSSMMMMSSSDK...",
    "...KDCMMMMMMMMCDK...",
    "...KDMJJJJJJJJMDK...",
    "...KDJTTJTTJTTJDK...",
    "...KDJJJJJJJJJJDK...",
    "...KDDMMMMMMMMDDK...",
    "....KDDDDDDDDDDK....",
    ".....KKKKKKKKKK.....",
]

COLORS = {
    "K": "#04151a",
    "D": "#0e3a44",
    "M": "#155462",
    "L": "#1f6f7c",
    "E": "#1a8c96",
    "G": "#29dfeb",
    "S": "#020a0c",
    "J": "#082a31",
    "T": "#cfe3dc",
    "C": "#29dfeb",
}

HERE = pathlib.Path(__file__).resolve().parent


def pixels(x0, y0, size):
    """The Warden head as SVG rects, top-left at (x0, y0), one pixel = size units."""
    out = []
    for y, row in enumerate(WARDEN):
        x = 0
        while x < len(row):
            c = row[x]
            if c == ".":
                x += 1
                continue
            # merge horizontal runs of the same color into one rect
            run = 1
            while x + run < len(row) and row[x + run] == c:
                run += 1
            out.append(f'<rect x="{x0 + x * size:g}" y="{y0 + y * size:g}" width="{run * size:g}" height="{size:g}" fill="{COLORS[c]}"/>')
            x += run
    return "\n  ".join(out)


def glow_spots(x0, y0, size):
    """Soft cyan halos around the glowing ear tips and the sculk spots on the cheeks."""
    spots = []
    for y, row in enumerate(WARDEN):
        for x, c in enumerate(row):
            if c == "G":
                spots.append((x0 + (x + 0.5) * size, y0 + (y + 0.5) * size, size * 2.0, "earGlow"))
            elif c == "C":
                spots.append((x0 + (x + 0.5) * size, y0 + (y + 0.5) * size, size * 1.8, "soulGlow"))
    return "\n  ".join(f'<circle cx="{cx:g}" cy="{cy:g}" r="{r:g}" fill="url(#{g})"/>' for cx, cy, r, g in spots)


DEFS = """<defs>
  <linearGradient id="bg" x1="0" y1="0" x2="0" y2="1">
   <stop offset="0" stop-color="#123842"/>
   <stop offset="1" stop-color="#050f12"/>
  </linearGradient>
  <radialGradient id="halo" cx="0.5" cy="0.5" r="0.5">
   <stop offset="0" stop-color="#29dfeb" stop-opacity="0.35"/>
   <stop offset="1" stop-color="#29dfeb" stop-opacity="0"/>
  </radialGradient>
  <radialGradient id="earGlow" cx="0.5" cy="0.5" r="0.5">
   <stop offset="0" stop-color="#8ff6fb" stop-opacity="0.9"/>
   <stop offset="1" stop-color="#29dfeb" stop-opacity="0"/>
  </radialGradient>
  <radialGradient id="soulGlow" cx="0.5" cy="0.5" r="0.5">
   <stop offset="0" stop-color="#29dfeb" stop-opacity="0.55"/>
   <stop offset="1" stop-color="#29dfeb" stop-opacity="0"/>
  </radialGradient>
 </defs>"""


def head(x0, y0, size):
    return f"""<circle cx="{x0 + 10 * size:g}" cy="{y0 + 9 * size:g}" r="{10 * size:g}" fill="url(#halo)"/>
  {pixels(x0, y0, size)}
  {glow_spots(x0, y0, size)}"""


def icon_svg(rounded=True):
    """Square app icon: the head on a dark sculk tile."""
    tile = '<rect x="8" y="8" width="240" height="240" rx="56" fill="url(#bg)" stroke="#29dfeb" stroke-opacity="0.55" stroke-width="4"/>' if rounded else ""
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<svg width="256" height="256" version="1.1" viewBox="0 0 256 256" xmlns="http://www.w3.org/2000/svg">
 <title>Prism Launcher Sculk Edition</title>
 {DEFS}
 {tile}
 <g shape-rendering="crispEdges">
  {head(28, 44, 10)}
 </g>
</svg>
"""


def bigsur_svg():
    """macOS style icon: the tile is smaller and leaves the usual margin."""
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<svg width="1024" height="1024" version="1.1" viewBox="0 0 1024 1024" xmlns="http://www.w3.org/2000/svg">
 <title>Prism Launcher Sculk Edition</title>
 {DEFS}
 <rect x="100" y="100" width="824" height="824" rx="185" fill="url(#bg)" stroke="#29dfeb" stroke-opacity="0.5" stroke-width="8"/>
 <g shape-rendering="crispEdges">
  {head(192, 172, 32)}
 </g>
</svg>
"""


def head_only_svg(size=256):
    """Just the head without a tile (instance icon, icon composer layer)."""
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<svg width="{size}" height="{size}" version="1.1" viewBox="0 0 256 256" xmlns="http://www.w3.org/2000/svg">
 {DEFS}
 <g shape-rendering="crispEdges">
  {head(28, 40, 10)}
 </g>
</svg>
"""


def logo_svg(text_color):
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<svg width="1024" height="256" version="1.1" viewBox="0 0 1024 256" xmlns="http://www.w3.org/2000/svg">
 <title>Prism Launcher Sculk Edition</title>
 {DEFS}
 <rect x="8" y="8" width="240" height="240" rx="56" fill="url(#bg)" stroke="#29dfeb" stroke-opacity="0.55" stroke-width="4"/>
 <g shape-rendering="crispEdges">
  {head(28, 44, 10)}
 </g>
 <text x="290" y="150" font-family="sans-serif" font-size="96" font-weight="bold" fill="{text_color}">Prism Launcher</text>
 <text x="294" y="208" font-family="sans-serif" font-size="44" fill="#29dfeb">Sculk Edition</text>
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
        HERE / "org.prismlauncher.PrismLauncher.logo.svg": logo_svg("#14191e"),
        HERE / "org.prismlauncher.PrismLauncher.logo.source.svg": logo_svg("#14191e"),
        HERE / "org.prismlauncher.PrismLauncher.logo-darkmode.svg": logo_svg("#ffffff"),
        HERE / "org.prismlauncher.PrismLauncher.Social.svg": logo_svg("#ffffff"),
        HERE / "PrismLauncher.icon/Assets/rainbow.svg": head_only_svg(),
        HERE / "PrismLauncher.icon/Assets/block.svg": EMPTY,
        HERE / "../launcher/resources/multimc/scalable/launcher.svg": icon_svg(),
        HERE / "../launcher/resources/multimc/scalable/instances/prismlauncher.svg": head_only_svg(),
    }
    for path, content in files.items():
        path.write_text(content, encoding="utf-8")
        print("wrote", path.resolve().relative_to(HERE.parent))


if __name__ == "__main__":
    main()
