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

# the face, like the Warden in the game: no eyes, a lighter face plate with a fold under it,
# glowing bioluminescent patches on the sides and a wide dark mouth with jagged teeth and fangs
rect(L + 3, T + 4, R - 3, T + 8, "L")
rect(L + 5, T + 5, R - 5, T + 7, "H")
rect(L + 3, T + 9, R - 3, T + 9, "N")
for x0 in (L + 1, R - 2):
    rect(x0, T + 6, x0 + 1, T + 8, "C")
    put(x0 + (1 if x0 < W // 2 else 0), T + 7, "B")
rect(L + 3, T + 10, R - 3, T + 15, "J")
rect(L + 4, T + 12, R - 4, T + 13, "Q")
for i, x in enumerate(range(L + 4, R - 3)):
    put(x, T + 11, "T" if i % 2 == 0 else "U")  # upper teeth
    put(x, T + 14, "U" if i % 2 == 0 else "T")  # lower teeth, offset
for x in range(L + 4, R - 3, 3):
    put(x, T + 12, "T")  # fangs reaching down
rect(L + 8, T + 12, L + 9, T + 13, "O")  # a soul glowing deep in the throat

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
