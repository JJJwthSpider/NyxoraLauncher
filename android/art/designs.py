#!/usr/bin/env python3
"""The flagship pieces of the wardrobe. Each is a plain model, as a Minecraft model is: drawn pixel by pixel in the
skin's own pixels (crowns, hats, a mask, ears, amulets, swords, capes), or, where a curve matters more than each
pixel (long horns, a scythe, a staff), drawn with shapes and laid over the skin's pixel grid (art/shapes.py).
art/cosmetics.py places them; art/hd.py paints them in three tones.
"""
import math

from shapes import Sprite, curve, lerp

GOLD, GOLD_D, GOLD_L = "#E6B43C", "#9C6E1E", "#FFE08A"
RUBY, WHITE, BLACK = "#E8364F", "#F4EEF0", "#120306"


class Pixels:
    """an accessory drawn by hand: rows of characters, one per skin pixel ('.' is empty), and their colours"""

    def __init__(self, rows, colors):
        self.rows, self.colors = rows, {ch: c for ch, c in colors.items() if c}
        for row in rows:
            for ch in row:
                assert ch == "." or ch in colors, "no colour for '%s'" % ch
        # a character whose colour is None is left empty (a hat without stars)
        self.rows = ["".join(ch if ch in self.colors else "." for ch in row) for row in rows]

    def layer(self, x, y, view="front", anim=""):
        return {"x": x, "y": y, "rows": self.rows, "colors": self.colors, "view": view, "anim": anim}


# ---------------------------------------------------------------------------------------------- head
def crown(gold=GOLD, gold_dark=GOLD_D, side_gem=RUBY, mid_gem="#5CB6FF", pearl=None, spikes=False):
    """a crown: points (with pearls, or sharp spikes), a band with three gems; the renderers bend it round the head"""
    if spikes:
        rows = ["a...aa...a",
                "a.a.aa.a.a",
                "aaaaaaaaaa",
                "abaaccaaba",
                "dddddddddd"]
    elif pearl:
        rows = ["p.p.pp.p.p",
                "a.a.aa.a.a",
                "aaaaaaaaaa",
                "abaaccaaba",
                "dddddddddd"]
    else:
        rows = ["a..a..a..a",
                "a..a..a..a",
                "aaaaaaaaaa",
                "abaaccaaba",
                "dddddddddd"]
    return Pixels(rows, {"a": gold, "b": side_gem, "c": mid_gem, "d": gold_dark, "p": pearl})


def tiara(metal="#D8DEE6", gem="#7FD6FF", star_gem=WHITE):
    """a thin tiara rising to a bright stone in the middle"""
    return Pixels(["....ss....",
                   "..aassaa..",
                   "aagaaaagaa"], {"a": metal, "g": gem, "s": star_gem})


def laurel(leaf="#6BAA3E", leaf_dark="#3E7A2A", berry=RUBY):
    """a laurel wreath: leaves in two greens (or golds), berries at the front"""
    return Pixels(["a.d.a..a.d.a"[1:11],
                   "dadabbadad"], {"a": leaf, "d": leaf_dark, "b": berry})


def halo(gold=GOLD):
    """a halo: a thin band that the renderers bend into a ring over the head (as they do with crowns)"""
    return Pixels(["aaaaaaaaaa"], {"a": gold})


def horns(base, tip):
    """two short horns curving up from the sides of the head, the tips in another colour"""
    return Pixels([".t........t.",
                   "tb........bt",
                   "bb........bb",
                   "bbb......bbb",
                   ".bbb....bbb."], {"b": base, "t": tip})


def wizard_hat(main, shade, band, buckle=GOLD, star=GOLD_L):
    """a wizard's hat: a wide brim, a tall cone with the tip bent aside, a band with a buckle, a star"""
    return Pixels(["......aa....",
                   ".....aaa....",
                   ".....aaa....",
                   "....aaaa....",
                   "....asaa....",
                   "...aaaaaa...",
                   "...bbggbb...",
                   ".cccccccccc.",
                   "cccccccccccc"], {"a": main, "c": shade, "b": band, "g": buckle, "s": star})


def oni_mask(face="#C81F3A", dark="#5A0A18", tusk=WHITE, eye=GOLD):
    """a demon mask over the face: angry brows, gold eyes, a dark mouth with tusks"""
    return Pixels(["aaaaaaaa",
                   "addaadda",
                   "aeeaaeea",
                   "aaaddaaa",
                   "aaaaaaaa",
                   "atddddta",
                   "atddddta",
                   ".aaaaaa."], {"a": face, "d": dark, "t": tusk, "e": eye})


def mask_horns(color="#E8DCC8"):
    return Pixels(["a........a",
                   "aa......aa",
                   ".a......a."], {"a": color})


def pointy_ears(outer, inner, tip=None):
    """cat, fox or wolf ears: the fur inside lighter, the tips darker"""
    return Pixels([".t......t.",
                   "aaa....aaa",
                   "aba....aba",
                   "aba....aba"], {"a": outer, "b": inner, "t": tip or outer})


# ---------------------------------------------------------------------------------------------- body
def amulet(chain, chain_dark, setting, stone, shape="drop"):
    """a pendant on a chain: links down to the chest, a setting and a stone (a drop or a crystal)"""
    links = ["a....a",
             ".d..d.",
             "..aa.."]
    if shape == "crystal":
        rows = links + ["..bb..",
                        ".bbbb.",
                        ".bbbb.",
                        "..bb.."]
    else:
        rows = links + [".sbbs.",
                        ".sbbs.",
                        "..ss.."]
    return Pixels(rows, {"a": chain, "d": chain_dark, "s": setting, "b": stone})


def dragon_amulet(gold=GOLD, gold_dark=GOLD_D, eye=RUBY):
    """a gold pendant shaped as a dragon's head: horns, ruby eyes, a snout"""
    return Pixels(["a....a",
                   ".d..d.",
                   "..aa..",
                   ".a..a.",
                   ".aaaa.",
                   ".eaae.",
                   "..dd.."], {"a": gold, "d": gold_dark, "e": eye})


def core(rim="#3A3F48", plate="#7C8794", glow="#5CF0FF"):
    """a reactor core on the chest: a plate, a dark ring and a glowing heart"""
    return Pixels([".pppp.",
                   "prrrrp",
                   "prggrp",
                   "prggrp",
                   "prrrrp",
                   ".pppp."], {"p": plate, "r": rim, "g": glow})


# ---------------------------------------------------------------------------------------------- swords
def sword(edge, mid, hilt, guard, gem=None, kind="sword"):
    """a sword held up at an angle, the grip at the bottom left: a blade with a light edge, a crossguard (with a gem), a
    grip and a pommel. Kinds: a sword, a thin curved blade, a greatsword."""
    if kind == "curved":
        rows = ["........a.",
                "........ab",
                ".......ab.",
                "......ab..",
                ".....ab...",
                ".d..ab....",
                "..dab.....",
                "..cg......",
                ".c..d.....",
                "d........."]
    elif kind == "great":
        rows = ["..........ab",
                ".........abb",
                "........abbb",
                ".......abbb.",
                "......abbb..",
                ".....abbb...",
                ".d..abbb....",
                ".dd.bbb.....",
                "..ddgb......",
                "..cdd.......",
                ".c..dd......",
                "d....d......"]
    else:
        rows = ["........ab",
                ".......abb",
                "......abb.",
                ".....abb..",
                "....abb...",
                ".d.abb....",
                "..gbb.....",
                "..cd......",
                ".c..d.....",
                "d........."]
    return Pixels(rows, {"a": edge, "b": mid, "c": hilt, "d": guard, "g": gem or guard})


# ---------------------------------------------------------------------------------------------- capes
EMBLEMS = {
    "crown": ["b.bb.b",
              "bbbbbb",
              "bbbbbb"],
    "leaf": [".bb.",
             "bbbb",
             "bbbb",
             ".bb.",
             ".b.."],
    "eye": [".bbbb.",
            "bb..bb",
            ".bbbb."],
    "snow": ["..bb..",
             "b.bb.b",
             "bbbbbb",
             "bbbbbb",
             "b.bb.b",
             "..bb.."],
    "n": ["b..b",
          "bb.b",
          "b.bb",
          "b..b"],
    "diamond": [".bb.",
                "bbbb",
                "bbbb",
                ".bb."],
}


def cape(main, trim, dark, emblem=None, emblem_color=GOLD, ermine=False):
    """a cape seen from the back: a collar, a trimmed hem, an emblem between the shoulder blades; with ermine, a white
    fur collar and hem with black tails"""
    rows = ["cccccccccc"] + ["aaaaaaaaaa"] * 13 + ["cccccccccc", "dddddddddd", ".dddddddd."]
    if ermine:
        rows[0], rows[1] = "wwwwwwwwww", "wkwwkwwkww"
        rows[14], rows[15], rows[16] = "wwwwwwwwww", "wkwwkwwkww", ".wwwwwwww."
    if emblem:
        art = EMBLEMS[emblem]
        left = (10 - len(art[0])) // 2
        top = 3 + (6 - len(art)) // 2
        for j, line in enumerate(art):
            row = list(rows[top + j])
            for i, ch in enumerate(line):
                if ch != ".":
                    row[left + i] = "b"
            rows[top + j] = "".join(row)
    return Pixels(rows, {"a": main, "b": emblem_color, "c": trim, "d": dark, "w": WHITE, "k": BLACK})


# ---------------------------------------------------------------------------------------------- drawn with shapes
def long_horns(base, tip, ridge, spread=3.4, rise=6.6, back=1.6, thick=1.0):
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


def ram_horns(color, ridge, tip=None, face_top=2.2):
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
    # the horns curl beside the head, not over the face: what would lie on the face (x 4..12 of the figure, below
    # the top of the head) is cut away
    s.rect(4, face_top, 12, 7.5, None)
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
