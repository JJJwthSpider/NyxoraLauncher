#!/usr/bin/env python3
"""The wardrobe of Vellora Client: every accessory as pixel art, written once and drawn by both apps.

Writes android/assets/cosmetics.json (Android: Cosmetics.java) and launcher/resources/sculk_theme/cosmetics.json
(desktop: VelloraCharacter.cpp). Coordinates are "skin pixels" of the 16x32 figure: the head is x 4..12, y 0..8, the
body x 4..12, y 8..20, the arms x 0..4 and 12..16, the legs y 20..32. Items may draw outside the figure.

A layer: x, y, rows (one character per pixel, ' ' and '.' empty), colors (character -> #RRGGBB or #RRGGBBAA),
view ("front", "back" or "both"), anim:
  bob    - floats up and down a little (halos)
  flap   - rises with the wing beat
  hop    - hops (pets on the ground)
Slots: head, body, back, pet, hand (in the right hand: it is at x 0..4, y 18..20), effect, emote.
  float  - floats slowly (flying pets)
  blink  - every second frame (bat wings)
An effect: particles {colors, count, motion: rise | fall | orbit | spark}.
Run:  python3 android/art/cosmetics.py
"""
import json
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
GOLD, GOLD_D, RUBY, BLACK, WHITE = "#E6B43C", "#9C6E1E", "#E8364F", "#120306", "#F4EEF0"

ITEMS = []


def item(id, slot, ru, en, rarity, layers=None, particles=None):
    d = {"id": id, "slot": slot, "title": ru, "titleEn": en, "rarity": rarity, "layers": layers or []}
    if particles:
        d["particles"] = particles
    # the wardrobe finds an item by its id alone: two items with one id would hide one of them
    assert all(other["id"] != id for other in ITEMS), "two items are called " + id
    ITEMS.append(d)


def L(x, y, rows, colors, view="front", anim=""):
    return {"x": x, "y": y, "rows": rows, "colors": colors, "view": view, "anim": anim}


def mirror(rows):
    return [r[::-1] for r in rows]


def wings(id, ru, en, rarity, light, mid, dark, shape=None):
    shape = shape or ["....aa", "..aabb", ".abbbc", "abbbcc", "abbcc.", "abcc..", ".bc...", ".c...."]
    colors = {"a": light, "b": mid, "c": dark}
    item(id, "back", ru, en, rarity, [L(-5, 6, shape, colors, "both", "flap"), L(15, 6, mirror(shape), colors, "both", "flap")])


# ---------------------------------------------------------------- head
item("crown", "head", "Корона", "Crown", "legendary",
     [L(3, -3, ["a..a..a..a", "aaaaaaaaaa", "abaacaabaa", "dddddddddd"], {"a": GOLD, "b": RUBY, "c": "#5CB6FF", "d": GOLD_D})])
item("horns", "head", "Рога демона", "Demon horns", "epic",
     [L(1, -5, ["b.........b", "a.........a", "aa.......aa", ".aa.....aa."], {"a": "#2A0A12", "b": "#7A0F22"}, "both")])
item("halo", "head", "Нимб", "Halo", "legendary",
     [L(3, -5, [".aaaaaaaa.", "a........a", ".bbbbbbbb."], {"a": GOLD, "b": GOLD_D}, "both", "bob")])
item("wizard_hat", "head", "Шляпа мага", "Wizard hat", "epic",
     [L(2, -9, ["......ab....", ".....aab....", ".....aabb...", "....aaabb...", "....aaabbb..", "...aaaabbb..", "...aaaaabbb.",
                "..aaaaaabbb.", "cccccccccccc", "dddddddddddd"], {"a": "#5A0A18", "b": "#8E1428", "c": GOLD, "d": BLACK}, "both")])
item("cat_ears", "head", "Кошачьи уши", "Cat ears", "rare",
     [L(3, -3, ["a........a", "aa......aa", "aba....aba"], {"a": "#1A0A0E", "b": "#E87A90"}, "both")])
item("bunny_ears", "head", "Заячьи уши", "Bunny ears", "rare",
     [L(4, -7, [".a....a.", ".ab..ab.", ".ab..ab.", ".ab..ab.", ".ab..ab.", ".aa..aa.", "..a..a.."], {"a": WHITE, "b": "#F2A7B8"}, "both")])
item("headphones", "head", "Наушники", "Headphones", "rare",
     [L(2, -1, [".aaaaaaaaaa.", ".a........a.", ".a........a.", ".a........a.", "bb........bb", "bb........bb", "bb........bb"],
        {"a": BLACK, "b": RUBY}, "both")])
item("shades", "head", "Тёмные очки", "Shades", "common",
     [L(4, 3, ["aaaaaaaa", ".abb.abb", ".aaa.aaa"], {"a": BLACK, "b": "#3A0D16"})])
item("eyepatch", "head", "Повязка пирата", "Eye patch", "common",
     [L(4, 2, ["a.......", ".a......", "..aaa...", "..aaa..a", ".......a"], {"a": BLACK})])
item("pirate_hat", "head", "Шляпа пирата", "Pirate hat", "epic",
     [L(1, -4, ["....aaaaaa....", "..aaaaaaaaaa..", ".aaaaabbaaaaa.", "aaaaaaaaaaaaaa", ".cccccccccccc."],
        {"a": "#1A0A0E", "b": WHITE, "c": GOLD}, "both")])
item("top_hat", "head", "Цилиндр", "Top hat", "rare",
     [L(3, -7, ["..aaaaaa..", "..aaaaaa..", "..aaaaaa..", "..aaaaaa..", "..bbbbbb..", "aaaaaaaaaa", "aaaaaaaaaa"],
        {"a": "#0E0507", "b": RUBY}, "both")])
item("viking_helmet", "head", "Шлем викинга", "Viking helmet", "epic",
     [L(1, -4, ["a............a", "aa..bbbbbb..aa", ".aabbbbbbbbaa.", "..bbccccccbb..", "..bbbbbbbbbb.."],
        {"a": "#E8DCC8", "b": "#7C8794", "c": "#5A626C"}, "both")])
item("flower_crown", "head", "Венок", "Flower crown", "rare",
     [L(3, -2, ["a.b.c.a.b.", "dddddddddd"], {"a": "#FF7AA2", "b": "#FFE08A", "c": "#B98CFF", "d": "#4C9A52"}, "both")])
item("beanie", "head", "Шапка", "Beanie", "common",
     [L(3, -3, ["....aa....", "..bbbbbb..", ".bbbbbbbb.", "cccccccccc"], {"a": WHITE, "b": "#8E1428", "c": "#5A0A18"}, "both")])
item("cap", "head", "Кепка", "Cap", "common",
     [L(3, -2, [".aaaaaaaa.", "aaaaaaaaaa", "bbbbbbbbbbbbb"], {"a": RUBY, "b": "#7A0F22"}, "both")])
item("devil_crown", "head", "Корона тьмы", "Crown of darkness", "legendary",
     [L(3, -4, ["a...a...a.", "aa.aaa.aa.", "aaaaaaaaaa", "abaaabaaab"], {"a": "#1A0A0E", "b": RUBY}, "both")])
item("mask", "head", "Маска", "Mask", "epic",
     [L(4, 2, ["aaaaaaaa", "abbaabba", "aaaaaaaa", ".aaaaaa."], {"a": "#E9E1E3", "b": BLACK})])

# ---------------------------------------------------------------- body
item("scarf", "body", "Шарф", "Scarf", "common",
     [L(4, 8, ["aaaaaaaa", "bbbbbbbb", ".....bb.", ".....bb.", ".....cc."], {"a": RUBY, "b": "#B01C38", "c": "#7A0F22"})])
item("chain", "body", "Золотая цепь", "Gold chain", "rare",
     [L(5, 9, ["a....a", ".a..a.", "..aa..", "..bb..", "..bb.."], {"a": GOLD, "b": RUBY})])
item("tie", "body", "Галстук", "Tie", "common", [L(7, 8, ["aa", "bb", "bb", "bb", "bb", "bb", "aa"], {"a": BLACK, "b": "#7A0F22"})])
item("heart_necklace", "body", "Сердце на цепочке", "Heart necklace", "rare",
     [L(5, 9, ["a....a", ".a..a.", "..aa..", ".b.b..", "bbbbb.", ".bbb..", "..b..."], {"a": "#C9C2C8", "b": RUBY})])
item("belt", "body", "Пояс", "Belt", "common", [L(4, 18, ["aaaaaaaa", "aaabbaaa"], {"a": "#2A0A12", "b": GOLD})])
item("armor", "body", "Нагрудник", "Chestplate", "epic",
     [L(4, 9, ["abbbbbba", "abccccba", "abccccba", "abbbbbba", ".abbbba."], {"a": "#5A626C", "b": "#9AA4AE", "c": RUBY})])
item("bow_tie", "body", "Бабочка", "Bow tie", "common", [L(6, 8, ["a.aa.a", "aaaaaa", "a.aa.a"], {"a": RUBY})])

# ---------------------------------------------------------------- back
wings("bat_wings", "Крылья летучей мыши", "Bat wings", "rare", "#4A1A26", "#2E1018", "#14070B")
wings("angel_wings", "Крылья ангела", "Angel wings", "legendary", WHITE, "#E8E2E6", "#C9C2C8")
wings("demon_wings", "Крылья демона", "Demon wings", "epic", RUBY, "#9E1630", "#5A0A18")
wings("dragon_wings", "Крылья дракона", "Dragon wings", "legendary", "#3A0D16", "#7A0F22", "#B01C38",
      ["...aaa", "..abbb", ".abbbc", "abbccc", "abbcc.", "abc.c.", "ab.c..", "a....."])
wings("butterfly_wings", "Крылья бабочки", "Butterfly wings", "epic", "#FF7AA2", "#B98CFF", "#5CB6FF",
      [".aa...", "abba..", "abbba.", ".abbb.", "..cc..", ".cccc.", ".cc...", "......"])
wings("fairy_wings", "Крылья феи", "Fairy wings", "rare", "#DDEFFF99", "#BCDCFF88", "#9AC0EE88")
item("crimson_cape", "back", "Багровая мантия", "Crimson mantle", "epic",
     [L(3, 8, ["cccccccccc", "aaaaaaaaaa", "aaaaaaaaaa", "aaaaabaaaa", "aaaabbaaaa", "aaaaaaaaaa", "aaaaaaaaaa", "aaaaaaaaaa", "aaaaaaaaaa",
               "aaaaaaaaaa", "aaaaaaaaaa", "aaaaaaaaaa", "aaaaaaaaaa", "aaaaaaaaaa", "aaaaaaaaaa", "dddddddddd", ".dddddddd."],
        {"a": "#7A0F22", "b": GOLD, "c": "#B01C38", "d": "#5A0A18"}, "back"),
      L(2, 9, ["a" + "." * 10 + "a"] * 15, {"a": "#7A0F22"}, "front")])
item("jetpack", "back", "Реактивный ранец", "Jetpack", "epic",
     [L(3, 9, ["aa....aa", "ab....ba", "ab....ba", "ab....ba", "aa....aa", "cc....cc", ".d....d."], {"a": "#5A626C", "b": "#9AA4AE", "c": "#FF8A3C", "d": "#FFE08A"}, "both", "bob")])
item("backpack", "back", "Рюкзак", "Backpack", "common",
     [L(4, 9, [".aaaaaa.", "abbbbbba", "abbccbba", "abbbbbba", "abbbbbba", ".aaaaaa."], {"a": "#3A1A10", "b": "#6E3A1E", "c": GOLD}, "back"),
      L(3, 9, ["a" + "......" + ".." + "a"] * 4, {"a": "#3A1A10"}, "front")])
item("sword_back", "back", "Меч за спиной", "Sword on the back", "rare",
     [L(2, 4, ["...........a", "..........ab", ".........ab.", "........ab..", ".......ab...", "......ab....", ".cc.ab......", "..cc........", ".c.cc.......", "c..........."],
        {"a": "#C9D4DE", "b": "#7C8794", "c": GOLD}, "both")])

# ---------------------------------------------------------------- pets (by the right foot, x 17..23)
item("cube_pet", "pet", "Тессеракт", "Tesseract", "legendary",
     [L(18, 24, [".aaa.", "abbba", "abcba", "abbba", ".aaa."], {"a": RUBY, "b": "#7A0F22", "c": "#FFC8CD"}, "both", "hop")])
item("bat_pet", "pet", "Летучая мышь", "Bat", "rare",
     [L(18, 14, ["a.....a", "aa.b.aa", ".abbba.", "..bcb.."], {"a": "#2E1018", "b": BLACK, "c": RUBY}, "both", "float")])
item("slime_pet", "pet", "Слизень", "Slime", "common",
     [L(18, 26, [".aaaa.", "abbbba", "abcbca", "abbbba", ".aaaa."], {"a": "#7A0F22", "b": "#C81F3A", "c": BLACK}, "both", "hop")])
item("ghost_pet", "pet", "Призрак", "Ghost", "rare",
     [L(18, 16, [".aaa.", "aaaaa", "abaab", "aaaaa", "aaaaa", "a.a.a"], {"a": "#F4EEF0CC", "b": BLACK}, "both", "float")])
item("fox_pet", "pet", "Лисёнок", "Fox", "epic",
     [L(17, 25, ["a...a...", "aa.aa...", "abbbb...", "acbcb..a", ".bbbbbaa", ".bbbbbb.", ".d..d..."], {"a": "#E06A1E", "b": "#F08A3C", "c": BLACK, "d": "#3A1A10"}, "both", "hop")])
item("cat_pet", "pet", "Котёнок", "Kitten", "rare",
     [L(17, 25, ["a.a.....", "aaa.....", "bab....a", "aaaaaaa.", ".aaaaa..", ".a...a.."], {"a": "#1A0A0E", "b": "#7CE07C"}, "both", "hop")])
item("axolotl_pet", "pet", "Аксолотль", "Axolotl", "epic",
     [L(17, 27, ["b.....b.", "baaaaab.", ".acaaca.", ".aaaaaaa", "..a..a.."], {"a": "#F6A3C0", "b": "#D9507A", "c": BLACK}, "both", "hop")])
item("dragon_pet", "pet", "Дракончик", "Little dragon", "legendary",
     [L(17, 13, ["a......a", "ab.cc.ba", "abcddcba", ".cccccc.", "..c..c..", ".c....c."], {"a": "#5A0A18", "b": "#9E1630", "c": RUBY, "d": GOLD}, "both", "float")])
item("bee_pet", "pet", "Пчела", "Bee", "common",
     [L(18, 15, [".aa.aa", ".aa.aa", "bcbcbc", "bcbcbd", ".cbcb."], {"a": "#DDEFFFAA", "b": "#FFD23C", "c": BLACK, "d": BLACK}, "both", "float")])
item("chick_pet", "pet", "Цыплёнок", "Chick", "common",
     [L(18, 27, [".aa..", "aaba.", "aaaac", ".aaa.", ".d.d."], {"a": "#FFE08A", "b": BLACK, "c": "#FF8A3C", "d": "#FF8A3C"}, "both", "hop")])

# ---------------------------------------------------------------- effects: particles around the player
item("flames", "effect", "Пламя", "Flames", "legendary", particles={"colors": ["#FF3B4F", "#FF8A3C", "#FFD23C"], "count": 18, "motion": "rise"})
item("hearts", "effect", "Сердечки", "Hearts", "rare", particles={"colors": ["#FF5A6E", "#FF8090"], "count": 8, "motion": "rise", "shape": "heart"})
item("snow", "effect", "Снегопад", "Snowfall", "rare", particles={"colors": ["#FFFFFF", "#DDEFFF"], "count": 22, "motion": "fall"})
item("sparkles", "effect", "Искры", "Sparkles", "epic", particles={"colors": ["#FFE08A", "#FFFFFF"], "count": 14, "motion": "spark"})
item("void_orbit", "effect", "Орбита пустоты", "Void orbit", "legendary", particles={"colors": ["#B01C38", "#E8364F", "#FFC8CD"], "count": 12, "motion": "orbit"})
item("blood_rain", "effect", "Кровавый дождь", "Blood rain", "epic", particles={"colors": ["#E8364F", "#9E1630"], "count": 20, "motion": "fall", "shape": "drop"})

# ---------------------------------------------------------------- emotes
for id, ru, en in [("wave", "Помахать", "Wave"), ("jump", "Прыжок", "Jump"), ("dance", "Танец", "Dance"), ("spin", "Вращение", "Spin"),
                   ("bow", "Поклон", "Bow"), ("cheer", "Ура!", "Cheer")]:
    item(id, "emote", ru, en, "common")

# ================================================================ more of everything (the wardrobe, part two)

def recolor(base, new_id, ru, en, rarity, colors):
    """the same shape as an item, other colours: {old -> new} by colour value"""
    src = next(i for i in ITEMS if i["id"] == base)
    layers = []
    for l in src["layers"]:
        layers.append(dict(l, colors={k: colors.get(v, v) for k, v in l["colors"].items()}))
    item(new_id, src["slot"], ru, en, rarity, layers, src.get("particles"))


# ---------------------------------------------------------------- head: hats
item("sombrero", "head", "Сомбреро", "Sombrero", "rare",
     [L(0, -4, ["......aaaa......", ".....abbbba.....", "....aaaaaaaa....", "aaaaccacacaaaaaa", ".aaaaaaaaaaaaaa."],
        {"a": "#D9A441", "b": "#B5832A", "c": RUBY}, "both")])
item("cowboy_hat", "head", "Ковбойская шляпа", "Cowboy hat", "rare",
     [L(1, -4, ["....aa..aa....", "...aaaaaaaa...", "...bbbbbbbb...", "a.aaaaaaaaaa.a", ".aaaaaaaaaaaa."], {"a": "#7A4A24", "b": "#3A1A10"}, "both")])
item("chef_hat", "head", "Колпак повара", "Chef hat", "common",
     [L(3, -6, [".aa.aa.aa.", "aaaaaaaaaa", "aaaaaaaaaa", ".aaaaaaaa.", ".aaaaaaaa.", "bbbbbbbbbb"], {"a": WHITE, "b": "#D6CCD0"}, "both")])
item("santa_hat", "head", "Новогодний колпак", "Santa hat", "rare",
     [L(3, -5, [".........aa", "......aaaab", "....aaaaa..", "..aaaaaaa..", "bbbbbbbbbb."], {"a": "#D6283A", "b": WHITE}, "both")])
item("party_hat", "head", "Праздничный колпак", "Party hat", "common",
     [L(5, -7, ["..c...", "..a...", ".aba..", ".abaa.", "abaaba", "aabaab"], {"a": "#5CB6FF", "b": "#FFE08A", "c": "#FF7AA2"}, "both")])
item("beret", "head", "Берет", "Beret", "common",
     [L(3, -2, ["....a.....", ".aaaaaaaa.", "aaaaaaaaab"], {"a": "#B01C38", "b": "#7A0F22"}, "both")])
item("graduation_cap", "head", "Шапка выпускника", "Graduation cap", "rare",
     [L(2, -3, ["aaaaaaaaaaaa", "..bbbbbbbb.c", "..bbbbbbbb.c"], {"a": "#1A1A22", "b": "#2A2A34", "c": GOLD}, "both")])
item("police_cap", "head", "Фуражка", "Officer cap", "rare",
     [L(3, -3, [".aaaaaaaa.", "aaaabbaaaa", "cccccccccc", "...ddddd.."], {"a": "#1E2A4A", "b": GOLD, "c": BLACK, "d": "#0E0E14"}, "both")])
item("propeller_cap", "head", "Кепка с пропеллером", "Propeller cap", "epic",
     [L(3, -5, ["aaa.c.bbb.", "....c.....", "..dedede..", ".dedededed", "eeeeeeeeee"], {"a": RUBY, "b": "#5CB6FF", "c": "#7C8794", "d": "#FFD23C", "e": "#2C5BD6"}, "both", "bob")])
item("samurai_helmet", "head", "Шлем самурая", "Samurai helmet", "legendary",
     [L(1, -5, ["...a......a...", "....a....a....", ".....abba.....", "..cccccccccc..", ".cccccccccccc.", "cc..........cc", "c............c"],
        {"a": GOLD, "b": RUBY, "c": "#2A0A12"}, "both")])
item("knight_helmet", "head", "Шлем рыцаря", "Knight helmet", "epic",
     [L(3, -3, ["....aa....", "....aa....", "bbbbbbbbbb"], {"a": RUBY, "b": "#9AA4AE"}, "both"),
      L(4, 0, ["cccccccc", "cccccccc", "cccccccc", "dddddddd", "c.c.c.c.", "cccccccc"], {"c": "#9AA4AE", "d": "#1A1A22"})])
item("pumpkin_head", "head", "Голова-тыква", "Pumpkin head", "epic",
     [L(7, -2, ["ab", ".b"], {"a": "#4C9A52", "b": "#2E6B32"}, "both"),
      L(4, 0, ["cccccccc", "cddccddc", "cddccddc", "cccccccc", "cdcddcdc", "ccddddcc", "cccccccc", "cccccccc"], {"c": "#E07A1E", "d": "#FFD23C"})])
item("creeper_mask", "head", "Маска крипера", "Creeper mask", "rare",
     [L(4, 0, ["aaaaaaaa", "abbaabba", "abbaabba", "aaabbaaa", "aabbbbaa", "aabbbbaa", "aabaabaa", "aaaaaaaa"], {"a": "#5BB04E", "b": "#13200F"})])
item("pilot_goggles", "head", "Очки пилота", "Pilot goggles", "rare",
     [L(3, 2, ["aaaaaaaaaa", "abbcaabbca", "abbbaabbba", "aaaaaaaaaa"], {"a": "#6E3A1E", "b": "#5CB6FF", "c": WHITE})])
item("vr_goggles", "head", "VR-шлем", "VR headset", "epic",
     [L(3, 2, ["aaaaaaaaaa", "abbbbbbbba", "abccbbccba", "aaaaaaaaaa"], {"a": "#1A1A22", "b": "#2C2C3A", "c": "#39FFD2"})])
item("glasses_3d", "head", "3D-очки", "3D glasses", "common", [L(4, 3, ["aaaaaaaa", ".bb..cc.", ".aa..aa."], {"a": WHITE, "b": "#E8364F", "c": "#3C8CFF"})])
item("heart_glasses", "head", "Очки-сердечки", "Heart glasses", "rare", [L(4, 3, ["aa.aa.aa", "aaaaaaaa", ".a.....a"], {"a": "#FF5A8A"})])
item("star_glasses", "head", "Очки-звёзды", "Star glasses", "rare", [L(4, 2, [".a....a.", "aaa..aaa", ".a....a."], {"a": "#FFD23C"})])
item("monocle", "head", "Монокль", "Monocle", "rare", [L(8, 3, [".aa.", "abba", ".aa.", "...a", "...a"], {"a": GOLD, "b": "#DDEFFF88"})])
item("nerd_glasses", "head", "Очки отличника", "Nerd glasses", "common", [L(4, 3, ["aaaaaaaa", "abbaabba", "aaa..aaa"], {"a": BLACK, "b": "#DDEFFF66"})])
item("mustache", "head", "Усы", "Mustache", "common", [L(4, 5, [".aa..aa.", "aaaaaaaa", "a......a"], {"a": "#3A1A10"})])
item("clown_nose", "head", "Нос клоуна", "Clown nose", "common", [L(7, 4, ["aa", "aa"], {"a": "#FF2A3C"})])
item("bandana_head", "head", "Бандана", "Bandana", "common",
     [L(3, -1, [".aaaaaaaa.", "aabaabaaba", "aaaaaaaaaa"], {"a": "#C62828", "b": WHITE}, "both")])
item("ninja_headband", "head", "Повязка ниндзя", "Ninja headband", "rare",
     [L(3, 1, ["aaaaaaaaaa", "abbbbbbbba"], {"a": "#1A1A22", "b": "#C62828"}, "front"),
      L(3, 1, ["aaaaaaaaaa", "aaaabbaaaa", "....aa....", "...a..a..."], {"a": "#C62828", "b": "#7A0F22"}, "back")])
item("fox_ears", "head", "Лисьи уши", "Fox ears", "rare", [L(3, -3, ["a........a", "ab......ba", "abb....bba"], {"a": "#E06A1E", "b": WHITE}, "both")])
item("wolf_ears", "head", "Волчьи уши", "Wolf ears", "rare", [L(3, -3, ["a........a", "aa......aa", "aba....aba"], {"a": "#7C8794", "b": "#3A3F48"}, "both")])
item("bear_ears", "head", "Медвежьи ушки", "Bear ears", "common", [L(3, -2, ["aa......aa", "ab......ba"], {"a": "#6E3A1E", "b": "#C08A5A"}, "both")])
item("antlers", "head", "Рога оленя", "Antlers", "epic",
     [L(1, -6, ["a.a......a.a", "aaa......aaa", ".a.a....a.a.", ".aa......aa.", "..a......a..", "..a......a.."], {"a": "#8A5A2E"}, "both")])
item("unicorn_horn", "head", "Рог единорога", "Unicorn horn", "legendary",
     [L(7, -5, ["..a", ".ab", ".ba", "bac", "abc"], {"a": "#FFE08A", "b": "#FF9AD5", "c": "#B98CFF"}, "both", "bob")])
item("ram_horns", "head", "Рога барана", "Ram horns", "epic",
     [L(2, -2, [".aa........aa.", "a..a......a..a", "a.a........a.a", ".a..........a."], {"a": "#D8C29D"}, "both")])
item("laurel_wreath", "head", "Лавровый венок", "Laurel wreath", "rare", [L(3, -1, ["a.a.a..a.a.a", "aaaaaaaaaa"], {"a": "#6BAA3E"}, "both")])
item("mushroom_hat", "head", "Шляпка гриба", "Mushroom cap", "rare",
     [L(2, -4, ["...aaaaaa...", "..abaaabaa..", ".aaaaaaaaaa.", "abaaaabaaaba"], {"a": "#D6283A", "b": WHITE}, "both")])
item("frog_hat", "head", "Шапка-лягушка", "Frog hat", "rare",
     [L(3, -4, [".ab....ab.", ".bb....bb.", "aaaaaaaaaa", "aaaaaaaaaa"], {"a": "#5BB04E", "b": BLACK}, "both")])
recolor("crown", "ice_crown", "Ледяная корона", "Ice crown", "legendary", {GOLD: "#BFE6FF", GOLD_D: "#5CB6FF", RUBY: WHITE})
recolor("crown", "emerald_crown", "Изумрудная корона", "Emerald crown", "epic", {GOLD: "#3EC27A", GOLD_D: "#1E7A48", RUBY: GOLD})
recolor("halo", "blue_halo", "Голубой нимб", "Blue halo", "epic", {GOLD: "#7FD6FF", GOLD_D: "#3C8CFF"})
recolor("halo", "crimson_halo", "Багровый нимб", "Crimson halo", "legendary", {GOLD: RUBY, GOLD_D: "#7A0F22"})
recolor("cat_ears", "white_cat_ears", "Белые ушки", "White cat ears", "rare", {"#1A0A0E": WHITE})
recolor("top_hat", "white_top_hat", "Белый цилиндр", "White top hat", "rare", {"#0E0507": WHITE, RUBY: "#2C5BD6"})
recolor("wizard_hat", "blue_wizard_hat", "Синяя шляпа мага", "Blue wizard hat", "epic", {"#5A0A18": "#1E2A6A", "#8E1428": "#3C4CB0"})

# ---------------------------------------------------------------- body
item("medal", "body", "Медаль", "Medal", "rare", [L(7, 8, ["ab", "ab", "cc", "cc"], {"a": RUBY, "b": "#3C8CFF", "c": GOLD})])
item("sash", "body", "Лента", "Sash", "epic",
     [L(4, 8, ["a.......", "aa......", ".aa.....", "..aa....", "...aa...", "....aa..", ".....aa.", "......aa", ".......a"], {"a": "#B01C38"})])
item("pearl_necklace", "body", "Жемчуг", "Pearls", "rare", [L(4, 8, ["a......a", ".a....a.", "..aaaa.."], {"a": WHITE})])
item("thick_chain", "body", "Толстая цепь", "Thick chain", "epic", [L(4, 8, ["aa....aa", ".aa..aa.", "..aaaa..", "...bb...", "...bb..."], {"a": GOLD, "b": "#FFE08A"})])
item("cross_necklace", "body", "Крестик", "Cross", "rare", [L(5, 9, ["a....a", ".a..a.", "..ab..", ".bbbb.", "..bb..", "..bb.."], {"a": "#C9C2C8", "b": GOLD})])
item("tesseract_amulet", "body", "Амулет тессеракта", "Tesseract amulet", "legendary",
     [L(5, 9, ["a....a", ".a..a.", "..aa..", ".bccb.", ".cddc.", ".bccb."], {"a": GOLD, "b": RUBY, "c": "#7A0F22", "d": "#FFC8CD"})])
item("v_badge", "body", "Значок V", "V badge", "rare", [L(9, 10, ["a.a", "a.a", ".a."], {"a": RUBY})])
item("heart_badge", "body", "Значок-сердце", "Heart badge", "common", [L(9, 10, ["a.a", "aaa", ".a."], {"a": "#FF5A8A"})])
item("star_badge", "body", "Значок-звезда", "Star badge", "common", [L(9, 10, [".a.", "aaa", "a.a"], {"a": "#FFD23C"})])
item("suspenders", "body", "Подтяжки", "Suspenders", "common", [L(5, 8, ["a....a"] * 11, {"a": "#2C5BD6"}, "both")])
item("bandolier", "body", "Патронташ", "Bandolier", "epic",
     [L(4, 8, ["a.......", "ba......", ".ba.....", "..ba....", "...ba...", "....ba..", ".....ba.", "......ba", ".......b"], {"a": "#6E3A1E", "b": GOLD})])
item("flower_lei", "body", "Гавайские цветы", "Flower lei", "rare", [L(4, 8, ["abcabcab", ".c....a.", "..bcab.."], {"a": "#FF7AA2", "b": "#FFE08A", "c": "#B98CFF"})])
item("bell_collar", "body", "Ошейник с колокольчиком", "Bell collar", "common", [L(4, 8, ["aaaaaaaa", "...bb...", "...bb..."], {"a": RUBY, "b": GOLD})])
item("tool_belt", "body", "Пояс с инструментами", "Tool belt", "rare",
     [L(4, 18, ["aaaaaaaa", "abcabdca", ".b.....d"], {"a": "#6E3A1E", "b": "#9AA4AE", "c": GOLD, "d": RUBY})])
item("fur_collar", "body", "Меховой воротник", "Fur collar", "epic", [L(3, 7, ["aaaaaaaaaa", "abaababaab", ".a......a."], {"a": "#E8DCC8", "b": "#C8BCA8"}, "both")])
item("vest", "body", "Жилет", "Vest", "rare", [L(4, 8, ["aa....aa", "aa....aa", "aab..baa", "aa....aa", "aab..baa", "aa....aa", "aaaaaaaa"], {"a": "#3A4A2A", "b": GOLD})])
recolor("scarf", "blue_scarf", "Синий шарф", "Blue scarf", "common", {RUBY: "#3C8CFF", "#B01C38": "#2C5BD6", "#7A0F22": "#1E3A8A"})
recolor("scarf", "green_scarf", "Зелёный шарф", "Green scarf", "common", {RUBY: "#5BB04E", "#B01C38": "#3E9B47", "#7A0F22": "#2E6B32"})
recolor("scarf", "white_scarf", "Белый шарф", "White scarf", "rare", {RUBY: WHITE, "#B01C38": "#D6CCD0", "#7A0F22": "#A89CA2"})
recolor("tie", "blue_tie", "Синий галстук", "Blue tie", "common", {"#7A0F22": "#2C5BD6"})
recolor("bow_tie", "gold_bow_tie", "Золотая бабочка", "Gold bow tie", "rare", {RUBY: GOLD})
recolor("bow_tie", "black_bow_tie", "Чёрная бабочка", "Black bow tie", "common", {RUBY: BLACK})
recolor("armor", "gold_armor", "Золотой нагрудник", "Gold chestplate", "legendary", {"#5A626C": GOLD_D, "#9AA4AE": GOLD})

# ---------------------------------------------------------------- back: wings, capes, tails, things on the back
wings("phoenix_wings", "Крылья феникса", "Phoenix wings", "legendary", "#FFD23C", "#FF8A3C", "#E8364F",
      ["...aab", "..aabc", ".aabbc", "aabbcc", "abbcc.", "abcc..", "bc....", "c....."])
wings("ice_wings", "Ледяные крылья", "Ice wings", "legendary", WHITE, "#BFE6FF", "#5CB6FF",
      ["....ab", "...abb", "..abbc", ".abbcc", "abbcc.", "abc...", ".bc...", "..c..."])
wings("bee_wings", "Крылья пчелы", "Bee wings", "common", "#FFFFFFAA", "#DDEFFF99", "#BCDCFF88", ["..aa..", ".abba.", ".abba.", "..aa..", "......", "......", "......", "......"])
wings("dragonfly_wings", "Крылья стрекозы", "Dragonfly wings", "rare", "#BFFFF4AA", "#7FE6D9AA", "#39C2B099",
      ["......", "aaabbc", "aabbcc", "......", "aabbcc", "abbcc.", "......", "......"])
wings("mech_wings", "Механические крылья", "Mechanical wings", "epic", "#C9D4DE", "#7C8794", "#3A3F48",
      ["....ab", "...abc", "..abc.", ".abcab", "abc.bc", ".bcabc", "..c.c.", "......"])
wings("golden_wings", "Золотые крылья", "Golden wings", "legendary", "#FFE08A", GOLD, GOLD_D)
wings("shadow_wings", "Крылья тени", "Shadow wings", "epic", "#3A2A4A", "#22162E", "#0E0812",
      ["...aaa", "..abbb", ".abbbc", "abbccc", "abbcc.", "abc.c.", "ab.c..", "a....."])
wings("emerald_wings", "Изумрудные крылья", "Emerald wings", "epic", "#7FE6A0", "#3EC27A", "#1E7A48")

def cape(id, ru, en, rarity, main, trim, dark, mark=None, mark_color=GOLD):
    rows = ["cccccccccc"] + ["aaaaaaaaaa"] * 14 + ["dddddddddd", ".dddddddd."]
    if mark:
        for j, r in enumerate(mark):
            row = list(rows[3 + j])
            for i, ch in enumerate(r):
                if ch != ".":
                    row[3 + i] = "b"
            rows[3 + j] = "".join(row)
    item(id, "back", ru, en, rarity, [L(3, 8, rows, {"a": main, "b": mark_color, "c": trim, "d": dark}, "back"),
                                       L(2, 9, ["a" + "." * 10 + "a"] * 15, {"a": main}, "front")])

cape("royal_cape", "Королевская мантия", "Royal mantle", "legendary", "#2C3E9E", GOLD, "#1E2A6A", ["#.#.", "####", "####"])
cape("forest_cape", "Лесной плащ", "Forest cloak", "rare", "#2E6B32", "#5BB04E", "#1E4A22", [".#..", "###.", ".#.."], "#8A5A2E")
cape("shadow_cape", "Плащ тени", "Shadow cloak", "epic", "#16101C", RUBY, "#0A060E", ["#..#", ".##.", "#..#"], RUBY)
cape("snow_cape", "Снежный плащ", "Snow cloak", "rare", WHITE, "#5CB6FF", "#BFE6FF", [".#.", "###", ".#."], "#5CB6FF")
cape("vellora_cape", "Плащ Vellora", "Vellora cloak", "legendary", "#5A0A18", RUBY, "#2A0A12", ["#..#", "#..#", ".##."], RUBY)
item("quiver", "back", "Колчан со стрелами", "Quiver", "rare",
     [L(8, 5, ["a.a.", "bbbb", "cccc", "cddc", "cccc", "cddc", "cccc", "cccc", ".cc."], {"a": WHITE, "b": "#8A5A2E", "c": "#6E3A1E", "d": GOLD}, "back")])
item("guitar_back", "back", "Гитара за спиной", "Guitar", "epic",
     [L(5, 4, ["....aa", "....a.", "...a..", "..b...", ".bbbb.", "bbccbb", "bbccbb", ".bbbb.", "bbbbbb", "bbbbbb", ".bbbb."], {"a": "#3A1A10", "b": RUBY, "c": BLACK}, "back")])
item("surfboard", "back", "Доска для серфинга", "Surfboard", "rare",
     [L(6, 3, [".aa.", "abba", "abba", "acca", "abba", "abba", "acca", "abba", "abba", "abba", "abba", "abba", ".aa."], {"a": "#5CB6FF", "b": WHITE, "c": "#FF8A3C"}, "back")])
item("shield_back", "back", "Щит за спиной", "Shield", "epic",
     [L(4, 9, ["aaaaaaaa", "abbbbbba", "abbccbba", "abccccba", "abbccbba", "abbbbbba", ".abbbba.", "..aaaa.."], {"a": "#9AA4AE", "b": "#7A0F22", "c": GOLD}, "back")])
item("crossed_swords", "back", "Скрещённые мечи", "Crossed swords", "legendary",
     [L(2, 4, ["a..........a", ".a........a.", "..a......a..", "...a....a...", "....a..a....", ".....aa.....", ".....aa.....", "....b..b....", "...c....c...", "..b......b.."], {"a": "#C9D4DE", "b": GOLD, "c": "#3A1A10"}, "both")])
item("turtle_shell", "back", "Панцирь черепахи", "Turtle shell", "rare",
     [L(4, 9, [".aaaaaa.", "abbabbaa", "abaabbba", "aabbaaba", "abbaabba", "abaabbaa", ".aaaaaa."], {"a": "#3E9B47", "b": "#2E6B32"}, "back")])
item("fox_tail", "back", "Лисий хвост", "Fox tail", "epic", [L(7, 17, [".aa.", "aaaa", "aaaa", "aaaa", ".aa.", ".ab.", "..b."], {"a": "#E06A1E", "b": WHITE}, "back", "bob")])
item("cat_tail", "back", "Кошачий хвост", "Cat tail", "rare", [L(8, 18, ["a.", "a.", "a.", ".a", ".a", "a.", "a."], {"a": "#1A0A0E"}, "back", "bob")])
item("devil_tail", "back", "Хвост демона", "Devil tail", "epic", [L(7, 18, [".a..", ".a..", "..a.", "..a.", ".a..", "aaa.", ".a.."], {"a": RUBY}, "back", "bob")])
item("dragon_tail", "back", "Хвост дракона", "Dragon tail", "legendary",
     [L(6, 17, [".aaaa.", "abaaba", ".aaaa.", "..aa..", "..ab..", "..aa..", "...a..", "...ab.", "....a."], {"a": "#7A0F22", "b": GOLD}, "back", "bob")])
recolor("jetpack", "rocket_pack", "Ракетный ранец", "Rocket pack", "legendary", {"#5A626C": "#7A0F22", "#9AA4AE": RUBY})
recolor("backpack", "blue_backpack", "Синий рюкзак", "Blue backpack", "common", {"#3A1A10": "#1E2A6A", "#6E3A1E": "#3C4CB0"})

# ---------------------------------------------------------------- in the hand: the right hand is at x 0..4, y 18..20
def blade(id, ru, en, rarity, edge, mid, hilt, guard):
    item(id, "hand", ru, en, rarity,
         [L(-3, 7, [".......ab", "......aba", ".....aba.", "....aba..", "...aba...", "..aba....", "d.ba.....", ".dd......", ".cd......", "c..d....."],
            {"a": edge, "b": mid, "c": hilt, "d": guard})])

blade("diamond_sword", "Алмазный меч", "Diamond sword", "epic", "#BFFFF4", "#39C2B0", "#3A1A10", "#1E7A6A")
blade("iron_sword", "Железный меч", "Iron sword", "common", WHITE, "#C9D4DE", "#3A1A10", "#7C8794")
blade("gold_sword", "Золотой меч", "Gold sword", "rare", "#FFF2B0", GOLD, "#3A1A10", GOLD_D)
blade("netherite_sword", "Незеритовый меч", "Netherite sword", "legendary", "#6A5A62", "#3A2E36", "#1A1218", "#2A2228")
blade("crimson_blade", "Багровый клинок", "Crimson blade", "legendary", "#FFC8CD", RUBY, "#2A0A12", GOLD)
item("pickaxe", "hand", "Кирка", "Pickaxe", "common",
     [L(-3, 9, [".aaaa..", "a...ba.", "...b..a", "..b....", ".b.....", "b......"], {"a": "#5CE0D0", "b": "#8A5A2E"})])
item("axe", "hand", "Топор", "Axe", "common", [L(-2, 9, ["..aa.", ".aaab", "..a.b", "...b.", "..b..", ".b..."], {"a": "#C9D4DE", "b": "#8A5A2E"})])
item("longbow", "hand", "Лук", "Bow", "rare", [L(-2, 8, ["..aa", ".a.b", "a..b", "a..b", "a..b", ".a.b", "..aa"], {"a": "#8A5A2E", "b": WHITE})])
item("trident", "hand", "Трезубец", "Trident", "legendary",
     [L(-3, 5, ["a.a.a", "aaaaa", "..a..", "..b..", "..b..", "..b..", "..b..", "..b..", "..b..", "..b.."], {"a": "#5CE0D0", "b": "#2E8B84"})])
item("torch", "hand", "Факел", "Torch", "common", [L(0, 12, [".a.", "aba", ".c.", ".c.", ".c.", ".c."], {"a": "#FF8A3C", "b": "#FFE08A", "c": "#8A5A2E"}, "front", "bob")])
item("lantern", "hand", "Фонарь", "Lantern", "rare", [L(0, 14, [".a.", "aaa", "aba", "aba", "aaa"], {"a": "#3A3F48", "b": "#FFD23C"}, "front", "bob")])
item("magic_staff", "hand", "Посох тессеракта", "Tesseract staff", "legendary",
     [L(-1, 3, [".aaa.", "abcba", "acdca", "abcba", ".aaa.", "..e..", "..e..", "..e..", "..e..", "..e..", "..e..", "..e..", "..e..", "..e.."],
        {"a": RUBY, "b": "#7A0F22", "c": "#FFC8CD", "d": WHITE, "e": "#3A1A10"}, "front", "bob")])
item("magic_wand", "hand", "Волшебная палочка", "Magic wand", "epic", [L(-1, 11, ["a.a..", ".b...", "a.c..", "...c.", "....c"], {"a": "#FFE08A", "b": WHITE, "c": "#3A1A10"})])
item("shield", "hand", "Щит", "Shield", "epic", [L(-3, 12, ["aaaaa", "abcba", "acccа".replace("а", "a"), "abcba", ".aba.", "..a.."], {"a": "#9AA4AE", "b": "#7A0F22", "c": GOLD})])
item("fishing_rod", "hand", "Удочка", "Fishing rod", "common", [L(-4, 6, ["a.....", "ba....", "b.a...", "b..a..", "c...a.", "c....a"], {"a": "#8A5A2E", "b": WHITE, "c": RUBY})])
item("bouquet", "hand", "Букет", "Bouquet", "rare", [L(-1, 13, ["abcab", ".aca.", "..d..", "..d.."], {"a": "#FF7AA2", "b": "#FFE08A", "c": "#B98CFF", "d": "#4C9A52"})])
item("heart_balloon", "hand", "Шарик-сердце", "Heart balloon", "rare",
     [L(-2, 0, ["aa.aa", "aaaaa", "aaaaa", ".aaa.", "..a..", "..b..", "..b..", "..b..", "...b.", "...b.", "..b..", "..b..", "..b.."], {"a": "#FF3B6E", "b": WHITE}, "front", "float")])
item("ice_cream", "hand", "Мороженое", "Ice cream", "common", [L(0, 14, [".ab.", "abba", ".cc.", ".cc.", "..c."], {"a": "#FF9AD5", "b": WHITE, "c": "#D9A441"})])
item("lollipop", "hand", "Леденец", "Lollipop", "common", [L(-1, 12, [".aba.", "abbba", ".aba.", "..c..", "..c.."], {"a": "#FF5A8A", "b": WHITE, "c": WHITE})])
item("talisman", "hand", "Талисман", "Talisman", "legendary", [L(-1, 14, [".aa.", "abba", "acca", ".aa."], {"a": GOLD, "b": "#3EC27A", "c": "#FFE08A"}, "front", "bob")])

# ---------------------------------------------------------------- pets
item("dog_pet", "pet", "Щенок", "Puppy", "rare", [L(17, 25, ["aa......", "aba.....", "aaaa...a", ".aaaaaa.", ".aaaaaa.", ".a.a.a.a"], {"a": "#C08A5A", "b": BLACK}, "both", "hop")])
item("wolf_pet", "pet", "Волчонок", "Wolf cub", "epic", [L(17, 25, ["a.a.....", "aaa.....", "aba....a", "aaaaaaa.", ".aaaaaa.", ".a..a..a"], {"a": "#C9D4DE", "b": BLACK}, "both", "hop")])
item("panda_pet", "pet", "Панда", "Panda", "epic", [L(18, 25, ["a...a", "bbbbb", "babab", "bbbbb", "aaaaa", "a...a"], {"a": BLACK, "b": WHITE}, "both", "hop")])
item("penguin_pet", "pet", "Пингвин", "Penguin", "rare", [L(18, 25, [".aaa.", "abcba", "aaaaa", "abbba", "abbba", ".d.d."], {"a": "#1A1A22", "b": WHITE, "c": "#FF8A3C", "d": "#FF8A3C"}, "both", "hop")])
item("frog_pet", "pet", "Лягушонок", "Froggy", "common", [L(18, 27, ["a...a", "aaaaa", "aabaa", ".aaa.", "a...a"], {"a": "#5BB04E", "b": RUBY}, "both", "hop")])
item("owl_pet", "pet", "Совёнок", "Owlet", "rare", [L(18, 14, ["a...a", "aaaaa", "bcacb", "aaaaa", "adada", ".a.a."], {"a": "#8A5A2E", "b": "#FFE08A", "c": BLACK, "d": "#C08A5A"}, "both", "float")])
item("bunny_pet", "pet", "Крольчонок", "Bunny", "common", [L(18, 24, [".a.a.", ".a.a.", "aaaaa", "abaab", "aaaaa", ".a.a."], {"a": WHITE, "b": "#FF7AA2"}, "both", "hop")])
item("turtle_pet", "pet", "Черепашка", "Turtle", "common", [L(17, 28, ["..aaaa..", ".abbbba.", "cabbbbac", ".c....c."], {"a": "#3E9B47", "b": "#2E6B32", "c": "#8BD13B"}, "both", "hop")])
item("snail_pet", "pet", "Улитка", "Snail", "common", [L(17, 28, ["...aaa.", "..abcba", "a.abbba", "dddddd."], {"a": "#D9A441", "b": "#B5832A", "c": "#7A4A24", "d": "#C8BCA8"}, "both", "hop")])
item("mushroom_pet", "pet", "Грибочек", "Shroom", "rare", [L(18, 26, [".aaa.", "abaab", "aaaaa", ".cdc.", ".ccc."], {"a": "#D6283A", "b": WHITE, "c": "#F4E6D0", "d": BLACK}, "both", "hop")])
item("creeper_pet", "pet", "Мини-крипер", "Mini creeper", "epic", [L(18, 24, ["aaaa", "baab", "abba", "abba", "aaaa", "a..a"], {"a": "#5BB04E", "b": "#13200F"}, "both", "hop")])
item("ghast_pet", "pet", "Мини-гаст", "Mini ghast", "epic", [L(17, 12, ["aaaaaa", "abaaba", "aaaaaa", "aacaaa", "a.a.a.", ".a.a.a"], {"a": WHITE, "b": BLACK, "c": RUBY}, "both", "float")])
item("spirit_pet", "pet", "Дух-хранитель", "Guardian spirit", "legendary", [L(18, 14, [".aaa.", "abcba", "aaaaa", "da.ad", ".a.a."], {"a": "#7FD6FF", "b": WHITE, "c": "#3C8CFF", "d": "#BFE6FF"}, "both", "float")])
item("drone_pet", "pet", "Дрон", "Drone", "epic", [L(17, 14, ["aa..aa", ".bbbb.", ".bcdb.", ".bbbb.", "a....a"], {"a": "#7C8794", "b": "#3A3F48", "c": RUBY, "d": "#39FFD2"}, "both", "float")])
item("jelly_pet", "pet", "Медуза", "Jellyfish", "rare", [L(18, 13, [".aaa.", "aabaa", "aaaaa", "c.c.c", ".c.c.", "c.c.c"], {"a": "#B98CFF", "b": WHITE, "c": "#FF9AD5"}, "both", "float")])
item("phoenix_pet", "pet", "Феникс", "Phoenix chick", "legendary", [L(17, 13, ["a....a", "ab..ba", "abccba", ".cdcc.", "..cc..", ".e..e."], {"a": "#E8364F", "b": "#FF8A3C", "c": "#FFD23C", "d": BLACK, "e": "#FF8A3C"}, "both", "float")])
item("sculk_pet", "pet", "Скалк-малыш", "Sculk baby", "legendary", [L(18, 26, [".aaa.", "abcba", "aacaa", "abbba", ".a.a."], {"a": "#0F2A33", "b": "#1E5B6A", "c": "#39FFD2"}, "both", "hop")])
item("pig_pet", "pet", "Поросёнок", "Piglet", "common", [L(17, 26, ["aa......", "abaaaaa.", "aacaaaa.", ".aaaaaa.", ".a.a.a.a"], {"a": "#F2A7B8", "b": BLACK, "c": "#D9507A"}, "both", "hop")])

# ---------------------------------------------------------------- effects
item("music_notes", "effect", "Ноты", "Music notes", "rare", particles={"colors": ["#FFFFFF", "#FF9AD5", "#7FD6FF"], "count": 9, "motion": "rise", "shape": "note"})
item("star_orbit", "effect", "Звёздная орбита", "Star orbit", "legendary", particles={"colors": ["#FFE08A", "#FFFFFF"], "count": 8, "motion": "orbit", "shape": "star"})
item("sakura", "effect", "Сакура", "Sakura", "epic", particles={"colors": ["#FFB7D5", "#FF9AC5", "#FFFFFF"], "count": 18, "motion": "fall"})
item("autumn_leaves", "effect", "Осенние листья", "Autumn leaves", "rare", particles={"colors": ["#E07A1E", "#D6283A", "#FFD23C"], "count": 14, "motion": "fall"})
item("bubbles", "effect", "Пузыри", "Bubbles", "common", particles={"colors": ["#BFE6FFAA", "#FFFFFFAA"], "count": 12, "motion": "rise"})
item("confetti", "effect", "Конфетти", "Confetti", "rare", particles={"colors": ["#FF5A6E", "#FFD23C", "#5CB6FF", "#7CE07C", "#B98CFF"], "count": 26, "motion": "fall"})
item("embers", "effect", "Угольки", "Embers", "epic", particles={"colors": ["#FF8A3C", "#FF3B4F"], "count": 16, "motion": "rise"})
item("fireflies", "effect", "Светлячки", "Fireflies", "rare", particles={"colors": ["#E6FF7A", "#B8FF5A"], "count": 12, "motion": "spark"})
item("ghost_wisps", "effect", "Блуждающие огни", "Wisps", "epic", particles={"colors": ["#7FD6FF", "#BFE6FF"], "count": 6, "motion": "orbit"})
item("rune_orbit", "effect", "Руны", "Runes", "legendary", particles={"colors": ["#B04CFF", "#E0B0FF"], "count": 10, "motion": "orbit", "shape": "star"})
item("crimson_aura", "effect", "Багровая аура", "Crimson aura", "legendary", particles={"colors": ["#E8364F", "#7A0F22", "#FFC8CD"], "count": 30, "motion": "rise"})

# ---------------------------------------------------------------- more emotes
for id, ru, en in [("clap", "Хлопать", "Clap"), ("salute", "Честь", "Salute"), ("shrug", "Ну не знаю", "Shrug"), ("point", "Указать", "Point"),
                   ("dab", "Дэб", "Dab"), ("flip", "Сальто", "Backflip"), ("think", "Думать", "Think"), ("punch", "Бокс", "Punch")]:
    item(id, "emote", ru, en, "rare")

OUTFITS = [
    {"title": "Демон-повелитель", "titleEn": "Demon lord", "head": "horns", "back": "demon_wings", "body": "scarf", "pet": "", "effect": "flames"},
    {"title": "Падший ангел", "titleEn": "Fallen angel", "head": "halo", "back": "angel_wings", "body": "heart_necklace", "pet": "", "effect": "sparkles"},
    {"title": "Маг тессеракта", "titleEn": "Tesseract mage", "head": "wizard_hat", "back": "crimson_cape", "body": "chain", "pet": "cube_pet", "effect": "void_orbit"},
    {"title": "Тень ночи", "titleEn": "Night shadow", "head": "mask", "back": "bat_wings", "body": "tie", "pet": "bat_pet", "effect": ""},
    {"title": "Король", "titleEn": "King", "head": "crown", "back": "crimson_cape", "body": "chain", "pet": "slime_pet", "effect": ""},
    {"title": "Повелитель драконов", "titleEn": "Dragon lord", "head": "devil_crown", "back": "dragon_wings", "body": "armor", "pet": "dragon_pet", "effect": "flames"},
    {"title": "Пират", "titleEn": "Pirate", "head": "pirate_hat", "back": "sword_back", "body": "belt", "pet": "", "effect": ""},
    {"title": "Викинг", "titleEn": "Viking", "head": "viking_helmet", "back": "sword_back", "body": "armor", "pet": "fox_pet", "effect": ""},
    {"title": "Фея", "titleEn": "Fairy", "head": "flower_crown", "back": "fairy_wings", "body": "heart_necklace", "pet": "bee_pet", "effect": "sparkles"},
    {"title": "Зимний", "titleEn": "Winter", "head": "beanie", "back": "backpack", "body": "scarf", "pet": "cat_pet", "effect": "snow"},
    {"title": "Изобретатель", "titleEn": "Inventor", "head": "headphones", "back": "jetpack", "body": "belt", "pet": "chick_pet", "effect": ""},
    {"title": "Джентльмен", "titleEn": "Gentleman", "head": "top_hat", "back": "", "body": "bow_tie", "pet": "axolotl_pet", "effect": ""},
    {"title": "Самурай", "titleEn": "Samurai", "head": "samurai_helmet", "back": "shadow_cape", "body": "sash", "pet": "fox_pet", "effect": "sakura", "hand": "crimson_blade"},
    {"title": "Феникс", "titleEn": "Phoenix", "head": "crimson_halo", "back": "phoenix_wings", "body": "tesseract_amulet", "pet": "phoenix_pet", "effect": "embers", "hand": "magic_staff"},
    {"title": "Снежная королева", "titleEn": "Snow queen", "head": "ice_crown", "back": "ice_wings", "body": "pearl_necklace", "pet": "penguin_pet", "effect": "snow", "hand": "magic_wand"},
    {"title": "Рыцарь", "titleEn": "Knight", "head": "knight_helmet", "back": "royal_cape", "body": "gold_armor", "pet": "wolf_pet", "effect": "", "hand": "diamond_sword"},
    {"title": "Лесной охотник", "titleEn": "Forest hunter", "head": "antlers", "back": "quiver", "body": "vest", "pet": "owl_pet", "effect": "fireflies", "hand": "longbow"},
    {"title": "Рок-звезда", "titleEn": "Rock star", "head": "star_glasses", "back": "guitar_back", "body": "thick_chain", "pet": "drone_pet", "effect": "music_notes", "hand": ""},
    {"title": "Хэллоуин", "titleEn": "Halloween", "head": "pumpkin_head", "back": "bat_wings", "body": "bell_collar", "pet": "ghost_pet", "effect": "embers", "hand": "lantern"},
    {"title": "Праздник", "titleEn": "Party", "head": "party_hat", "back": "", "body": "flower_lei", "pet": "bunny_pet", "effect": "confetti", "hand": "heart_balloon"},
    {"title": "Шахтёр", "titleEn": "Miner", "head": "pilot_goggles", "back": "blue_backpack", "body": "tool_belt", "pet": "creeper_pet", "effect": "", "hand": "pickaxe"},
    {"title": "Повелитель Vellora", "titleEn": "Vellora lord", "head": "devil_crown", "back": "vellora_cape", "body": "v_badge", "pet": "sculk_pet", "effect": "crimson_aura", "hand": "talisman"},
    {"title": "Морской волк", "titleEn": "Sea wolf", "head": "bandana_head", "back": "surfboard", "body": "bandolier", "pet": "turtle_pet", "effect": "bubbles", "hand": "trident"},
    {"title": "Единорог", "titleEn": "Unicorn", "head": "unicorn_horn", "back": "butterfly_wings", "body": "heart_badge", "pet": "jelly_pet", "effect": "sparkles", "hand": "lollipop"},
]

data = {"items": ITEMS, "outfits": OUTFITS}
for target in (ROOT / "android" / "assets" / "cosmetics.json", ROOT / "launcher" / "resources" / "sculk_theme" / "cosmetics.json"):
    target.write_text(json.dumps(data, ensure_ascii=False, indent=1), encoding="utf-8")
print(len(ITEMS), "items,", len(OUTFITS), "outfits")
