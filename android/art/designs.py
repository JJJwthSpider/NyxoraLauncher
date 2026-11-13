#!/usr/bin/env python3
"""The flagship pieces of the wardrobe, drawn with shapes at three times the resolution (art/shapes.py): crowns with
set gems, curved horns, hats with a bent tip, swords with a fuller, a guard and a wrapped grip, staffs, scythes,
amulets, masks. art/cosmetics.py places them; art/hd.py shades them like the rest of the wardrobe.
"""
import math

from shapes import Sprite, curve, lerp, star

GOLD, GOLD_D, GOLD_L = "#E6B43C", "#9C6E1E", "#FFE08A"
RUBY = "#E8364F"


# ---------------------------------------------------------------------------------------------- head
def crown(gold=GOLD, gold_dark=GOLD_D, side_gem=RUBY, mid_gem="#5CB6FF", points=5, pearl=None, spikes=False):
    """a crown seen from the front: points with pearls (or sharp spikes), a band with gems in settings"""
    s = Sprite(10, 5.6)
    for k in range(points):
        cx = 0.9 + k * (8.2 / (points - 1))
        middle = k == points // 2
        tall = (3.6 if middle else 2.8 if k % 2 == 0 else 2.0) if spikes else (3.0 if middle else 2.4 if k % 2 == 0 else 1.9)
        half = 0.75 if spikes else 1.0
        s.poly([(cx - half, 3.3), (cx, 3.3 - tall), (cx + half, 3.3)], gold)
        if not spikes:
            s.ellipse(cx - 0.6, 3.3 - tall - 0.9, cx + 0.6, 3.3 - tall + 0.3, pearl or gold)
    s.rect(0, 3.0, 10, 5.6, gold)
    s.line([(0, 3.1), (10, 3.1)], gold_dark, 1 / 3)
    s.line([(0, 5.4), (10, 5.4)], gold_dark, 1 / 3)
    for k, gx in enumerate((1.8, 5.0, 8.2)):
        s.ellipse(gx - 0.95, 3.45, gx + 0.95, 5.15, gold_dark)
        s.ellipse(gx - 0.7, 3.65, gx + 0.7, 4.95, mid_gem if k == 1 else side_gem)
    for gx in (3.4, 6.6):
        s.ellipse(gx - 0.32, 4.0, gx + 0.32, 4.6, gold_dark)
    return s


def tiara(metal="#D8DEE6", metal_dark="#7C8794", gem="#7FD6FF", star_gem="#FFFFFF"):
    """a thin tiara: a band rising to a star in the middle, small gems along it"""
    s = Sprite(10, 4.6)
    s.poly([(0, 3.6), (3.0, 2.8), (5.0, 1.6), (7.0, 2.8), (10, 3.6), (10, 4.4), (7.0, 3.7), (5.0, 3.0), (3.0, 3.7), (0, 4.4)], metal)
    s.line([(0, 4.3), (3.0, 3.6), (5.0, 2.9), (7.0, 3.6), (10, 4.3)], metal_dark, 1 / 3)
    s.poly(star(5.0, 1.4, 1.5, 0.6), metal)
    s.poly(star(5.0, 1.4, 0.9, 0.38), star_gem)
    for gx in (2.0, 3.6, 6.4, 8.0):
        gy = 3.5 if gx in (2.0, 8.0) else 3.0
        s.ellipse(gx - 0.42, gy - 0.4, gx + 0.42, gy + 0.4, gem)
    return s


def laurel(leaf_color="#6BAA3E", leaf_dark="#3E7A2A", berry=RUBY, stem=None):
    """a laurel wreath: two branches of pointed leaves rising from the back to meet at the front, berries between them"""
    s = Sprite(10, 4.2)
    stem = stem or leaf_dark
    s.line([(0.3, 3.0), (2.6, 3.6), (5.0, 3.8), (7.4, 3.6), (9.7, 3.0)], stem, 0.45)
    for k in range(6):
        t = k / 5
        for side in (-1, 1):
            x = 5.0 + side * (0.5 + 4.2 * t)
            y = 3.75 - 0.75 * t * t
            # a leaf above the stem leaning to the front, one below leaning out
            for up, tilt in ((1, 30), (-1, 62)):
                a = math.radians(-90 + side * tilt) if up > 0 else math.radians(90 - side * tilt)
                length = 1.7 if up > 0 else 1.1
                bx, by = x, y - 0.1 * up
                tipx, tipy = bx + math.cos(a) * length, by + math.sin(a) * length
                nx, ny = -math.sin(a) * 0.42, math.cos(a) * 0.42
                mx, my = bx + math.cos(a) * length * 0.45, by + math.sin(a) * length * 0.45
                s.poly([(bx, by), (mx + nx, my + ny), (tipx, tipy), (mx - nx, my - ny)], leaf_color if (k + (up > 0)) % 2 else leaf_dark)
    for bx in (3.7, 6.3):
        s.ellipse(bx - 0.38, 3.3, bx + 0.38, 4.06, berry)
    return s


def horns(base, tip, ridge, spread=3.4, rise=6.6, back=1.6, thick=1.0):
    """two horns rising from the top of the head: out to the side, then the tip curling back in, thick at the root and
    ringed across"""
    s = Sprite(14, 7.2)
    for side in (-1, 1):
        root = (7.0 + side * 2.6, 7.0)
        ctrl = (root[0] + side * spread, root[1] - rise * 0.55)
        end = (root[0] + side * (spread - back), root[1] - rise)
        pts_out, pts_in = [], []
        for k in range(13):
            t = k / 12
            x = (1 - t) ** 2 * root[0] + 2 * (1 - t) * t * ctrl[0] + t * t * end[0]
            y = (1 - t) ** 2 * root[1] + 2 * (1 - t) * t * ctrl[1] + t * t * end[1]
            dx = 2 * (1 - t) * (ctrl[0] - root[0]) + 2 * t * (end[0] - ctrl[0])
            dy = 2 * (1 - t) * (ctrl[1] - root[1]) + 2 * t * (end[1] - ctrl[1])
            norm = math.hypot(dx, dy) or 1
            nx, ny = -dy / norm, dx / norm
            w = (1.35 * (1 - t) ** 0.9 + 0.08) * thick
            pts_out.append((x + nx * w, y + ny * w))
            pts_in.append((x - nx * w, y - ny * w))
        s.poly(pts_out + pts_in[::-1], base)
        s.poly(pts_out[8:] + pts_in[8:][::-1], tip)
        for k in (2, 4, 6):
            s.line([pts_out[k], pts_in[k]], ridge, 1 / 3)
    return s


def wizard_hat(main, shade, band, buckle=GOLD, stars=GOLD_L):
    """a wizard's hat: a wide brim, a tall cone whose tip bends back, a band with a buckle and small stars"""
    s = Sprite(14, 11.5)
    s.ellipse(0, 8.6, 14, 11.2, shade)
    s.ellipse(0.6, 8.4, 13.4, 10.4, main)
    cone = [(3.2, 9.4), (4.6, 5.8), (5.6, 3.2), (6.8, 1.4), (8.8, 0.2), (10.4, 0.9), (8.4, 1.6), (8.4, 3.4), (9.6, 6.0), (10.8, 9.4)]
    s.poly(cone, main)
    s.poly([(8.8, 0.2), (10.4, 0.9), (8.4, 1.6), (7.6, 1.2)], shade)
    s.poly([(3.6, 7.8), (10.4, 7.8), (10.8, 9.3), (3.2, 9.3)], band)
    s.rect(6.2, 7.6, 7.8, 9.5, buckle)
    s.rect(6.6, 8.0, 7.4, 9.1, band)
    if stars:
        for cx, cy, r in ((5.6, 5.4, 0.8), (8.0, 4.0, 0.6), (6.7, 2.6, 0.5)):
            s.poly(star(cx, cy, r, r * 0.42), stars)
    return s


def halo(gold=GOLD, gold_dark=GOLD_D, glow=GOLD_L):
    """a halo seen a little from above: a ring with a light inner rim and a darker underside"""
    s = Sprite(10, 3.2)
    s.ellipse(0, 0, 10, 3.2, gold_dark)
    s.ellipse(0, 0, 10, 2.7, gold)
    s.ellipse(0.9, 0.55, 9.1, 2.35, glow)
    s.ellipse(1.5, 0.9, 8.5, 2.25, None)
    return s


def ram_horns(color, ridge, tip=None):
    """ram horns: a thick ridged horn curling round beside each ear"""
    s = Sprite(16, 7.5)
    for side in (-1, 1):
        cx, cy = 8 + side * 5.3, 3.6
        outer, inner = [], []
        for k in range(17):
            t = k / 16
            a = math.radians(-100 + 330 * t) if side > 0 else math.radians(-80 - 330 * t)
            r = 3.3 - 2.2 * t
            w = 0.95 - 0.55 * t
            x, y = cx + math.cos(a) * r, cy + math.sin(a) * r
            outer.append((x + math.cos(a) * w, y + math.sin(a) * w))
            inner.append((x - math.cos(a) * w, y - math.sin(a) * w))
        s.poly(outer + inner[::-1], color)
        for k in range(2, 15, 2):
            s.line([outer[k], inner[k]], ridge, 1 / 3)
        if tip:
            s.poly(outer[13:] + inner[13:][::-1], tip)
    return s


EMBLEMS = {
    "crown": lambda s, cx, cy, c: s.poly([(cx - 1.6, cy + 1.0), (cx - 1.6, cy - 0.6), (cx - 0.8, cy + 0.1), (cx, cy - 1.1), (cx + 0.8, cy + 0.1),
                                          (cx + 1.6, cy - 0.6), (cx + 1.6, cy + 1.0)], c),
    "leaf": lambda s, cx, cy, c: (s.poly(curve([(cx, cy - 1.6), (cx + 1.1, cy - 0.2), (cx, cy + 1.4), (cx - 1.1, cy - 0.2)], 5), c),
                                  s.line([(cx, cy + 1.9), (cx, cy - 1.0)], c, 1 / 3)),
    "eye": lambda s, cx, cy, c: (s.poly(curve([(cx - 1.8, cy), (cx, cy - 1.0), (cx + 1.8, cy), (cx, cy + 1.0)], 4), c),
                                 s.ellipse(cx - 0.5, cy - 0.5, cx + 0.5, cy + 0.5, None)),
    "snow": lambda s, cx, cy, c: [s.line([(cx - math.cos(a) * 1.6, cy - math.sin(a) * 1.6), (cx + math.cos(a) * 1.6, cy + math.sin(a) * 1.6)], c, 0.34)
                                  for a in (math.pi / 2, math.pi / 6, -math.pi / 6)],
    "n": lambda s, cx, cy, c: (s.rect(cx - 1.6, cy - 1.5, cx - 0.8, cy + 1.5, c), s.rect(cx + 0.8, cy - 1.5, cx + 1.6, cy + 1.5, c),
                               s.poly([(cx - 1.6, cy - 1.5), (cx - 0.7, cy - 1.5), (cx + 1.6, cy + 1.5), (cx + 0.7, cy + 1.5)], c)),
    "v": lambda s, cx, cy, c: s.poly([(cx - 1.7, cy - 1.2), (cx - 0.8, cy - 1.2), (cx, cy + 0.4), (cx + 0.8, cy - 1.2), (cx + 1.7, cy - 1.2), (cx, cy + 1.6)], c),
    "diamond": lambda s, cx, cy, c: s.poly([(cx, cy - 1.5), (cx + 1.1, cy), (cx, cy + 1.5), (cx - 1.1, cy)], c),
}


def cape(main, trim, dark, emblem=None, emblem_color=GOLD, ermine=False):
    """a cape seen from the back: a collar, folds falling from the shoulders, a trimmed hem, an emblem between the shoulder
    blades; with ermine, a white fur trim with black tails"""
    s = Sprite(11, 17.2)
    body = [(0.5, 0.0), (10.5, 0.0), (10.9, 15.6)] + [(10.9 - 10.9 * k / 8, 15.8 + (0.9 if k % 2 else 0.0)) for k in range(9)] + [(0.1, 15.6)]
    s.poly(body, main)
    # folds: from the shoulders down, darker valleys
    for fx in (2.4, 5.5, 8.6):
        s.line([(fx, 3.0), (fx + (fx - 5.5) * 0.12, 16.2)], dark, 0.34)
    # the trim along the sides and the hem
    border = 0.55 if not ermine else 1.0
    s.inside(lambda sp: (sp.rect(0, 0, border + 0.4, 18, trim), sp.rect(11 - border - 0.4, 0, 11, 18, trim), sp.rect(0, 15.0 - border, 11, 18, trim)))
    # the collar
    s.poly([(0.3, 0.0), (10.7, 0.0), (10.2, 1.7), (5.5, 2.3), (0.8, 1.7)], trim)
    if ermine:
        for ex, ey in ((0.8, 4.0), (0.8, 9.0), (10.2, 6.5), (10.2, 11.5), (2.2, 16.0), (5.5, 16.0), (8.8, 16.0), (3.0, 1.0), (8.0, 1.0)):
            s.poly([(ex - 0.3, ey - 0.5), (ex + 0.3, ey - 0.5), (ex, ey + 0.6)], "#120306")
    if emblem:
        cx, cy = 5.5, 6.0
        s.ellipse(cx - 2.4, cy - 2.4, cx + 2.4, cy + 2.4, trim)
        s.ellipse(cx - 1.95, cy - 1.95, cx + 1.95, cy + 1.95, dark)
        EMBLEMS[emblem](s, cx, cy, emblem_color)
    return s


def oni_mask(face="#C81F3A", dark="#5A0A18", tusk="#F4EEF0", gold=GOLD):
    """a demon mask over the face: horns, fierce brows, gold eyes, tusks"""
    s = Sprite(8, 8.6)
    s.poly(curve([(0.3, 1.6), (4.0, 0.8), (7.7, 1.6), (7.8, 5.2), (6.4, 8.2), (4.0, 8.5), (1.6, 8.2), (0.2, 5.2)], 4), face)
    for side in (-1, 1):
        cx = 4.0 + side * 2.0
        s.poly([(cx - 0.8, 2.6 + side * 0.2), (cx + 0.9, 2.6 - side * 0.2), (cx + side * 0.2, 3.6)], dark)
        s.ellipse(cx - 0.75, 3.4, cx + 0.75, 4.6, gold)
        s.ellipse(cx - 0.25, 3.7, cx + 0.25, 4.3, dark)
        s.poly([(cx - side * 0.2, 6.4), (cx + side * 0.6, 6.4), (cx + side * 0.4, 8.0)], tusk)
    s.poly([(2.0, 6.0), (6.0, 6.0), (5.4, 7.0), (2.6, 7.0)], dark)
    s.poly([(3.4, 4.6), (4.6, 4.6), (4.0, 5.6)], dark)
    return s


def mask_horns(color="#E8DCC8", dark="#9C8A6A"):
    s = Sprite(12, 4)
    for side in (-1, 1):
        x = 6 + side * 3.6
        s.poly([(x - side * 0.8, 4.0), (x + side * 0.8, 4.0), (x + side * 2.2, 0.4)], color)
        s.line([(x + side * 0.2, 3.0), (x + side * 0.8, 3.0)], dark, 1 / 3)
    return s


def pointy_ears(outer, inner, tip=None, tall=4.4):
    """fox, cat or wolf ears: curved triangles with a tuft of lighter fur inside, the tips darker"""
    s = Sprite(10, tall)
    for cx, side in ((2.0, -1), (8.0, 1)):
        top = (cx + side * 0.3, 0.0)
        s.poly(curve([(cx - 1.9, tall), (cx - 1.3 + side * 0.3, tall * 0.45), top, (cx + 1.3 + side * 0.3, tall * 0.45), (cx + 1.9, tall)], 4), outer)
        s.poly([(cx - 0.85, tall), (cx + side * 0.2, tall * 0.38), (cx + 0.85, tall)], inner)
        if tip:
            s.inside(lambda sp: sp.rect(cx - 2, 0, cx + 2, tall * 0.24, tip))
    return s


# ---------------------------------------------------------------------------------------------- body
def amulet(chain, chain_dark, setting, stone, shape="round"):
    """a pendant on a chain: links down to the chest, a setting and a stone (round, a drop or a crystal)"""
    s = Sprite(7, 8)
    for k in range(7):
        t = k / 6
        for side in (-1, 1):
            x = 3.5 + side * (3.2 - 2.6 * t)
            y = 0.4 + 3.2 * t
            s.ellipse(x - 0.38, y - 0.3, x + 0.38, y + 0.3, chain if k % 2 else chain_dark)
    if shape == "crystal":
        s.poly([(3.5, 3.4), (4.9, 4.9), (3.5, 7.9), (2.1, 4.9)], setting)
        s.poly([(3.5, 3.9), (4.4, 5.0), (3.5, 7.2), (2.6, 5.0)], stone)
    elif shape == "drop":
        s.poly(curve([(3.5, 3.5), (5.0, 5.6), (3.5, 7.8), (2.0, 5.6)], 5), setting)
        s.poly(curve([(3.5, 4.2), (4.5, 5.7), (3.5, 7.1), (2.5, 5.7)], 5), stone)
    else:
        s.ellipse(1.6, 3.6, 5.4, 7.4, setting)
        s.ellipse(2.3, 4.3, 4.7, 6.7, stone)
    return s


def dragon_amulet(gold=GOLD, gold_dark=GOLD_D, eye=RUBY):
    """a gold pendant shaped as a coiled dragon's head with a ruby eye"""
    s = amulet(gold, gold_dark, gold_dark, gold)
    s.poly(curve([(2.0, 4.6), (3.6, 3.8), (5.4, 4.4), (5.0, 6.2), (3.6, 7.6), (2.2, 6.6)], 4), gold)
    s.poly([(3.0, 4.4), (5.6, 3.6), (4.6, 5.0)], gold_dark)
    s.ellipse(3.6, 5.0, 4.5, 5.8, eye)
    s.line([(2.4, 6.4), (3.6, 7.0), (4.8, 6.0)], gold_dark, 1 / 3)
    return s


def core(rim="#3A3F48", plate="#7C8794", glow="#5CF0FF"):
    """a reactor core on the chest: a plate, a ring and a glowing heart"""
    s = Sprite(6, 6)
    s.poly([(1.0, 0.2), (5.0, 0.2), (5.8, 1.0), (5.8, 5.0), (5.0, 5.8), (1.0, 5.8), (0.2, 5.0), (0.2, 1.0)], plate)
    s.ellipse(0.9, 0.9, 5.1, 5.1, rim)
    s.ellipse(1.5, 1.5, 4.5, 4.5, glow)
    s.ellipse(2.4, 2.4, 3.6, 3.6, "#E8FFFF")
    for a in range(0, 360, 90):
        x, y = 3 + math.cos(math.radians(a + 45)) * 2.5, 3 + math.sin(math.radians(a + 45)) * 2.5
        s.ellipse(x - 0.3, y - 0.3, x + 0.3, y + 0.3, rim)
    return s


# ---------------------------------------------------------------------------------------------- in the hand
def sword(edge, mid, hilt, guard, gem=None, length=12.0, width=1.7, curved=False):
    """a sword held up at an angle: a blade with a light edge, a darker bevel and a fuller, a crossguard with a gem,
    a wrapped grip and a pommel; the grip at the bottom left"""
    s = Sprite(12, 12)
    grip0, base = (1.2, 10.8), (3.4, 8.6)
    d = (math.sqrt(0.5), -math.sqrt(0.5))
    n = (math.sqrt(0.5), math.sqrt(0.5))
    tip = (base[0] + d[0] * (length - 4.0), base[1] + d[1] * (length - 4.0))
    w = width / 2
    bend = 0.9 if curved else 0.0
    near_tip = (tip[0] - d[0] * 1.6 + n[0] * bend, tip[1] - d[1] * 1.6 + n[1] * bend)
    blade = [(base[0] + n[0] * w, base[1] + n[1] * w), (near_tip[0] + n[0] * w, near_tip[1] + n[1] * w), tip,
             (near_tip[0] - n[0] * w, near_tip[1] - n[1] * w), (base[0] - n[0] * w, base[1] - n[1] * w)]
    s.poly(blade, mid)
    # the upper half of the blade catches the light
    s.poly([(base[0] - n[0] * w, base[1] - n[1] * w), (near_tip[0] - n[0] * w, near_tip[1] - n[1] * w), tip, near_tip, base], edge)
    # the fuller along the middle
    s.line([lerp(base, tip, 0.06), lerp(base, tip, 0.62)], mid, 0.34)
    # the crossguard
    g = 2.0
    s.poly([(base[0] + n[0] * g - d[0] * 0.3, base[1] + n[1] * g - d[1] * 0.3), (base[0] + n[0] * g + d[0] * 0.5, base[1] + n[1] * g + d[1] * 0.5),
            (base[0] - n[0] * g + d[0] * 0.5, base[1] - n[1] * g + d[1] * 0.5), (base[0] - n[0] * g - d[0] * 0.3, base[1] - n[1] * g - d[1] * 0.3)], guard)
    if gem:
        s.ellipse(base[0] - 0.55, base[1] - 0.55, base[0] + 0.55, base[1] + 0.55, gem)
    # the grip, wrapped, and the pommel
    s.line([base, grip0], hilt, 0.95)
    for k in range(1, 4):
        p = lerp(base, grip0, k / 4)
        s.line([(p[0] - n[0] * 0.45, p[1] - n[1] * 0.45), (p[0] + n[0] * 0.45, p[1] + n[1] * 0.45)], guard, 1 / 3)
    s.ellipse(grip0[0] - 0.75, grip0[1] - 0.75, grip0[0] + 0.75, grip0[1] + 0.75, guard)
    if gem:
        s.ellipse(grip0[0] - 0.35, grip0[1] - 0.35, grip0[0] + 0.35, grip0[1] + 0.35, gem)
    return s


def scythe(blade="#C9D4DE", blade_dark="#5A626C", shaft="#3A1A10", band=GOLD):
    """a reaper's scythe: a long shaft and a curved blade at the top"""
    s = Sprite(12, 14)
    s.line([(1.4, 13.6), (8.2, 0.8)], shaft, 0.8)
    for t in (0.25, 0.6):
        p = lerp((1.4, 13.6), (8.2, 0.8), t)
        s.line([(p[0] - 0.5, p[1] - 0.25), (p[0] + 0.5, p[1] + 0.25)], band, 0.4)
    blade_pts = curve([(8.0, 1.0), (4.6, 0.2), (0.6, 2.0), (2.6, 2.1), (5.6, 2.2), (8.4, 2.6)], 6)
    s.poly(blade_pts, blade)
    s.line([(8.2, 2.4), (4.6, 1.8), (1.2, 2.2)], blade_dark, 1 / 3)
    return s


def staff(wood="#5A3420", wood_dark="#3A1A10", crystal="#7FD6FF", claw=GOLD):
    """a staff with a floating crystal held by gold claws"""
    s = Sprite(9, 15)
    s.line([(1.6, 14.6), (6.0, 4.4)], wood, 0.85)
    s.line([(2.3, 13.0), (2.8, 11.8)], wood_dark, 0.5)
    s.line([(4.6, 7.6), (5.0, 6.6)], wood_dark, 0.5)
    for side in (-1, 1):
        s.line([(6.0, 4.6), (6.0 + side * 1.4, 3.2), (6.0 + side * 0.9, 1.6)], claw, 0.4)
    s.poly([(6.0, 0.2), (7.2, 2.2), (6.0, 4.0), (4.8, 2.2)], crystal)
    s.poly([(6.0, 0.6), (6.6, 2.1), (6.0, 2.6)], "#E8FFFF")
    return s
