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
]

data = {"items": ITEMS, "outfits": OUTFITS}
for target in (ROOT / "android" / "assets" / "cosmetics.json", ROOT / "launcher" / "resources" / "sculk_theme" / "cosmetics.json"):
    target.write_text(json.dumps(data, ensure_ascii=False, indent=1), encoding="utf-8")
print(len(ITEMS), "items,", len(OUTFITS), "outfits")
