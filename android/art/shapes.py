#!/usr/bin/env python3
"""Drawing accessories at three times the resolution of a skin pixel with shapes: polygons, ellipses and lines, for the
pieces a drawing of a few pixels cannot carry (feathered wings, membranes on bones, crowns with set gems, swords with a
fuller and a wrapped grip...). A Sprite is measured in skin pixels; its layer has "res": 3 and art/hd.py shades it the
same way as the rest of the wardrobe (without scaling it up again).
"""
import math

from PIL import Image, ImageDraw

import hd

RES = hd.RES


class Sprite:
    def __init__(self, width, height):
        self.width, self.height = width, height
        self.img = Image.new("L", (round(width * RES), round(height * RES)), 0)
        self.draw = ImageDraw.Draw(self.img)
        self.palette = {}

    def ink(self, color):
        if color is None:
            return 0  # erases
        if color not in self.palette:
            self.palette[color] = len(self.palette) + 1
            assert len(self.palette) < len(hd.PALETTE_CHARS), "too many colours in one sprite"
        return self.palette[color]

    @staticmethod
    def at(points):
        return [(x * RES, y * RES) for x, y in points]

    def poly(self, points, color):
        self.draw.polygon(self.at(points), fill=self.ink(color))

    def rect(self, x0, y0, x1, y1, color):
        self.draw.rectangle([x0 * RES, y0 * RES, x1 * RES - 1, y1 * RES - 1], fill=self.ink(color))

    def ellipse(self, x0, y0, x1, y1, color):
        self.draw.ellipse([x0 * RES, y0 * RES, x1 * RES - 1, y1 * RES - 1], fill=self.ink(color))

    def line(self, points, color, width=1 / 3):
        self.draw.line(self.at(points), fill=self.ink(color), width=max(1, round(width * RES)), joint="curve")

    def inside(self, draw):
        """draws with `draw(sprite)` only over what is already drawn (veins inside a wing, a pattern on a cape)"""
        before = self.img.copy()
        draw(self)
        a, b = before.load(), self.img.load()
        for y in range(self.img.height):
            for x in range(self.img.width):
                if a[x, y] == 0:
                    b[x, y] = 0

    def outline(self, points, color, width=1 / 3):
        self.line(list(points) + [points[0]], color, width)

    def layer(self, x, y, view="front", anim="", mirror=False):
        """the sprite as a layer of an accessory, its top left corner at (x, y) skin pixels"""
        chars = hd.PALETTE_CHARS
        pixels = self.img.load()
        rows = []
        for yy in range(self.img.height):
            row = "".join("." if pixels[xx, yy] == 0 else chars[pixels[xx, yy] - 1] for xx in range(self.img.width))
            rows.append(row[::-1] if mirror else row)
        rows = [r.rstrip(".") for r in rows]
        return {"x": x, "y": y, "rows": rows, "colors": {chars[i - 1]: c for c, i in self.palette.items()}, "view": view, "anim": anim,
                "res": RES}


# ---------------------------------------------------------------------------------------------- geometry helpers
def lerp(a, b, t):
    return (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t)


def along(points, t):
    """a point at t (0..1) along a polyline"""
    lengths = [math.dist(points[i], points[i + 1]) for i in range(len(points) - 1)]
    total = sum(lengths)
    d = t * total
    for i, seg in enumerate(lengths):
        if d <= seg or i == len(lengths) - 1:
            return lerp(points[i], points[i + 1], min(1.0, d / seg if seg else 0))
        d -= seg
    return points[-1]


def leaf(base, angle, length, width, round_tip=0.85):
    """a feather (or a petal, a leaf): from its base along `angle` (radians, 0 = right, pi/2 = down)"""
    dx, dy = math.cos(angle), math.sin(angle)
    px, py = -dy, dx
    bx, by = base
    w = width / 2
    return [(bx + px * w * 0.7, by + py * w * 0.7),
            (bx + dx * length * 0.35 + px * w, by + dy * length * 0.35 + py * w),
            (bx + dx * length * round_tip + px * w * 0.75, by + dy * length * round_tip + py * w * 0.75),
            (bx + dx * length, by + dy * length),
            (bx + dx * length * round_tip - px * w * 0.75, by + dy * length * round_tip - py * w * 0.75),
            (bx + dx * length * 0.35 - px * w, by + dy * length * 0.35 - py * w),
            (bx - px * w * 0.7, by - py * w * 0.7)]


def star(cx, cy, outer, inner, points=5, turn=-math.pi / 2):
    out = []
    for k in range(points * 2):
        r = outer if k % 2 == 0 else inner
        a = turn + k * math.pi / points
        out.append((cx + math.cos(a) * r, cy + math.sin(a) * r))
    return out


# ---------------------------------------------------------------------------------------------- wings
def feather_wing(light, mid, dark, edge=None, span=15.0, height=16.0, primaries=8, secondaries=6, tip_color=None):
    """a feathered wing seen from behind, its root on the right (at the shoulder blade): the leading edge rises from the
    shoulder to the wrist (the highest point) and comes down to the tip; long primaries fan out from the hand, the
    longest at the tip, secondaries hang from the forearm, a row of coverts lies over their bases"""
    edge = edge or dark
    s = Sprite(span, height)
    shoulder, elbow, wrist, tip = (span - 0.4, 4.2), (span * 0.72, 1.5), (span * 0.47, 0.4), (span * 0.12, 3.0)
    # secondaries first (they are under the primaries): from the forearm, longer toward the wrist
    for k in range(secondaries):
        t = k / max(1, secondaries - 1)                   # 0 at the shoulder .. 1 at the wrist
        base = along([shoulder, elbow, wrist], 0.1 + 0.85 * t)
        shape = leaf(base, math.radians(100 + 8 * t), height * (0.22 + 0.36 * t), 2.3)
        s.poly(shape, mid if k % 2 == 0 else dark)
        s.outline(shape, edge)
    # primaries, the outermost first: from the hand, fanned from down at the wrist to out at the tip
    for k in reversed(range(primaries)):
        t = k / max(1, primaries - 1)                     # 0 at the wrist .. 1 at the tip
        base = along([wrist, tip], 0.05 + 0.9 * t)
        angle = math.radians(112 + 46 * t)
        length = height * (0.62 + 0.36 * t)
        shape = leaf(base, angle, length, 2.0)
        s.poly(shape, tip_color if (tip_color and k % 2 == 0) else (mid if k % 2 else dark))
        s.outline(shape, edge)
    # coverts: a row of short rounded feathers over the bases, from the shoulder to the tip
    arm = [shoulder, elbow, wrist, tip]
    for k in range(11):
        t = 0.03 + 0.92 * k / 10
        base = along(arm, t)
        shape = leaf((base[0], base[1] + 0.4), math.radians(96 + 40 * t), 2.8 - 0.9 * t, 1.6, 0.7)
        s.poly(shape, light)
        s.outline(shape, mid)
    # the leading edge: the bone under the smallest, lightest feathers
    s.line(arm, light, 1.1)
    return s


def membrane_wing(skin, membrane, bone, claw, span=14.0, height=15.0, fingers=4, torn=False):
    """a bat, demon or dragon wing: an arm and long fingers as bones, a membrane stretched between them with scalloped
    edges, a claw at the wrist"""
    s = Sprite(span, height)
    shoulder, elbow, wrist = (span - 0.6, 3.4), (span * 0.66, 1.6), (span * 0.40, 1.1)
    tips = []
    for k in range(fingers):
        t = k / max(1, fingers - 1)
        a = math.radians(196 - 92 * t)                    # the first finger up and out, the last one down
        length = span * 0.42 + height * 0.42 * t + (span * 0.08 if k == 0 else 0)
        tips.append((wrist[0] + math.cos(a) * length, wrist[1] + math.sin(a) * length))
    tips.append((shoulder[0] - 0.6, shoulder[1] + height * 0.5))  # the membrane comes down to the body
    edge = [tips[0]]
    for k in range(len(tips) - 1):
        a, b = tips[k], tips[k + 1]
        mid = lerp(a, b, 0.5)
        to_wrist = (wrist[0] - mid[0], wrist[1] - mid[1])
        n = math.hypot(*to_wrist) or 1
        sag = (3.6 if k % 2 else 1.6) if torn else 2.9
        edge.append((mid[0] + to_wrist[0] / n * sag, mid[1] + to_wrist[1] / n * sag))
        edge.append(b)
    s.poly([shoulder, elbow, wrist] + edge + [shoulder], membrane)
    # the membrane near the arm in shade: the wing is cupped
    s.poly([shoulder, elbow, wrist, lerp(wrist, tips[1], 0.3), lerp(wrist, tips[-2], 0.3), lerp(shoulder, tips[-1], 0.35)], skin)
    # bones: the arm thick, the fingers thinner toward the tips
    s.line([shoulder, elbow, wrist], bone, 1.0)
    for tip in tips[:-1]:
        s.line([wrist, lerp(wrist, tip, 0.6)], bone, 0.7)
        s.line([lerp(wrist, tip, 0.6), tip], bone, 0.45)
    # the claw on the wrist and the thumb
    s.poly([(wrist[0] + 0.3, wrist[1] + 0.1), (wrist[0] - 0.9, wrist[1] - 1.3), (wrist[0] - 0.2, wrist[1] - 0.5), (wrist[0] + 0.9, wrist[1] - 0.6)], claw)
    return s


def curve(points, steps=6):
    """a smooth closed outline through the points (Catmull-Rom)"""
    out = []
    n = len(points)
    for i in range(n):
        p0, p1, p2, p3 = points[i - 1], points[i], points[(i + 1) % n], points[(i + 2) % n]
        for k in range(steps):
            t = k / steps
            t2, t3 = t * t, t * t * t
            out.append(tuple(0.5 * ((2 * p1[j]) + (-p0[j] + p2[j]) * t + (2 * p0[j] - 5 * p1[j] + 4 * p2[j] - p3[j]) * t2
                                    + (-p0[j] + 3 * p1[j] - 3 * p2[j] + p3[j]) * t3) for j in range(2)))
    return out


def insect_wing(edge_color, fill, inner, kind="butterfly", vein=None, spot=None):
    """the wings of a butterfly, a fairy, a bee or a dragonfly on one side: an upper and a lower wing from the root on
    the right, smooth outlines, veins from the root, spots"""
    vein = vein or edge_color
    if kind == "butterfly":
        s = Sprite(11, 13)
        root = (10.6, 4.2)
        upper = curve([root, (8.0, 1.2), (3.0, 0.3), (0.4, 1.6), (0.9, 5.2), (5.0, 6.0)])
        lower = curve([root, (5.5, 6.4), (2.4, 9.0), (3.6, 12.4), (7.2, 11.0), (9.6, 7.4)])
        for lobe in (lower, upper):
            s.poly(lobe, edge_color)
        s.poly(curve([(9.6, 4.2), (7.6, 2.0), (3.4, 1.4), (1.8, 2.2), (2.2, 4.6), (5.4, 5.2)]), fill)
        s.poly(curve([(9.4, 5.2), (5.8, 7.0), (3.6, 9.2), (4.4, 11.2), (7.0, 10.2), (8.8, 7.4)]), inner)
        if spot:
            s.ellipse(2.6, 2.2, 4.4, 4.0, spot)
            s.ellipse(5.4, 8.0, 6.8, 9.4, spot)
        for tip in ((2.0, 1.6), (1.4, 4.4), (4.4, 10.8), (7.4, 10.4)):
            s.line([root, tip], vein, 1 / 3)
        return s
    if kind == "fairy":
        s = Sprite(10, 14)
        root = (9.6, 5.0)
        s.poly(curve([root, (6.6, 2.2), (2.2, 0.2), (0.6, 1.2), (2.6, 4.4), (6.4, 6.0)]), fill)
        s.poly(curve([root, (6.8, 7.4), (3.4, 11.6), (2.6, 13.4), (4.6, 12.6), (8.2, 8.6)]), inner)
        for tip in ((1.4, 1.0), (3.6, 3.8), (3.2, 12.4), (5.6, 10.0)):
            s.line([root, tip], vein, 1 / 3)
        return s
    if kind == "bee":
        s = Sprite(7, 7)
        root = (6.6, 3.6)
        s.poly(curve([root, (4.4, 0.8), (1.2, 0.6), (0.6, 2.6), (3.2, 4.0)]), fill)
        s.poly(curve([root, (4.0, 4.6), (2.2, 6.4), (3.8, 6.8), (5.6, 5.4)]), inner)
        for tip in ((1.2, 1.6), (2.8, 6.2)):
            s.line([root, tip], vein, 1 / 3)
        return s
    # a dragonfly: two long narrow wings, almost level, with a net of veins
    s = Sprite(13, 8)
    root = (12.6, 3.6)
    s.poly(curve([root, (8.0, 1.6), (2.0, 0.6), (0.4, 1.4), (2.4, 2.8), (8.0, 3.4)]), fill)
    s.poly(curve([root, (8.0, 4.4), (2.4, 5.2), (0.8, 6.4), (3.0, 7.0), (8.4, 5.8)]), inner)
    def veins(sp):
        for x in range(2, 12, 2):
            sp.line([(x, 0.0), (x - 0.4, 3.4)], vein, 1 / 3)
            sp.line([(x, 4.0), (x - 0.6, 7.6)], vein, 1 / 3)
        sp.line([root, (1.0, 1.4)], vein, 1 / 3)
        sp.line([root, (1.4, 6.4)], vein, 1 / 3)
    s.inside(veins)
    return s
