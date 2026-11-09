#!/usr/bin/env python3
"""The finishing of every accessory of the wardrobe, done the way a pixel artist finishes a sprite (art/cosmetics.py
calls `finish` on each layer before writing cosmetics.json):

1. three times the resolution: Scale3x (AdvMAME3x) turns every pixel into 3x3 and rounds the stair steps of the
   diagonals and of the corners, keeping the pixel-art look;
2. one light from the top left over the whole item: the silhouette is shaded as a rounded form (lit on the top and the
   left, in shade on the bottom and the right), and every material gets a bevel where it meets another one;
3. colour ramps instead of darker and lighter copies: the shadows lean to blue and purple and get a little more
   saturated, the lights lean to yellow, as painters do; five tones per colour;
4. a selective outline: the edge of the silhouette in a darker tone of its own colour, not black;
5. materials: gold and steel get strong contrast and a bright glint where the light hits, gems a white spark in the top
   left and a deep bottom right;
6. relief: how far each pixel is from the edge of the shape (0..4). The 3D views make an item thicker in the middle
   than at its edges, so a wing or a badge is rounded instead of a flat plate.

The layer gets "res": 3 (rows of three times the size; one row character is a third of a skin pixel) and "relief"
(rows of digits). The views (VelloraCharacter.cpp, CharacterView.java and the wardrobe tiles) read both.
"""
import colorsys

RES = 3
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


# ---------------------------------------------------------------------------------------------- 1. Scale3x
def scale3x(grid):
    """AdvMAME3x on a grid of characters; the outside counts as empty"""
    h, w = len(grid), len(grid[0]) if grid else 0

    def at(x, y):
        if 0 <= x < w and 0 <= y < h:
            return grid[y][x]
        return EMPTY

    out = [[EMPTY] * (w * 3) for _ in range(h * 3)]
    for y in range(h):
        for x in range(w):
            A, B, C = at(x - 1, y - 1), at(x, y - 1), at(x + 1, y - 1)
            D, E, F = at(x - 1, y), at(x, y), at(x + 1, y)
            G, H, I = at(x - 1, y + 1), at(x, y + 1), at(x + 1, y + 1)
            if B != H and D != F:
                e = [
                    D if D == B else E,
                    B if (D == B and E != C) or (B == F and E != A) else E,
                    F if B == F else E,
                    D if (D == B and E != G) or (D == H and E != A) else E,
                    E,
                    F if (B == F and E != I) or (H == F and E != C) else E,
                    D if D == H else E,
                    H if (D == H and E != I) or (H == F and E != G) else E,
                    F if H == F else E,
                ]
            else:
                e = [E] * 9
            for k in range(9):
                out[y * 3 + k // 3][x * 3 + k % 3] = e[k]
    return out


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
            k = -t
            hh = hue_toward(h, 0.70, 0.07 * k)          # shadows lean to blue-violet
            ss = min(1.0, s * (1 + 0.10 * k) + 0.03 * k)
            ll = l * (1 - 0.19 * k * contrast)
            if l < 0.12:
                ll = max(0.02, l * (1 - 0.30 * k))
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


def glint(color):
    """the brightest spot on metal or a gem: almost white, a little of the colour"""
    r, g, b, a = color
    return (min(255, int(r * 0.25 + 255 * 0.75)), min(255, int(g * 0.25 + 255 * 0.75)), min(255, int(b * 0.25 + 240 * 0.75)), a)


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
    """a layer of an accessory, finished: three times the resolution (a plain drawing is scaled up, one drawn at three
    times already, art/shapes.py, is kept), shaded, outlined, glinting, with relief"""
    if "relief" in layer:
        return layer  # finished already
    native = layer.get("res", 1) == RES
    rows = layer["rows"]
    width = max((len(r) for r in rows), default=0)
    if width == 0:
        return layer
    colors = layer["colors"]
    base = [[(ch if ch in colors and ch not in (".", " ") else EMPTY) for ch in r.ljust(width, ".")] for r in rows]
    # gems: small areas of a saturated colour (at most 4 skin pixels)
    rid0, sizes0 = regions(base)
    gem_limit = 4 * RES * RES if native else 4
    gem_cells = set()
    for y, row in enumerate(base):
        for x, ch in enumerate(row):
            if ch == EMPTY:
                continue
            r, g, b, a = parse(colors[ch])
            _, l, s = colorsys.rgb_to_hls(r / 255, g / 255, b / 255)
            if sizes0[rid0[y][x]] <= gem_limit and s > 0.55 and 0.25 < l < 0.85 and a > 200 and material(r, g, b, a) != "gold":
                gem_cells.add((x, y))

    grid = base if native else scale3x(base)
    h, w = len(grid), len(grid[0])
    mask = [[grid[y][x] != EMPTY for x in range(w)] for y in range(h)]
    dist = distance_to_edge(mask, 6)
    rid, sizes = regions(grid)
    if native:
        gem = [[(x, y) in gem_cells for x in range(w)] for y in range(h)]
    else:
        gem = [[(x // 3, y // 3) in gem_cells and grid[y][x] == base[y // 3][x // 3] for x in range(w)] for y in range(h)]

    # the box of the item: a soft light from above (the top lighter, the bottom darker)
    filled_rows = [y for y in range(h) if any(mask[y])]
    top, bottom = (filled_rows[0], filled_rows[-1]) if filled_rows else (0, 1)

    ramps = {ch: ramp(parse(colors[ch])) for ch in set(c for row in grid for c in row if c != EMPTY)}
    out_rows, relief_rows, palette = [], [], {}

    def code(rgba):
        key = to_hex(*rgba)
        if key not in palette:
            if len(palette) >= len(PALETTE_CHARS):
                # too many tones in one layer: take the nearest one already there
                def far(k):
                    pr, pg, pb, pa = parse(k)
                    return (pr - rgba[0]) ** 2 + (pg - rgba[1]) ** 2 + (pb - rgba[2]) ** 2 + (pa - rgba[3]) ** 2
                return palette[min(palette, key=far)]
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
            tones, mat = ramps[ch]

            def other(nx, ny):
                return not (0 <= nx < w and 0 <= ny < h) or grid[ny][nx] != ch

            # the form: the gradient of the distance to the edge, lit from the top left
            d = dist[y][x]
            dl = dist[y][x - 1] if x > 0 else 0
            dr = dist[y][x + 1] if x < w - 1 else 0
            du = dist[y - 1][x] if y > 0 else 0
            dd = dist[y + 1][x] if y < h - 1 else 0
            gx, gy = (dr - dl) / 2, (dd - du) / 2              # where the form rises
            light = (gx * 0.55 + gy * 0.85) * (1.0 if d < 5 else 0.4)
            # the soft light of the whole item: the top lighter than the bottom
            light += 0.25 - 0.5 * (y - top) / max(1, bottom - top)
            # a bevel where a material meets another one (or the edge): lit above and on the left, shaded below and right
            if other(x, y - 1) or other(x - 1, y):
                light += 0.55
            if other(x, y + 1) or other(x + 1, y):
                light -= 0.65
            tone = max(-2, min(2, round(light * 1.25)))
            # the selective outline: the edge of the silhouette a tone darker than its own colour
            edge = any(not (0 <= x + dx < w and 0 <= y + dy < h) or not mask[y + dy][x + dx]
                       for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
            if edge:
                lit = y == 0 or not mask[y - 1][x] or x == 0 or not mask[y][x - 1]
                tone = min(2, tone) if lit else max(-2, tone - 1)
            rgba = tones[tone + 2]
            # metal: a bright glint where the light is strongest; gems: a spark in the top left, deep in the bottom right
            if mat in ("gold", "steel") and tone == 2 and not edge and (x + y) % 5 == 0:
                rgba = glint(tones[4])
            if gem[y][x]:
                if native:
                    up_left = 0 < x and 0 < y and gem[y - 1][x] and gem[y][x - 1]
                    near_top_left = not (y > 1 and gem[y - 2][x]) and not (x > 1 and gem[y][x - 2]) and up_left
                    if near_top_left:
                        rgba = glint(tones[2])
                    elif not (y + 1 < h and gem[y + 1][x]) or not (x + 1 < w and gem[y][x + 1]):
                        rgba = tones[0]
                else:
                    gx3, gy3 = x % 3, y % 3
                    cell_top_left = (x // 3, y // 3)
                    left_gem = (cell_top_left[0] - 1, cell_top_left[1]) in gem_cells
                    up_gem = (cell_top_left[0], cell_top_left[1] - 1) in gem_cells
                    if not left_gem and not up_gem and gx3 == 0 and gy3 == 0:
                        rgba = glint(tones[2])
                    elif gx3 == 2 and gy3 == 2:
                        rgba = tones[0]
            line.append(code(rgba))
            # relief: thicker in the middle of the shape (0 at the edge .. 4 inside)
            rel.append(str(min(4, (d - 1) // 2)))
        out_rows.append("".join(line).rstrip("."))
        relief_rows.append("".join(rel).rstrip("."))
    finished = dict(layer)
    finished["rows"] = out_rows
    finished["relief"] = relief_rows
    finished["colors"] = {c: k for k, c in palette.items()}
    finished["res"] = RES
    return finished
