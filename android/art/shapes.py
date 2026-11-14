#!/usr/bin/env python3
"""Drawing accessories with shapes: polygons, ellipses and lines, for the pieces whose outline a few hand-placed pixels
do not carry well (feathered wings, membranes on bones, curved horns, swords...). A Sprite is measured in skin pixels
and drawn three times finer; `layer` then gives it out in whole skin pixels of the figure, like every other accessory
(a pixel is filled when enough of it is covered, in the colour that covers most of it), so the shapes stay smooth in
outline and plain in detail, as a model made of cubes with a Minecraft texture is. art/hd.py shades it afterwards.
"""
import math

from PIL import Image, ImageDraw

import hd

RES = 3      # the drawing is three times finer than a skin pixel
COVER = 4    # of the 9 parts of a skin pixel, how many must be drawn for the pixel to be filled


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
        """the sprite as a layer of an accessory, its top left corner at (x, y) skin pixels of the figure. The layer is
        in whole skin pixels: (x, y) may be fractional, the drawing is then laid over the figure's pixel grid where it
        falls"""
        chars = hd.PALETTE_CHARS
        img = self.img.transpose(Image.FLIP_LEFT_RIGHT) if mirror else self.img
        ix, iy = math.floor(x), math.floor(y)
        sx, sy = int((x - ix) * RES + 0.5), int((y - iy) * RES + 0.5)   # the offset inside the first skin pixel
        pixels = img.load()
        cols, lines = -(-(img.width + sx) // RES), -(-(img.height + sy) // RES)
        rows = []
        for j in range(lines):
            row = []
            for i in range(cols):
                count = {}
                for v in range(RES):
                    for u in range(RES):
                        px, py = i * RES + u - sx, j * RES + v - sy
                        if 0 <= px < img.width and 0 <= py < img.height and pixels[px, py]:
                            count[pixels[px, py]] = count.get(pixels[px, py], 0) + 1
                if sum(count.values()) >= COVER:
                    # the colour that covers most of the pixel; of equals, the one drawn last (it lies on top)
                    row.append(chars[max(count, key=lambda c: (count[c], c)) - 1])
                else:
                    row.append(".")
            rows.append("".join(row).rstrip("."))
        return {"x": ix, "y": iy, "rows": rows, "colors": {chars[i - 1]: c for c, i in self.palette.items()}, "view": view, "anim": anim}


def mirrored(layer):
    """the twin of a layer on the other side of the figure (its axis is x = 8): the left wing of a right one"""
    width = max(len(r) for r in layer["rows"])
    return dict(layer, x=16 - layer["x"] - width, rows=[r.ljust(width, ".")[::-1].rstrip(".") for r in layer["rows"]])


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
def feather_wing(light, mid, dark, span=15.0, height=16.0, primaries=5, secondaries=4, tip_color=None):
    """a feathered wing seen from behind, its root on the right (at the shoulder blade): the leading edge rises from the
    shoulder to the wrist (the highest point) and comes down to the tip; long primaries fan out from the hand, the
    longest at the tip, secondaries hang from the forearm, small feathers lie over their bases. Painted plainly, in
    three bands: the small feathers light, the long ones in the middle tone, their ends dark (or `tip_color`)"""
    ends = tip_color or dark
    s = Sprite(span, height)
    shoulder, elbow, wrist, tip = (span - 0.4, 4.2), (span * 0.72, 1.5), (span * 0.47, 0.4), (span * 0.12, 3.0)

    # every feather whole in the colour of the ends, then shorter in the middle tone over it: the ends show only along
    # the fringe of the wing, as one clean scalloped band
    feathers = []
    for k in range(secondaries):                           # from the forearm, longer toward the wrist
        t = k / max(1, secondaries - 1)                   # 0 at the shoulder .. 1 at the wrist
        feathers.append((along([shoulder, elbow, wrist], 0.1 + 0.85 * t), math.radians(100 + 8 * t), height * (0.24 + 0.36 * t), 3.4))
    for k in range(primaries):                             # from the hand, fanned from down at the wrist to out at the tip
        t = k / max(1, primaries - 1)                     # 0 at the wrist .. 1 at the tip
        feathers.append((along([wrist, tip], 0.05 + 0.9 * t), math.radians(112 + 46 * t), height * (0.62 + 0.36 * t), 3.2))
    for base, angle, length, width in feathers:
        s.poly(leaf(base, angle, length, width), ends)
    for base, angle, length, width in feathers:
        s.poly(leaf(base, angle, length * 0.7, width + 0.6), mid)
    # the small feathers over the arm: one light band along the leading edge
    arm = [shoulder, elbow, wrist, tip]
    s.line([(x, y + 1.3) for x, y in arm], light, 3.0)
    s.line(arm, light, 1.4)
    return s


def membrane_wing(skin, membrane, bone, claw, span=14.0, height=15.0, fingers=3, torn=False):
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
    # the membrane next to the arm in shade: the wing is cupped
    s.inside(lambda sp: sp.line([(x, y + 1.4) for x, y in (shoulder, elbow, wrist)], skin, 1.6))
    # bones: the arm thick, the fingers thinner toward the tips
    s.line([shoulder, elbow, wrist], bone, 1.3)
    for tip in tips[:-1]:
        s.line([wrist, tip], bone, 0.9)
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
    the right, painted plainly (a rim, the wing, a spot)"""
    if kind == "butterfly":
        s = Sprite(11, 13)
        root = (10.6, 4.2)
        upper = curve([root, (8.0, 1.2), (3.0, 0.3), (0.4, 1.6), (0.9, 5.2), (5.0, 6.0)])
        lower = curve([root, (5.5, 6.4), (2.4, 9.0), (3.6, 12.4), (7.2, 11.0), (9.6, 7.4)])
        for lobe in (lower, upper):
            s.poly(lobe, edge_color)
        s.poly(curve([(9.4, 4.0), (7.6, 2.2), (3.4, 1.6), (2.0, 2.4), (2.4, 4.4), (5.4, 5.0)]), fill)
        s.poly(curve([(9.0, 5.6), (6.0, 7.2), (4.0, 9.2), (4.6, 11.0), (6.8, 10.0), (8.4, 7.6)]), inner)
        if spot:
            s.rect(3, 2, 5, 4, spot)
            s.rect(5, 8, 7, 10, spot)
        return s
    if kind == "fairy":
        s = Sprite(10, 14)
        root = (9.6, 5.0)
        s.poly(curve([root, (6.6, 2.2), (2.2, 0.2), (0.6, 1.2), (2.6, 4.4), (6.4, 6.0)]), fill)
        s.poly(curve([root, (6.8, 7.4), (3.4, 11.6), (2.6, 13.4), (4.6, 12.6), (8.2, 8.6)]), inner)
        return s
    if kind == "bee":
        s = Sprite(7, 7)
        root = (6.6, 3.6)
        s.poly(curve([root, (4.4, 0.8), (1.2, 0.6), (0.6, 2.6), (3.2, 4.0)]), fill)
        s.poly(curve([root, (4.0, 4.6), (2.2, 6.4), (3.8, 6.8), (5.6, 5.4)]), inner)
        return s
    # a dragonfly: two long narrow wings, almost level
    s = Sprite(13, 8)
    root = (12.6, 3.6)
    s.poly(curve([root, (8.0, 1.2), (2.0, 0.2), (0.4, 1.4), (2.4, 3.0), (8.0, 3.6)]), fill)
    s.poly(curve([root, (8.0, 4.4), (2.4, 5.0), (0.8, 6.4), (3.0, 7.4), (8.4, 6.2)]), inner)
    return s
