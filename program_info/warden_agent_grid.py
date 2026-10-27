"""Builds the detailed 30x26 Warden head of the AI agent. Run it and paste the printed rows into WARDEN in
warden_icon.py and s_head in launcher/ui/widgets/WardenWidget.cpp."""
W, H = 30, 26
g = [["."] * W for _ in range(H)]

def put(x, y, c):
    if 0 <= x < W and 0 <= y < H:
        g[y][x] = c

def rect(x0, y0, x1, y1, c):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            put(x, y, c)

# head: skin with a lit top and a shaded bottom and sides
L, R, T, B = 6, 23, 8, 25  # 18 wide, 18 tall
rect(L, T, R, B, "M")
rect(L + 1, T + 1, R - 1, T + 2, "L")
rect(L + 3, T + 1, R - 3, T + 1, "H")          # highlight on the crown
rect(L, T, L + 1, B, "D"); rect(R - 1, T, R, B, "D")
rect(L + 2, B - 2, R - 2, B - 1, "D")
for x in range(L, R + 1):
    put(x, T - 1, "K"); put(x, B, "K")
for y in range(T, B + 1):
    put(L - 1, y, "K"); put(R + 1, y, "K")
put(L - 1, T - 1, "."); put(R + 1, T - 1, "."); put(L - 1, B, "."); put(R + 1, B, ".")
rect(L, B - 1, L, B - 1, "K"); rect(R, B - 1, R, B - 1, "K")  # rounded chin corners

# brow ridge and the deep eye sockets (the Warden is blind)
rect(L + 2, T + 4, R - 2, T + 4, "N")
for x0 in (L + 3, R - 7):
    rect(x0, T + 5, x0 + 4, T + 7, "S")
    rect(x0 + 1, T + 5, x0 + 3, T + 5, "X")    # rim of the socket catches some light
# nose bridge
rect(L + 8, T + 5, L + 9, T + 8, "L")

# glowing souls on the cheeks
for x0 in (L + 2, R - 3):
    rect(x0, T + 9, x0 + 1, T + 10, "C")
    put(x0, T + 9, "B")

# the mouth: jaws, two rows of teeth with shade, a glowing throat
mx0, mx1, my0, my1 = L + 4, R - 4, T + 10, T + 15
rect(mx0, my0, mx1, my1, "J")
rect(mx0 + 1, my0 + 2, mx1 - 1, my1 - 2, "Q")
rect(mx0 + 4, my0 + 2, mx1 - 4, my1 - 2, "O")  # soul glow deep in the throat
for x in range(mx0 + 1, mx1, 2):
    put(x, my0 + 1, "T"); put(x + 1, my0 + 1, "U" if x + 1 < mx1 else "J")
for x in range(mx0 + 2, mx1, 2):
    put(x, my1 - 1, "T"); put(x - 1, my1 - 1, "U")

# sculk veins creeping over the forehead
for x, y in ((L + 4, T + 2), (L + 5, T + 3), (L + 6, T + 3), (R - 5, T + 2), (R - 6, T + 3), (R - 4, T + 1)):
    put(x, y, "V")

# the horns: curved antlers that taper towards the tip: dark rim, body, glowing core, a bright tip
def horn(side):
    # (x, y, half width) from the base at the head up to the tip
    spine = [(8, 7, 2), (7, 6, 2), (6, 5, 2), (5, 4, 2), (5, 3, 1), (4, 2, 1), (4, 1, 1), (3, 0, 0)]
    m = (lambda x: W - 1 - x) if side else (lambda x: x)
    for i, (x, y, half) in enumerate(spine):
        for dx in range(-half - 1, half + 2):
            edge = abs(dx) == half + 1
            if edge and g[y][m(x + dx)] == ".":
                put(m(x + dx), y, "F")
            elif not edge:
                core = dx == 0 and i >= 2
                put(m(x + dx), y, ("W" if i >= 6 else "G") if core else "E")
    # a short side prong with a glowing tip
    for x, y, c in ((9, 4, "E"), (10, 3, "E"), (10, 2, "G"), (11, 4, "F"), (9, 3, "F")):
        put(m(x), y, c)
horn(0); horn(1)

rows = ["".join(r) for r in g]
if __name__ == "__main__":
    for r in rows:
        print(f'    "{r}",')
