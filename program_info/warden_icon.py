#!/usr/bin/env python3
"""Generates the SculkShoot Client icon, logos and the Warden pictures.

The head is pixel art (20x17) so it looks like Minecraft; the same grid is used by the
animated Warden in the launcher (launcher/ui/widgets/WardenWidget.cpp).

Run from the program_info directory:  python3 warden_icon.py
Then run genicons.sh (or the PNG/ICO/ICNS steps in it) to refresh the raster icons.
"""

import pathlib

# The Warden head of the AI agent, pixel for pixel like the Warden in Minecraft (taken from a picture with
# warden_from_picture.py): a 16x16 head in columns 10..25 with the antlers spreading to the sides.
# The same grid is in launcher/ui/widgets/WardenWidget.cpp.
# A light antler tip   B, C antler   D..H dark skin   I, J, M darkest skin   K teal in the mouth   L the mouth
WARDEN = [
    ".A.B............................B.A.",
    ".BBB............................BBB.",
    "BBBC......DDDDDDDDDDDDDDDD......CBBB",
    "BAC.......DEDFFDGFDFFFFFFD.......CAB",
    "BBCE......EDFFFFFFFFFFFFFF......ECBB",
    "ABBCBD.E..FFFHGHFHFFHHHHHH..E.DBCBBA",
    "A.BBDDDEG.IHGIGIHIIIHIHIII.GEDDDBB.A",
    "...A..FFGGHGIIIHIHHIHIIIIIGGFF..A...",
    "...A..AFEGIGIIHFFFFFFHIIIIGEFA..A...",
    "........EGJIIHFFFKKFFFIIIIGE........",
    "..........JIIFGLLLLLLGFIII..........",
    "..........JIFLLLLLLLLLLFII..........",
    "..........IIHLLMMMMMMLLHII..........",
    "..........IHLMMMHHHHMMMLHI..........",
    "..........IGFMMKKKEKFMMFGI..........",
    "..........IIHFFEIIIIEFFHII..........",
    "..........IIHHHIIIIIIHHHII..........",
    "..........IIIHIIIIIIIIHIIH..........",
]

COLORS = {"A": "#a2b1b8", "B": "#3d5c6a", "C": "#344852", "D": "#263137", "E": "#172123", "F": "#1a2328", "G": "#16151a", "H": "#1b1a20", "I": "#090911", "J": "#0e0d13", "K": "#2e3f46", "L": "#000000", "M": "#06050b"}
COLS, ROWS = len(WARDEN[0]), len(WARDEN)
# the soul glowing in the throat (also in WardenWidget.cpp, animated there)
MOUTH_CENTER = (17.5, 11.5)
# glowing dots along the left antler (mirrored on the right) and sparkles around the head
ANTLER_DOTS = [(2, 1), (1, 4), (4, 5), (3, 6), (7, 5), (8, 7)]
SPARKLES = [(3.5, 10.0), (32.5, 10.0), (6.5, 14.0), (29.5, 14.0), (5.0, 1.5), (31.0, 1.5)]

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
    """The glowing parts of the Warden: light along the antlers, their tips and the soul in the throat."""
    spots = []
    for y, row in enumerate(WARDEN):
        for x, c in enumerate(row):
            if c == "A":
                spots.append((x0 + (x + 0.5) * size, y0 + (y + 0.5) * size, size * 1.5, "earGlow"))  # the antler tips
    cells = ""
    # the antlers carry a stripe of light from the head out to the tips
    for y, row in enumerate(WARDEN):
        for x, c in enumerate(row):
            if c in "BC" and (x < 10 or x > 25) and (x + y) % 3 == 0:
                cells += f'<rect x="{x0 + x * size:g}" y="{y0 + y * size:g}" width="{size:g}" height="{size:g}" fill="#2fb8c2"/>'
    for dx, dy in ANTLER_DOTS:
        for x in (dx, COLS - 1 - dx):
            cells += f'<rect x="{x0 + x * size:g}" y="{y0 + dy * size:g}" width="{size:g}" height="{size:g}" fill="#c4fdff"/>'
            spots.append((x0 + (x + 0.5) * size, y0 + (dy + 0.5) * size, size * 1.8, "earGlow"))
    for i, (sx, sy) in enumerate(SPARKLES):
        cx, cy, arm = x0 + (sx + 0.5) * size, y0 + (sy + 0.5) * size, size * (1.4 if i % 2 == 0 else 1.0)
        cells += (f'<path d="M{cx - arm:g} {cy:g}H{cx + arm:g}M{cx:g} {cy - arm:g}V{cy + arm:g}" stroke="#c8fcff" '
                  f'stroke-width="{size * 0.35:g}" stroke-linecap="round"/>')
    mx, my = MOUTH_CENTER
    throat = (f'<ellipse cx="{x0 + (mx + 0.5) * size:g}" cy="{y0 + (my + 0.5) * size:g}" rx="{4 * size:g}" ry="{2.6 * size:g}" '
              f'fill="url(#soulGlow)"/>')
    return cells + throat + "\n  ".join(f'<circle cx="{cx:g}" cy="{cy:g}" r="{r:g}" fill="url(#{g})"/>' for cx, cy, r, g in spots)


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
  <linearGradient id="beam" x1="0" y1="1" x2="0" y2="0">
   <stop offset="0" stop-color="#3ff0f8" stop-opacity="0.75"/>
   <stop offset="1" stop-color="#3ff0f8" stop-opacity="0"/>
  </linearGradient>
  <radialGradient id="soulGlow" cx="0.5" cy="0.5" r="0.5">
   <stop offset="0" stop-color="#29dfeb" stop-opacity="0.55"/>
   <stop offset="1" stop-color="#29dfeb" stop-opacity="0"/>
  </radialGradient>
 </defs>"""


def head(x0, y0, size):
    return f"""<circle cx="{x0 + COLS / 2 * size:g}" cy="{y0 + ROWS * 0.55 * size:g}" r="{12 * size:g}" fill="url(#halo)"/>
  {pixels(x0, y0, size)}
  {glow_spots(x0, y0, size)}"""


def emblem(cx, cy, scale):
    """The former emblem (kept for reference): a sniper reticle made of sculk. Two Warden horns grow out of the ring,
    the centre is the glowing soul of a sculk sensor. Drawn for a 256 unit square and scaled."""
    k = scale
    def P(x, y):
        return f"{cx + x * k:g} {cy + y * k:g}"
    horn = lambda side: (
        f'<path d="M{P(side * 46, -62)} C{P(side * 58, -92)} {P(side * 86, -104)} {P(side * 104, -118)} '
        f'C{P(side * 96, -94)} {P(side * 84, -78)} {P(side * 70, -58)} Z" fill="#1a8c96" stroke="#29dfeb" stroke-width="{3 * k:g}"/>'
        f'<circle cx="{cx + side * 98 * k:g}" cy="{cy - 110 * k:g}" r="{10 * k:g}" fill="url(#earGlow)"/>')
    ticks = "".join(
        f'<path d="M{P(x1, y1)} L{P(x2, y2)}" stroke="#29dfeb" stroke-width="{11 * k:g}" stroke-linecap="round"/>'
        for x1, y1, x2, y2 in ((0, -96, 0, -66), (0, 66, 0, 96), (-96, 0, -66, 0), (66, 0, 96, 0)))
    return f"""<circle cx="{cx:g}" cy="{cy:g}" r="{100 * k:g}" fill="url(#halo)"/>
  {horn(-1)}
  {horn(1)}
  <circle cx="{cx:g}" cy="{cy:g}" r="{78 * k:g}" fill="#071418" stroke="#0e8f99" stroke-width="{4 * k:g}"/>
  <circle cx="{cx:g}" cy="{cy:g}" r="{78 * k:g}" fill="none" stroke="#29dfeb" stroke-width="{10 * k:g}"
          stroke-dasharray="{82 * k:g} {40.5 * k:g}" stroke-dashoffset="{-20 * k:g}"/>
  {ticks}
  <circle cx="{cx:g}" cy="{cy:g}" r="{44 * k:g}" fill="none" stroke="#0e8f99" stroke-width="{3 * k:g}" stroke-dasharray="{6 * k:g} {6 * k:g}"/>
  <circle cx="{cx:g}" cy="{cy:g}" r="{40 * k:g}" fill="url(#soulGlow)"/>
  <path d="M{P(0, -30)} L{P(26, 0)} L{P(0, 30)} L{P(-26, 0)} Z" fill="#0b5560" stroke="#29dfeb" stroke-width="{4 * k:g}"/>
  <path d="M{P(0, -15)} L{P(13, 0)} L{P(0, 15)} L{P(-13, 0)} Z" fill="#b8fbff"/>"""


def sonic_boom(cx, cy, scale):
    """The SculkShoot emblem: a sculk shrieker fires the Warden's sonic boom. Bone jaws frame a glowing soul mouth at
    the bottom, a beam with widening cyan rings shoots up from it. Drawn for a 256 unit square and scaled."""
    k = scale

    def P(x, y):
        return f"{cx + x * k:g} {cy + y * k:g}"

    def ring(y, rx, ry, width, opacity):
        return (f'<ellipse cx="{cx:g}" cy="{cy + y * k:g}" rx="{rx * k:g}" ry="{ry * k:g}" fill="none" stroke="#3ff0f8" '
                f'stroke-width="{width * k:g}" stroke-opacity="{opacity}"/>'
                f'<ellipse cx="{cx:g}" cy="{cy + y * k:g}" rx="{rx * k:g}" ry="{ry * k:g}" fill="none" stroke="#e6feff" '
                f'stroke-width="{width * 0.35 * k:g}" stroke-opacity="{opacity}"/>')

    jaw = lambda side: (
        # a bone jaw: a curved horn rising from the base and bending over the mouth
        f'<path d="M{P(side * 30, 78)} C{P(side * 70, 74)} {P(side * 92, 50)} {P(side * 86, 18)} '
        f'L{P(side * 70, 30)} C{P(side * 70, 52)} {P(side * 52, 62)} {P(side * 26, 62)} Z" '
        f'fill="#efe6d2" stroke="#9c8f74" stroke-width="{3 * k:g}" stroke-linejoin="round"/>'
        f'<path d="M{P(side * 86, 18)} L{P(side * 98, 4)} L{P(side * 76, 22)} Z" fill="#efe6d2" stroke="#9c8f74" stroke-width="{2 * k:g}"/>')
    particles = "".join(
        f'<rect x="{cx + x * k:g}" y="{cy + y * k:g}" width="{s * k:g}" height="{s * k:g}" fill="#8ff6fb" fill-opacity="{o}"/>'
        for x, y, s, o in ((-62, -40, 7, 0.9), (54, -70, 6, 0.8), (-44, -92, 5, 0.6), (70, -12, 5, 0.7), (-80, 6, 4, 0.5), (36, -104, 4, 0.5)))
    return f"""<circle cx="{cx:g}" cy="{cy + 10 * k:g}" r="{108 * k:g}" fill="url(#halo)"/>
  <path d="M{P(-22, 52)} L{P(-34, -112)} L{P(34, -112)} L{P(22, 52)} Z" fill="url(#beam)"/>
  {ring(14, 30, 9, 7, 1)}
  {ring(-30, 46, 13, 7, 0.9)}
  {ring(-78, 64, 18, 6, 0.7)}
  <rect x="{cx - 64 * k:g}" y="{cy + 62 * k:g}" width="{128 * k:g}" height="{40 * k:g}" rx="{6 * k:g}" fill="#0d3b45" stroke="#0e8f99" stroke-width="{3 * k:g}"/>
  <rect x="{cx - 50 * k:g}" y="{cy + 74 * k:g}" width="{10 * k:g}" height="{10 * k:g}" fill="#29dfeb"/>
  <rect x="{cx + 34 * k:g}" y="{cy + 84 * k:g}" width="{10 * k:g}" height="{8 * k:g}" fill="#29dfeb" fill-opacity="0.8"/>
  <rect x="{cx - 8 * k:g}" y="{cy + 88 * k:g}" width="{8 * k:g}" height="{8 * k:g}" fill="#1a8c96"/>
  <ellipse cx="{cx:g}" cy="{cy + 62 * k:g}" rx="{40 * k:g}" ry="{13 * k:g}" fill="#03161b" stroke="#29dfeb" stroke-width="{3 * k:g}"/>
  <ellipse cx="{cx:g}" cy="{cy + 62 * k:g}" rx="{26 * k:g}" ry="{7 * k:g}" fill="url(#soulGlow)"/>
  {jaw(-1)}
  {jaw(1)}
  {particles}"""


def icon_svg(rounded=True):
    """Square app icon: the head on a dark sculk tile."""
    tile = '<rect x="8" y="8" width="240" height="240" rx="56" fill="url(#bg)" stroke="#29dfeb" stroke-opacity="0.55" stroke-width="4"/>' if rounded else ""
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<svg width="256" height="256" version="1.1" viewBox="0 0 256 256" xmlns="http://www.w3.org/2000/svg">
 <title>SculkShoot Client</title>
 {DEFS}
 {tile}
 {sonic_boom(128, 122, 0.95)}
</svg>
"""


def bigsur_svg():
    """macOS style icon: the tile is smaller and leaves the usual margin."""
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<svg width="1024" height="1024" version="1.1" viewBox="0 0 1024 1024" xmlns="http://www.w3.org/2000/svg">
 <title>SculkShoot Client</title>
 {DEFS}
 <rect x="100" y="100" width="824" height="824" rx="185" fill="url(#bg)" stroke="#29dfeb" stroke-opacity="0.5" stroke-width="8"/>
 {sonic_boom(512, 495, 3.05)}
</svg>
"""


def head_only_svg(size=256):
    """Just the head without a tile (instance icon, icon composer layer)."""
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<svg width="{size}" height="{size}" version="1.1" viewBox="0 0 256 256" xmlns="http://www.w3.org/2000/svg">
 {DEFS}
 <g shape-rendering="crispEdges">
  {head(2, 66, 7.0)}
 </g>
</svg>
"""


def logo_svg(text_color):
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<svg width="1024" height="256" version="1.1" viewBox="0 0 1024 256" xmlns="http://www.w3.org/2000/svg">
 <title>SculkShoot Client</title>
 {DEFS}
 <rect x="8" y="8" width="240" height="240" rx="56" fill="url(#bg)" stroke="#29dfeb" stroke-opacity="0.55" stroke-width="4"/>
 {sonic_boom(128, 122, 0.95)}
 <text x="290" y="150" font-family="sans-serif" font-size="104" font-weight="bold" fill="{text_color}">SculkShoot</text>
 <text x="294" y="210" font-family="sans-serif" font-size="48" fill="#29dfeb">Client</text>
</svg>
"""


def agent_svg():
    """The AI agent: the Warden head with a speech bubble, because the agent talks to you in a chat."""
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<svg width="256" height="256" version="1.1" viewBox="0 0 256 256" xmlns="http://www.w3.org/2000/svg">
 {DEFS}
 <g shape-rendering="crispEdges">
  {head(2, 116, 7.0)}
 </g>
 <path d="M110 16h72a12 12 0 0 1 12 12v36a12 12 0 0 1-12 12h-48l-20 22 4-22h-8a12 12 0 0 1-12-12v-36a12 12 0 0 1 12-12z"
       fill="#0b1f25" stroke="#29dfeb" stroke-width="5"/>
 <circle cx="125" cy="46" r="8" fill="#8ff6fb"/>
 <circle cx="146" cy="46" r="8" fill="#29dfeb"/>
 <circle cx="167" cy="46" r="8" fill="#1a8c96"/>
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
        HERE / "SculkShootClient.icon/Assets/rainbow.svg": head_only_svg(),
        HERE / "SculkShootClient.icon/Assets/block.svg": EMPTY,
        HERE / "../launcher/resources/multimc/scalable/launcher.svg": icon_svg(),
        HERE / "../launcher/resources/multimc/scalable/instances/prismlauncher.svg": head_only_svg(),
        HERE / "../launcher/resources/sculk_theme/agent.svg": agent_svg(),
    }
    for path, content in files.items():
        path.write_text(content, encoding="utf-8")
        print("wrote", path.resolve().relative_to(HERE.parent))


if __name__ == "__main__":
    main()
