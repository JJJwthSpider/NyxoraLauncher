#!/usr/bin/env python3
"""The finishing of every accessory of the wardrobe (art/cosmetics.py calls `finish` on each layer before writing
cosmetics.json). An accessory is a plain model, like a Minecraft model made in Blockbench: its pixels are the size of
the skin's pixels and it is painted the way Minecraft's own textures are, in three tones of each colour:

1. one light from the top left: a pixel on the top or left edge of its colour is the light tone, one on the bottom or
   right edge the shadow tone, the rest the colour itself. A stroke two pixels wide reads as round, a gem gets its
   light corner and its deep one, a plate its bevel;
2. the tones are a painter's: the shadow leans to blue-violet and is a little purer, the light leans to warm yellow;
   gold and steel get more contrast;
3. relief: the pixels inside a shape stand out a little (digit 2) over those on its edge (0), so the 3D views show a
   wing or a badge as a bevelled piece, not a flat plate.

The layer gets "relief" (rows of digits). The views (VelloraCharacter.cpp, CharacterView.java and the wardrobe tiles)
read it; they also read "res" (row characters per skin pixel), which is 1 here.
"""
import colorsys

RES = 1
# the characters of a finished palette: printable, not '.', ' ', '"' or '\\' (and below 128: Cosmetics.java indexes by them)
PALETTE_CHARS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!#$%&()*+,-/:;<=>?@[]^_`{|}~"
EMPTY = "."


def parse(hex_color):
    h = hex_color.lstrip("#")
    r, g, b = int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16)
    a = int(h[6:8], 16) if len(h) == 8 else 255
    return r, g, b, a


def to_hex(r, g, b, a):
    s = "#%02X%02X%02X" % (r, g, b)
    return s + ("%02X" % a if a < 255 else "")


# ---------------------------------------------------------------------------------------------- helpers
def distance_to_edge(mask, cap):
    """for each filled cell, the distance (chessboard, at most cap) to the nearest empty cell or the outside"""
    h, w = len(mask), len(mask[0])
    big = cap + 1
    d = [[0 if not mask[y][x] else big for x in range(w)] for y in range(h)]
    for y in range(h):
        for x in range(w):
            if mask[y][x] and (x == 0 or y == 0 or x == w - 1 or y == h - 1):
                d[y][x] = 1
    # two passes of a chamfer
    for y in range(h):
        for x in range(w):
            if d[y][x] == 0:
                continue
            best = d[y][x]
            for dx, dy in ((-1, 0), (0, -1), (-1, -1), (1, -1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < w and 0 <= ny < h:
                    best = min(best, d[ny][nx] + 1)
            d[y][x] = best
    for y in range(h - 1, -1, -1):
        for x in range(w - 1, -1, -1):
            if d[y][x] == 0:
                continue
            best = d[y][x]
            for dx, dy in ((1, 0), (0, 1), (1, 1), (-1, 1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < w and 0 <= ny < h:
                    best = min(best, d[ny][nx] + 1)
            d[y][x] = best
    return [[min(cap, v) for v in row] for row in d]


def hue_toward(h, target, amount):
    """moves a hue (0..1) toward a target hue along the shorter way"""
    diff = (target - h + 0.5) % 1.0 - 0.5
    return (h + diff * amount) % 1.0


def material(r, g, b, a):
    h, l, s = colorsys.rgb_to_hls(r / 255, g / 255, b / 255)
    if a < 200:
        return "glass"
    if 0.07 <= h <= 0.16 and s > 0.45 and l > 0.35:
        return "gold"
    if s < 0.16 and 0.45 < l < 0.9 and (b >= r - 8):
        return "steel"
    return "cloth"


def ramp(color):
    """five tones of a colour, from the deepest shadow to the brightest light, painter's way"""
    r, g, b, a = color
    h, l, s = colorsys.rgb_to_hls(r / 255, g / 255, b / 255)
    mat = material(r, g, b, a)
    contrast = 1.35 if mat in ("gold", "steel") else 1.0
    tones = []
    for t in (-2, -1, 0, 1, 2):
        if t < 0:
            # a shadow is the same paint with less light: darker, a little purer, leaning to blue-violet. It is worked
            # out from the colour's purity against its own brightness (HSV): a pale colour (bone, cream, white) keeps a
            # pale shadow, where the lightness scale (HLS) would turn it vivid orange. A pale colour leans less, too.
            k = -t
            hv, sv, vv = colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)
            lean = 0.07 * k * (0.35 + 0.65 * min(1.0, sv / 0.4))
            dark = max(0.03, vv * (1 - 0.30 * k)) if l < 0.12 else vv * (1 - 0.19 * k * contrast)
            rr, gg, bb = colorsys.hsv_to_rgb(hue_toward(hv, 0.70, lean), min(1.0, sv * (1 + 0.10 * k) + 0.03 * k), max(0.0, dark))
            tones.append((round(rr * 255), round(gg * 255), round(bb * 255), a))
            continue
        elif t > 0:
            hh = hue_toward(h, 0.15, 0.06 * t)           # lights lean to warm yellow
            ss = s * (1 - 0.08 * t)
            ll = l + (1 - l) * (0.16 * t * contrast)
            if l < 0.12:
                ll = l + 0.07 * t                         # a sheen on black, not grey
        else:
            hh, ss, ll = h, s, l
        rr, gg, bb = colorsys.hls_to_rgb(hh, max(0.0, min(1.0, ll)), max(0.0, min(1.0, ss)))
        tones.append((round(rr * 255), round(gg * 255), round(bb * 255), a))
    return tones, mat


def regions(grid):
    """the connected areas of one character (4-neighbours): id per cell and the size of each area"""
    h, w = len(grid), len(grid[0])
    rid = [[-1] * w for _ in range(h)]
    sizes = []
    for y in range(h):
        for x in range(w):
            if grid[y][x] == EMPTY or rid[y][x] >= 0:
                continue
            ch, stack, n = grid[y][x], [(x, y)], 0
            rid[y][x] = len(sizes)
            while stack:
                cx, cy = stack.pop()
                n += 1
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = cx + dx, cy + dy
                    if 0 <= nx < w and 0 <= ny < h and rid[ny][nx] < 0 and grid[ny][nx] == ch:
                        rid[ny][nx] = len(sizes)
                        stack.append((nx, ny))
            sizes.append(n)
    return rid, sizes


# ---------------------------------------------------------------------------------------------- the finishing
def finish(layer, slot=""):
    """a layer of an accessory, finished: three tones lit from the top left, and the relief"""
    if "relief" in layer:
        return layer  # finished already
    assert layer.get("res", 1) == 1, "accessories are drawn in skin pixels"
    rows = layer["rows"]
    width = max((len(r) for r in rows), default=0)
    if width == 0:
        return layer
    colors = layer["colors"]
    grid = [[(ch if ch in colors and ch not in (".", " ") else EMPTY) for ch in r.ljust(width, ".")] for r in rows]
    h, w = len(grid), width
    mask = [[grid[y][x] != EMPTY for x in range(w)] for y in range(h)]
    dist = distance_to_edge(mask, 3)
    rid, sizes = regions(grid)
    ramps = {ch: ramp(parse(colors[ch])) for ch in set(c for row in grid for c in row if c != EMPTY)}
    out_rows, relief_rows, palette = [], [], {}

    def code(rgba):
        key = to_hex(*rgba)
        if key not in palette:
            assert len(palette) < len(PALETTE_CHARS), "too many tones in one layer"
            palette[key] = PALETTE_CHARS[len(palette)]
        return palette[key]

    for y in range(h):
        line, rel = [], []
        for x in range(w):
            ch = grid[y][x]
            if ch == EMPTY:
                line.append(".")
                rel.append(".")
                continue
            tones, _ = ramps[ch]

            def other(nx, ny):
                return not (0 <= nx < w and 0 <= ny < h) or grid[ny][nx] != ch

            lit = other(x, y - 1) or other(x - 1, y)
            shaded = other(x, y + 1) or other(x + 1, y)
            tone = 0
            if sizes[rid[y][x]] > 1 and lit != shaded:
                tone = 1 if lit else -1
            line.append(code(tones[tone + 2]))
            rel.append("2" if dist[y][x] > 1 else "0")
        out_rows.append("".join(line).rstrip("."))
        relief_rows.append("".join(rel).rstrip("."))
    finished = dict(layer)
    finished["rows"] = out_rows
    finished["relief"] = relief_rows
    finished["colors"] = {c: k for k, c in palette.items()}
    finished.pop("res", None)
    return finished
