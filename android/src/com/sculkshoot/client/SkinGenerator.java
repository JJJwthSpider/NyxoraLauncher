package com.sculkshoot.client;

import android.graphics.Bitmap;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.Random;

/**
 * Arkan's skin and cape generator: a description in words ("рыцарь в золотых доспехах", "рыжая девушка-маг", "ниндзя с
 * красной повязкой", "galaxy cape with stars") becomes Minecraft skins (64x64, both layers, slim arms for girls) and capes
 * (64x32). The words choose the look (knight, wizard, ninja, pirate, princess, astronaut, demon, cyber...), the colours of
 * the clothes, hair, eyes and skin, an emblem on the chest; every variant is drawn with shading and texture like a
 * hand-made skin. It works without internet; a new press gives new variants.
 */
final class SkinGenerator {
    private SkinGenerator() {}

    static final class Skin {
        final Bitmap bitmap;
        final boolean slim;
        final String title;

        Skin(Bitmap bitmap, boolean slim, String title) {
            this.bitmap = bitmap;
            this.slim = slim;
            this.title = title;
        }
    }

    /**
     * What was chosen with the buttons under the description: it wins over the words. 0 or null is "as the words say, or
     * at random". The same options with the same seed give the same skins, so a tap on a colour repaints the variants.
     */
    static final class Options {
        int clothes, hair, eyes, skin;
        int gender;        // 0: as said, 1: a boy, 2: a girl
        String emblem;     // null: as said or at random; "": none
        String hairStyle;  // null: as said or at random
        int second;        // capes: the second colour
        String pattern;    // capes: gradient, stripes, checker, diagonal, border, galaxy, flames, ice, leaves, rainbow

        boolean empty() {
            return clothes == 0 && hair == 0 && eyes == 0 && skin == 0 && gender == 0 && emblem == null && hairStyle == null && second == 0
                && pattern == null;
        }
    }

    /* ------------------------------------------------------------ "surprise me" */

    private static final String[] SURPRISE_WHO = { "рыцарь", "ниндзя", "маг", "пират", "принцесса", "космонавт", "демон", "кибер", "зомби", "скелет",
                                                   "эльф", "викинг", "вампир", "ангел", "король", "солдат", "хакер в худи", "агент в костюме", "повар",
                                                   "доктор", "футболист", "ведьма", "девушка-маг", "девушка-эльф", "девушка в худи" };
    private static final String[] SURPRISE_COLOR = { "в красном", "в бордовом", "в синем", "в зелёном", "в фиолетовом", "в золотом", "в чёрном",
                                                     "в белом", "в розовом", "в оранжевом", "в бирюзовом", "в голубом", "" };
    private static final String[] SURPRISE_DETAIL = { "с короной", "с бородой", "со светящимися глазами", "с черепом", "со звездой", "с молнией",
                                                      "с длинными волосами", "с ирокезом", "в капюшоне", "в шлеме", "с сердцем", "рыжий", "блондин", "", "" };
    private static final String[] SURPRISE_CAPE = { "галактика", "огненный плащ", "ледяной плащ", "плащ в полоску", "плащ в клетку", "радужный плащ",
                                                    "лесной плащ", "плащ с рамкой", "плащ" };
    private static final String[] SURPRISE_CAPE_COLOR = { "красный", "бордовый", "синий", "зелёный", "фиолетовый", "золотой", "чёрный", "белый",
                                                          "розовый", "оранжевый", "бирюзовый", "голубой" };
    private static final String[] SURPRISE_CAPE_MARK = { "со звездой", "с сердцем", "с молнией", "с короной", "с луной", "с черепом", "с крипером",
                                                         "с алмазом", "с пламенем", "", "" };

    /** a description at random, for the 🎲 button: "викинг в синем с бородой", "фиолетовая галактика со звездой" */
    static String surprise(Random rnd, boolean cape) {
        StringBuilder b = new StringBuilder();
        if (cape) {
            String what = SURPRISE_CAPE[rnd.nextInt(SURPRISE_CAPE.length)];
            String color = SURPRISE_CAPE_COLOR[rnd.nextInt(SURPRISE_CAPE_COLOR.length)];
            if (what.startsWith("галактика") || what.startsWith("радужн") || what.startsWith("огнен") || what.startsWith("ледян") || what.startsWith("лесн")) {
                b.append(what);
            } else {
                b.append(color).append(' ').append(what);
            }
            String mark = SURPRISE_CAPE_MARK[rnd.nextInt(SURPRISE_CAPE_MARK.length)];
            if (!mark.isEmpty()) {
                b.append(' ').append(mark);
            }
        } else {
            b.append(SURPRISE_WHO[rnd.nextInt(SURPRISE_WHO.length)]);
            String color = SURPRISE_COLOR[rnd.nextInt(SURPRISE_COLOR.length)];
            if (!color.isEmpty()) {
                b.append(' ').append(color);
            }
            String detail = SURPRISE_DETAIL[rnd.nextInt(SURPRISE_DETAIL.length)];
            if (!detail.isEmpty()) {
                b.append(' ').append(detail);
            }
        }
        String text = b.toString();
        return Character.toUpperCase(text.charAt(0)) + text.substring(1);
    }

    /* ------------------------------------------------------------ the words */

    private static final String[][] COLORS = { { "бордов", "6E1423" }, { "burgundy", "6E1423" }, { "maroon", "6E1423" }, { "красн", "C62828" }, { "red", "C62828" },
                                               { "алый", "E53935" }, { "розов", "EC6FA8" }, { "pink", "EC6FA8" }, { "оранж", "EF7C1E" }, { "orange", "EF7C1E" },
                                               { "рыж", "D9632B" }, { "жёлт", "F2C230" }, { "желт", "F2C230" }, { "yellow", "F2C230" }, { "золот", "E0A526" },
                                               { "gold", "E0A526" }, { "зелён", "3E9B47" }, { "зелен", "3E9B47" }, { "green", "3E9B47" }, { "изумруд", "1E8E5A" },
                                               { "салат", "8BD13B" }, { "lime", "8BD13B" }, { "бирюз", "23B5B5" }, { "cyan", "23B5B5" }, { "teal", "23B5B5" },
                                               { "голуб", "5AB4F0" }, { "син", "2C5BD6" }, { "blue", "2C5BD6" }, { "фиолет", "7B3FC4" }, { "purple", "7B3FC4" },
                                               { "violet", "7B3FC4" }, { "сирен", "B48CE6" }, { "чёрн", "1E1E24" }, { "черн", "1E1E24" }, { "black", "1E1E24" },
                                               { "тёмн", "26232B" }, { "темн", "26232B" }, { "dark", "26232B" }, { "бел", "ECECEC" }, { "white", "ECECEC" },
                                               { "серебр", "C0C6CE" }, { "silver", "C0C6CE" }, { "серый", "8A8F98" }, { "серая", "8A8F98" }, { "серые", "8A8F98" },
                                               { "серого", "8A8F98" }, { "grey", "8A8F98" }, { "gray", "8A8F98" }, { "коричнев", "6B4423" }, { "brown", "6B4423" },
                                               { "бежев", "D8C29D" }, { "beige", "D8C29D" }, { "неон", "39FF88" }, { "neon", "39FF88" } };

    private static final String[][] EMBLEMS = { { "сердц", "heart" }, { "сердеч", "heart" }, { "heart", "heart" }, { "любов", "heart" }, { "звезд", "star" },
                                                { "звёзд", "star" }, { "star", "star" }, { "череп", "skull" }, { "skull", "skull" }, { "крипер", "creeper" },
                                                { "creeper", "creeper" }, { "молни", "bolt" }, { "lightning", "bolt" }, { "flash", "bolt" }, { "лун", "moon" },
                                                { "moon", "moon" }, { "огон", "flame" }, { "огн", "flame" }, { "пламя", "flame" }, { "fire", "flame" }, { "алмаз", "diamond" },
                                                { "diamond", "diamond" }, { "корон", "crown" }, { "crown", "crown" }, { "vellora", "v" }, { "веллора", "v" } };

    /** the looks: the words that call them, the look id */
    private static final String[][] THEMES = { { "рыцар", "knight" }, { "knight", "knight" }, { "латы", "knight" }, { "доспех", "knight" }, { "броня", "knight" },
                                               { "armor", "knight" }, { "паладин", "knight" }, { "воин", "knight" }, { "warrior", "knight" }, { "ниндзя", "ninja" },
                                               { "ninja", "ninja" }, { "шиноби", "ninja" }, { "ассасин", "ninja" }, { "assassin", "ninja" }, { "маг", "wizard" },
                                               { "волшеб", "wizard" }, { "wizard", "wizard" }, { "mage", "wizard" }, { "колдун", "wizard" }, { "чародей", "wizard" },
                                               { "некромант", "wizard" }, { "ведьм", "witch" }, { "witch", "witch" }, { "пират", "pirate" }, { "pirate", "pirate" },
                                               { "капитан", "pirate" }, { "зомби", "zombie" }, { "zombie", "zombie" }, { "скелет", "skeleton" }, { "skeleton", "skeleton" },
                                               { "демон", "demon" }, { "demon", "demon" }, { "дьявол", "demon" }, { "devil", "demon" }, { "ангел", "angel" },
                                               { "angel", "angel" }, { "кибер", "cyber" }, { "cyber", "cyber" }, { "робот", "cyber" }, { "robot", "cyber" },
                                               { "андроид", "cyber" }, { "космонавт", "astronaut" }, { "астронавт", "astronaut" }, { "astronaut", "astronaut" },
                                               { "космос", "astronaut" }, { "принцесс", "princess" }, { "princess", "princess" }, { "королев", "princess" },
                                               { "queen", "princess" }, { "фея", "princess" }, { "fairy", "princess" }, { "король", "king" }, { "короля", "king" },
                                               { "king", "king" }, { "принц", "king" }, { "prince", "king" }, { "царь", "king" }, { "император", "king" }, { "вампир", "vampire" },
                                               { "vampire", "vampire" }, { "дракул", "vampire" }, { "эльф", "elf" }, { "elf", "elf" }, { "лучник", "elf" },
                                               { "archer", "elf" }, { "охотни", "elf" }, { "викинг", "viking" }, { "viking", "viking" }, { "варвар", "viking" },
                                               { "barbarian", "viking" }, { "солдат", "soldier" }, { "soldier", "soldier" }, { "военн", "soldier" },
                                               { "камуфляж", "soldier" }, { "camo", "soldier" }, { "спецназ", "soldier" }, { "худи", "hoodie" }, { "hoodie", "hoodie" },
                                               { "толстовк", "hoodie" }, { "хакер", "hoodie" }, { "hacker", "hoodie" }, { "геймер", "hoodie" }, { "gamer", "hoodie" },
                                               { "стример", "hoodie" }, { "костюм", "suit" }, { "suit", "suit" }, { "бизнес", "suit" }, { "агент", "suit" },
                                               { "agent", "suit" }, { "мафи", "suit" }, { "детектив", "suit" }, { "повар", "chef" }, { "chef", "chef" },
                                               { "доктор", "doctor" }, { "врач", "doctor" }, { "doctor", "doctor" }, { "учён", "doctor" }, { "scientist", "doctor" },
                                               { "футбол", "sport" }, { "спорт", "sport" }, { "football", "sport" }, { "баскетбол", "sport" } };

    private static final String[] GIRL = { "девушк", "девочк", "girl", "woman", "женщин", "леди", "lady", "she", "аниме", "anime", "кавай", "kawaii" };

    private static String[] words(String prompt) {
        return prompt.toLowerCase(Locale.ROOT).replace('ё', 'е').split("[^\\p{L}\\p{N}]+");
    }

    private static boolean has(String[] words, String... stems) {
        for (String w : words) {
            for (String s : stems) {
                String stem = s.replace('ё', 'е');
                if (!stem.isEmpty() && w.startsWith(stem)) {
                    return true;
                }
            }
        }
        return false;
    }

    private static int colorOf(String word) {
        String w = word.replace('ё', 'е');
        for (String[] c : COLORS) {
            String stem = c[0].replace('ё', 'е');
            if (w.startsWith(stem) && !(stem.equals("бел") && w.startsWith("белк"))) {
                return 0xFF000000 | Integer.parseInt(c[1], 16);
            }
        }
        return 0;
    }

    /** the colour said next to a word ("рыжие волосы", "eyes blue", "с зелёной кожей") */
    private static int colorNear(String[] words, String... nouns) {
        for (int i = 0; i < words.length; i++) {
            boolean noun = false;
            for (String n : nouns) {
                noun |= words[i].startsWith(n);
            }
            if (!noun) {
                continue;
            }
            for (int d = 1; d <= 2; d++) {
                if (i - d >= 0 && colorOf(words[i - d]) != 0) {
                    return colorOf(words[i - d]);
                }
                if (i + d < words.length && colorOf(words[i + d]) != 0) {
                    return colorOf(words[i + d]);
                }
            }
        }
        return 0;
    }

    /** the colours said for the clothes, in order, without those said for hair, eyes or skin */
    private static List<Integer> clothesColors(String[] words) {
        List<Integer> out = new ArrayList<Integer>();
        for (int i = 0; i < words.length; i++) {
            int c = colorOf(words[i]);
            if (c == 0) {
                continue;
            }
            boolean other = false;
            for (int d = -2; d <= 2; d++) {
                if (d != 0 && i + d >= 0 && i + d < words.length) {
                    String n = words[i + d];
                    other |= n.startsWith("волос") || n.startsWith("hair") || n.startsWith("глаз") || n.startsWith("eye") || n.startsWith("кож") || n.startsWith("skin");
                }
            }
            if (words[i].startsWith("рыж")) {
                other = true;  // "рыжая" is about the hair
            }
            if (!other) {
                out.add(c);
            }
        }
        return out;
    }

    /* ------------------------------------------------------------ colours */

    private static int shade(int c, float f) {
        int r = Math.max(0, Math.min(255, Math.round(((c >> 16) & 0xFF) * f)));
        int g = Math.max(0, Math.min(255, Math.round(((c >> 8) & 0xFF) * f)));
        int b = Math.max(0, Math.min(255, Math.round((c & 0xFF) * f)));
        return 0xFF000000 | (r << 16) | (g << 8) | b;
    }

    private static int mix(int a, int b, float t) {
        int r = Math.round(((a >> 16) & 0xFF) * (1 - t) + ((b >> 16) & 0xFF) * t);
        int g = Math.round(((a >> 8) & 0xFF) * (1 - t) + ((b >> 8) & 0xFF) * t);
        int bl = Math.round((a & 0xFF) * (1 - t) + (b & 0xFF) * t);
        return 0xFF000000 | (r << 16) | (g << 8) | bl;
    }

    /** a colour turned on the colour wheel a little (variants of one look) */
    private static int hueShift(int c, float degrees) {
        float[] hsv = new float[3];
        android.graphics.Color.colorToHSV(c, hsv);
        hsv[0] = (hsv[0] + degrees + 360) % 360;
        return android.graphics.Color.HSVToColor(hsv);
    }

    private static final int[] SKIN_TONES = { 0xFFF2C7A5, 0xFFE8B48C, 0xFFD39A6A, 0xFFB57A4A, 0xFF8D5524, 0xFFFFDBB8 };
    private static final int[] HAIR_COLORS = { 0xFF2B1B12, 0xFF4A2C1A, 0xFF6B4423, 0xFFB5834A, 0xFFE3C16F, 0xFF1E1E24, 0xFFA33A1E };

    /* ------------------------------------------------------------ the texture */

    private static final int TOP = 0, BOTTOM = 1, RIGHT = 2, FRONT = 3, LEFT = 4, BACK = 5;

    /** paints one face pixel: the face, the place on it and the face's size */
    private interface Painter {
        int at(int face, int x, int y, int w, int h);
    }

    private static final class Tex {
        final int[] px;
        final int w, h;
        final Random rnd;

        Tex(int w, int h, Random rnd) {
            this.w = w;
            this.h = h;
            this.px = new int[w * h];
            this.rnd = rnd;
        }

        void set(int x, int y, int c) {
            if (x >= 0 && y >= 0 && x < w && y < h) {
                px[y * w + x] = c;
            }
        }

        /** a box of the skin layout at (u, v); `noise` gives the cloth its grain, the faces are shaded like Minecraft's */
        void box(int u, int v, int bw, int bh, int bd, Painter p, float noise) {
            for (int face = 0; face < 6; face++) {
                int fx, fy, fw, fh;
                switch (face) {
                case TOP: fx = u + bd; fy = v; fw = bw; fh = bd; break;
                case BOTTOM: fx = u + bd + bw; fy = v; fw = bw; fh = bd; break;
                case RIGHT: fx = u; fy = v + bd; fw = bd; fh = bh; break;
                case FRONT: fx = u + bd; fy = v + bd; fw = bw; fh = bh; break;
                case LEFT: fx = u + bd + bw; fy = v + bd; fw = bd; fh = bh; break;
                default: fx = u + 2 * bd + bw; fy = v + bd; fw = bw; fh = bh; break;
                }
                for (int y = 0; y < fh; y++) {
                    for (int x = 0; x < fw; x++) {
                        int c = p.at(face, x, y, fw, fh);
                        if (c == 0) {
                            continue;
                        }
                        float f = 1 + (rnd.nextFloat() - 0.5f) * noise;
                        if (face == TOP) {
                            f *= 1.06f;
                        } else if (face == BOTTOM) {
                            f *= 0.72f;
                        } else if (face == RIGHT || face == LEFT) {
                            f *= 0.9f;
                        } else if (face == BACK) {
                            f *= 0.94f;
                        }
                        if (face != TOP && face != BOTTOM && fh > 4) {
                            f *= 1.04f - 0.1f * y / fh;  // lighter at the top
                        }
                        set(fx + x, fy + y, (c >>> 24) == 0xFF ? shade(c, f) : c);
                    }
                }
            }
        }

        Bitmap bitmap() {
            return Bitmap.createBitmap(px, w, h, Bitmap.Config.ARGB_8888);
        }
    }

    /* ------------------------------------------------------------ the look */

    private static final class Look {
        String theme = "casual";
        String title = "";
        boolean girl;
        int skin, hair, eyes, top, accent, pants, shoes, metal;
        String hairStyle = "short";  // short, long, spiky, bun, bald, mohawk
        String head = "";            // hood, helmet, bandana, crown, mask, tiara, cap, visor, viking, space, headband, horns, halo
        String outfit = "tshirt";    // tshirt, hoodie, jacket, suit, robe, armor, ninja, dress, space, skeleton, zombie, fur, camo, coat, jersey, pirate
        String emblem = "";
        boolean beard, glowEyes, eyepatch, gloves, longSleeves = true, belt;
    }

    private static final String[] EMBLEM_CHOICES = { "heart", "star", "bolt", "moon", "flame", "diamond", "v", "" };
    private static final String[] HAIR_STYLES = { "short", "spiky", "long", "short", "bun", "mohawk" };

    private static Look look(String prompt, Random rnd, int variant, Options o) {
        String[] w = words(prompt);
        Look l = new Look();
        l.girl = has(w, GIRL) || has(w, "принцесс", "princess", "ведьм", "witch", "фея", "fairy", "королев", "queen");
        if (o != null && o.gender != 0) {
            l.girl = o.gender == 2;
        }
        for (String[] t : THEMES) {
            if (has(w, t[0])) {
                l.theme = t[1];
                break;
            }
        }
        l.skin = SKIN_TONES[rnd.nextInt(SKIN_TONES.length)];
        l.hair = HAIR_COLORS[rnd.nextInt(HAIR_COLORS.length)];
        l.eyes = new int[] { 0xFF3B6FD6, 0xFF2E8B57, 0xFF6B4423, 0xFF1E1E24, 0xFF4FA3E0 }[rnd.nextInt(5)];
        l.hairStyle = l.girl ? (rnd.nextBoolean() ? "long" : "bun") : HAIR_STYLES[rnd.nextInt(HAIR_STYLES.length)];
        int base = 0xFF000000 | Color.HUE[rnd.nextInt(Color.HUE.length)];
        l.top = base;
        l.accent = hueShift(base, 150 + rnd.nextInt(60));
        l.pants = new int[] { 0xFF2C3E73, 0xFF26232B, 0xFF4A4F5A, 0xFF5B3A26, 0xFF1E3A5F }[rnd.nextInt(5)];
        l.shoes = new int[] { 0xFF1E1E24, 0xFFECECEC, 0xFF5B3A26, 0xFF8A3442 }[rnd.nextInt(4)];
        l.metal = 0xFFB9C0CA;
        l.emblem = EMBLEM_CHOICES[(rnd.nextInt(EMBLEM_CHOICES.length) + variant) % EMBLEM_CHOICES.length];
        l.longSleeves = rnd.nextBoolean();
        l.outfit = new String[] { "tshirt", "hoodie", "jacket", "tshirt" }[(variant + rnd.nextInt(2)) % 4];

        switch (l.theme) {
        case "knight":
            l.outfit = "armor";
            l.metal = has(w, "золот", "gold") ? 0xFFE0A526 : has(w, "черн", "black", "темн", "dark") ? 0xFF3A3A44 : 0xFFB9C0CA;
            l.head = variant % 2 == 0 ? "helmet" : "";
            l.accent = 0xFFC62828;
            l.title = "Рыцарь";
            l.emblem = variant % 3 == 0 ? "crown" : variant % 3 == 1 ? "" : "diamond";
            break;
        case "ninja":
            l.outfit = "ninja";
            l.top = 0xFF1E1E24;
            l.accent = 0xFFC62828;
            l.pants = 0xFF1E1E24;
            l.shoes = 0xFF26232B;
            l.head = "mask";
            l.emblem = "";
            l.title = "Ниндзя";
            break;
        case "wizard":
        case "witch":
            l.outfit = "robe";
            l.top = new int[] { 0xFF4B2A8C, 0xFF2C3E9E, 0xFF6E1423, 0xFF1E5B4A }[variant % 4];
            l.accent = 0xFFE0A526;
            l.head = variant % 2 == 0 ? "hood" : "";
            l.beard = !l.girl && variant % 2 == 0;
            if (l.beard) {
                l.hair = 0xFFE6E6E6;
            }
            l.emblem = variant % 2 == 0 ? "star" : "moon";
            l.title = l.theme.equals("witch") ? "Ведьма" : "Маг";
            break;
        case "pirate":
            l.outfit = "pirate";
            l.top = new int[] { 0xFF4A2C1A, 0xFF26232B, 0xFF6E1423 }[variant % 3];
            l.accent = 0xFFE0A526;
            l.head = "bandana";
            l.eyepatch = variant % 2 == 0;
            l.beard = !l.girl && variant % 3 != 1;
            l.pants = 0xFF3B2A1E;
            l.shoes = 0xFF1E1E24;
            l.emblem = "";
            l.title = "Пират";
            break;
        case "zombie":
            l.outfit = "zombie";
            l.skin = 0xFF5E9E52;
            l.top = 0xFF2CA6A4;
            l.pants = 0xFF4B3A8C;
            l.eyes = 0xFF1E1E24;
            l.hair = 0xFF2E4A2A;
            l.hairStyle = "short";
            l.emblem = "";
            l.title = "Зомби";
            break;
        case "skeleton":
            l.outfit = "skeleton";
            l.skin = 0xFFE8E4D8;
            l.top = 0xFF1A1A1F;
            l.emblem = "";
            l.hairStyle = "bald";
            l.title = "Скелет";
            break;
        case "demon":
            l.outfit = variant % 2 == 0 ? "suit" : "robe";
            l.skin = 0xFFB3262F;
            l.top = 0xFF1E1E24;
            l.accent = 0xFFFF6B1E;
            l.hair = 0xFF1E1E24;
            l.eyes = 0xFFFFD34A;
            l.glowEyes = true;
            l.head = "horns";
            l.emblem = "flame";
            l.title = "Демон";
            break;
        case "angel":
            l.outfit = "robe";
            l.top = 0xFFF1EEE6;
            l.accent = 0xFFE0A526;
            l.hair = 0xFFF0D68A;
            l.eyes = 0xFF4FA3E0;
            l.head = "halo";
            l.emblem = "";
            l.title = "Ангел";
            break;
        case "cyber":
            l.outfit = "suit";
            l.top = 0xFF16161D;
            l.accent = new int[] { 0xFF23E0E0, 0xFFFF3D9A, 0xFF39FF88, 0xFFB04CFF }[variant % 4];
            l.pants = 0xFF16161D;
            l.shoes = 0xFF26232B;
            l.head = "visor";
            l.glowEyes = true;
            l.eyes = l.accent;
            l.emblem = "bolt";
            l.title = "Кибер";
            break;
        case "astronaut":
            l.outfit = "space";
            l.top = 0xFFECECEC;
            l.accent = new int[] { 0xFFEF7C1E, 0xFF2C5BD6, 0xFFC62828 }[variant % 3];
            l.pants = 0xFFDADADA;
            l.shoes = 0xFF8A8F98;
            l.head = variant % 2 == 0 ? "space" : "";
            l.emblem = "";
            l.title = "Космонавт";
            break;
        case "king":
            l.outfit = "robe";
            l.top = 0xFF8E1C2C;
            l.accent = 0xFFE0A526;
            l.head = "crown";
            l.beard = !l.girl && variant % 2 == 0;
            l.emblem = "crown";
            l.title = "Король";
            break;
        case "princess":
            l.outfit = "dress";
            l.top = new int[] { 0xFFEC6FA8, 0xFFB48CE6, 0xFF5AB4F0, 0xFFF1EEE6 }[variant % 4];
            l.accent = 0xFFE0A526;
            l.head = "tiara";
            l.hairStyle = "long";
            l.girl = true;
            l.emblem = "heart";
            l.title = "Принцесса";
            break;
        case "vampire":
            l.outfit = "suit";
            l.skin = 0xFFE9E4E0;
            l.top = 0xFF1E1E24;
            l.accent = 0xFFB3121F;
            l.hair = 0xFF1E1E24;
            l.eyes = 0xFFE53935;
            l.glowEyes = true;
            l.emblem = "";
            l.title = "Вампир";
            break;
        case "elf":
            l.outfit = "jacket";
            l.top = 0xFF3E7B3A;
            l.accent = 0xFF6B4423;
            l.hair = 0xFFE3C16F;
            l.hairStyle = "long";
            l.pants = 0xFF5B3A26;
            l.shoes = 0xFF4A2C1A;
            l.belt = true;
            l.head = variant % 2 == 0 ? "headband" : "";
            l.emblem = "";
            l.title = "Эльф";
            break;
        case "viking":
            l.outfit = "fur";
            l.top = 0xFF5B3A26;
            l.accent = 0xFF9A8F80;
            l.beard = !l.girl;
            l.hair = new int[] { 0xFFA33A1E, 0xFFE3C16F, 0xFF6B4423 }[variant % 3];
            l.head = variant % 2 == 0 ? "viking" : "";
            l.pants = 0xFF4A3B2E;
            l.shoes = 0xFF3B2A1E;
            l.emblem = "";
            l.title = "Викинг";
            break;
        case "soldier":
            l.outfit = "camo";
            l.top = 0xFF4B5D3A;
            l.pants = 0xFF4B5D3A;
            l.shoes = 0xFF1E1E24;
            l.head = variant % 2 == 0 ? "cap" : "";
            l.belt = true;
            l.emblem = "";
            l.title = "Солдат";
            break;
        case "hoodie":
            l.outfit = "hoodie";
            l.top = has(w, "хакер", "hacker") ? 0xFF1E1E24 : l.top;
            l.title = "Худи";
            break;
        case "suit":
            l.outfit = "suit";
            l.top = 0xFF1E1E24;
            l.accent = new int[] { 0xFFC62828, 0xFF2C5BD6, 0xFF6E1423 }[variant % 3];
            l.pants = 0xFF1E1E24;
            l.shoes = 0xFF16161D;
            l.emblem = "";
            l.title = "Агент";
            break;
        case "chef":
        case "doctor":
            l.outfit = "coat";
            l.top = 0xFFF1EEE6;
            l.accent = l.theme.equals("chef") ? 0xFFC62828 : 0xFF5AB4F0;
            l.pants = 0xFF26232B;
            l.emblem = "";
            l.title = l.theme.equals("chef") ? "Повар" : "Доктор";
            break;
        case "sport":
            l.outfit = "jersey";
            l.emblem = "";
            l.title = "Спортсмен";
            break;
        default:
            l.title = l.girl ? "Девушка" : "Игрок";
            break;
        }

        // the colours said in words win: the first for the clothes, the second for the accent
        List<Integer> clothes = clothesColors(w);
        if (!clothes.isEmpty() && ("cyber".equals(l.theme) || "vampire".equals(l.theme) || "demon".equals(l.theme))) {
            // dark looks keep their dark clothes; the colour said lights them up (neon lines, the lining, the fire)
            l.accent = clothes.get(0);
            if ("cyber".equals(l.theme)) {
                l.eyes = l.accent;
            }
            clothes.clear();
        }
        if (!clothes.isEmpty()) {
            int c = clothes.get(0);
            if ("armor".equals(l.outfit)) {
                l.metal = c;
            } else {
                l.top = variant == 0 ? c : mix(c, hueShift(c, variant * 12f - 18), 0.35f);
            }
            if (clothes.size() > 1) {
                l.accent = clothes.get(1);
                if (!"suit".equals(l.outfit) && !"ninja".equals(l.outfit)) {
                    l.pants = shade(clothes.get(1), 0.75f);
                }
            }
            if ("ninja".equals(l.theme) && clothes.size() == 1) {
                l.accent = c;  // "ниндзя с красной повязкой"
                l.top = 0xFF1E1E24;
            }
        }
        int hair = colorNear(w, "волос", "hair", "прическ");
        if (has(w, "рыж", "ginger", "redhead")) {
            hair = 0xFFD9632B;
        } else if (has(w, "блонд", "blond")) {
            hair = 0xFFE8CF7A;
        } else if (has(w, "брюнет")) {
            hair = 0xFF2B1B12;
        }
        if (hair != 0) {
            l.hair = hair;
        }
        int eyes = colorNear(w, "глаз", "eye");
        if (eyes != 0) {
            l.eyes = eyes;
        }
        int skin = colorNear(w, "кож", "skin");
        if (skin != 0) {
            l.skin = skin;
        }
        for (String[] e : EMBLEMS) {
            if (has(w, e[0])) {
                l.emblem = e[1];
                break;
            }
        }
        if (has(w, "длинн", "long")) {
            l.hairStyle = "long";
        } else if (has(w, "лыс", "bald")) {
            l.hairStyle = "bald";
        } else if (has(w, "ирокез", "mohawk")) {
            l.hairStyle = "mohawk";
        }
        if (has(w, "бород", "beard")) {
            l.beard = true;
        }
        if (has(w, "капюшон", "hood")) {
            l.head = "hood";
        }
        if (has(w, "шлем", "helmet")) {
            l.head = "helmet";
        }
        if (has(w, "повязк", "headband")) {
            l.head = "ninja".equals(l.theme) ? "mask" : "headband";
        }
        if (has(w, "корон", "crown")) {
            l.head = "crown";
        }
        if (has(w, "рог", "horn")) {
            l.head = "horns";
        }
        if (has(w, "светящ", "glow")) {
            l.glowEyes = true;
        }
        if (o != null) {
            applyOptions(l, o, variant);
        }
        return l;
    }

    /** the buttons win over the words */
    private static void applyOptions(Look l, Options o, int variant) {
        if (o.gender == 2 && !"princess".equals(l.theme) && ("bald".equals(l.hairStyle) || "mohawk".equals(l.hairStyle) || "spiky".equals(l.hairStyle))) {
            l.hairStyle = variant % 2 == 0 ? "long" : "bun";
        }
        if (o.gender == 1) {
            if ("princess".equals(l.theme)) {
                l.outfit = "robe";
                l.head = "crown";
                l.title = "Принц";
            }
            if ("Девушка".equals(l.title)) {
                l.title = "Игрок";
            }
        } else if (o.gender == 2 && "Игрок".equals(l.title)) {
            l.title = "Девушка";
        }
        if (o.clothes != 0) {
            int c = o.clothes;
            if ("armor".equals(l.outfit)) {
                l.metal = c;
            } else if ("cyber".equals(l.theme) || "vampire".equals(l.theme) || "demon".equals(l.theme) || "ninja".equals(l.theme)) {
                l.accent = c;  // dark looks keep their dark clothes; the colour lights them up
                if ("cyber".equals(l.theme)) {
                    l.eyes = c;
                }
            } else {
                l.top = variant == 0 ? c : mix(c, hueShift(c, variant * 12f - 18), 0.35f);
            }
        }
        if (o.hair != 0) {
            l.hair = o.hair;
        }
        if (o.eyes != 0) {
            l.eyes = o.eyes;
        }
        if (o.skin != 0) {
            l.skin = o.skin;
        }
        if (o.emblem != null) {
            l.emblem = o.emblem;
        }
        if (o.hairStyle != null) {
            l.hairStyle = o.hairStyle;
        }
    }

    /** a few bright, pretty base colours for random looks */
    private static final class Color {
        static final int[] HUE = { 0xC62828, 0x2C5BD6, 0x3E9B47, 0x7B3FC4, 0xEF7C1E, 0x23B5B5, 0x6E1423, 0x1E1E24, 0xEC6FA8, 0xE0A526 };
    }

    /* ------------------------------------------------------------ emblems: 5x5 pictures for the chest and capes */

    /** the emblems a button can choose, and their 5x5 pictures (the buttons draw them) */
    static final String[] EMBLEM_IDS = { "heart", "star", "bolt", "moon", "flame", "diamond", "crown", "skull", "creeper", "v" };

    static String[] emblemRows(String id) {
        switch (id) {
        case "heart": return new String[] { ".#.#.", "#####", "#####", ".###.", "..#.." };
        case "star": return new String[] { "..#..", ".###.", "#####", ".###.", ".#.#." };
        case "skull": return new String[] { ".###.", "#####", "#.#.#", "#####", ".#.#." };
        case "creeper": return new String[] { "##.##", "##.##", "..#..", ".###.", ".#.#." };
        case "bolt": return new String[] { "...##", "..##.", ".####", "..##.", ".##.." };
        case "moon": return new String[] { ".###.", "##...", "##...", "##...", ".###." };
        case "flame": return new String[] { "..#..", ".##..", ".###.", "#####", ".###." };
        case "diamond": return new String[] { ".###.", "#####", ".###.", "..#..", "....." };
        case "crown": return new String[] { "#.#.#", "#####", "#####", ".....", "....." };
        case "v": return new String[] { "#...#", "#...#", ".#.#.", ".#.#.", "..#.." };
        default: return null;
        }
    }

    /* ------------------------------------------------------------ skins */

    static List<Skin> skins(String prompt, int count, long seed) {
        return skins(prompt, count, seed, null);
    }

    static List<Skin> skins(String prompt, int count, long seed, Options options) {
        List<Skin> out = new ArrayList<Skin>();
        if (prompt == null) {
            prompt = "";
        }
        for (int i = 0; i < count; i++) {
            Random rnd = new Random(seed * 31 + i * 7919L + prompt.hashCode());
            Look l = look(prompt, rnd, i, options);
            out.add(new Skin(paint(l, rnd), l.girl, l.title + " #" + (i + 1)));
        }
        return out;
    }

    private static Bitmap paint(final Look l, Random rnd) {
        final Tex t = new Tex(64, 64, rnd);
        final int armW = l.girl ? 3 : 4;
        paintHead(t, l);
        paintBody(t, l);
        paintArm(t, l, 40, 16, 40, 32, armW, true);
        paintArm(t, l, 32, 48, 48, 48, armW, false);
        paintLeg(t, l, 0, 16, 0, 32, true);
        paintLeg(t, l, 16, 48, 0, 48, false);
        return t.bitmap();
    }

    private static void paintHead(final Tex t, final Look l) {
        final boolean skull = "skeleton".equals(l.outfit);
        final int skin = l.skin, hair = l.hair;
        final String style = l.hairStyle;
        t.box(0, 0, 8, 8, 8, new Painter() {
            public int at(int face, int x, int y, int w, int h) {
                if (face == TOP) {
                    if ("bald".equals(style) || skull) {
                        return skin;
                    }
                    if ("mohawk".equals(style)) {
                        return x == 3 || x == 4 ? hair : skin;
                    }
                    return (x + y) % 5 == 0 ? shade(hair, 1.15f) : hair;
                }
                if (face == BOTTOM) {
                    return shade(skin, 0.8f);
                }
                if (face == BACK) {
                    if ("bald".equals(style) || skull || "mohawk".equals(style) && x != 3 && x != 4) {
                        return skin;
                    }
                    boolean longHair = "long".equals(style) || "bun".equals(style);
                    return y <= (longHair ? 7 : 5) ? ((x * 3 + y) % 7 == 0 ? shade(hair, 0.85f) : hair) : skin;
                }
                if (face == RIGHT || face == LEFT) {
                    if ("bald".equals(style) || skull || "mohawk".equals(style)) {
                        return skin;
                    }
                    boolean longHair = "long".equals(style) || "bun".equals(style);
                    // hair over the top and down the back side; a sideburn near the face
                    int nearFace = face == RIGHT ? x : 7 - x;  // 7 is next to the face
                    if (y <= 1 || longHair && nearFace <= 5 || y <= 3 && nearFace <= 4 || nearFace <= 1 && y <= 6) {
                        return hair;
                    }
                    return skin;
                }
                // the face
                if (skull) {
                    if (y >= 3 && y <= 4 && (x == 1 || x == 2 || x == 5 || x == 6)) {
                        return 0xFF1A1A1F;
                    }
                    if (y == 5 && (x == 3 || x == 4)) {
                        return 0xFF3A3A40;
                    }
                    if (y == 6 && x >= 2 && x <= 5) {
                        return x % 2 == 0 ? 0xFF1A1A1F : 0xFFD8D4C8;
                    }
                    return skin;
                }
                if (!"bald".equals(style) && !"mohawk".equals(style)) {
                    if (y == 0) {
                        return hair;
                    }
                    if (y == 1 && ("long".equals(style) || "spiky".equals(style) ? (x != 3 && x != 4 || "spiky".equals(style) && x % 2 == 0) : x <= 1 || x >= 6)) {
                        return hair;
                    }
                    if (y >= 2 && y <= 5 && "long".equals(style) && (x == 0 || x == 7)) {
                        return hair;
                    }
                }
                // eyebrows, eyes, nose, mouth
                if (y == 3 && (x == 1 || x == 2 || x == 5 || x == 6)) {
                    return shade(hair, 0.8f);
                }
                if (y == 4) {
                    if (l.eyepatch && (x == 1 || x == 2)) {
                        return 0xFF16161D;
                    }
                    if (x == 1 || x == 6) {
                        return l.glowEyes ? shade(l.eyes, 1.25f) : 0xFFF4F4F4;
                    }
                    if (x == 2 || x == 5) {
                        return l.eyes;
                    }
                }
                if (l.girl && y == 5 && (x == 1 || x == 6)) {
                    return mix(skin, 0xFFE57A8A, 0.45f);  // blush
                }
                if (y == 5 && (x == 3 || x == 4)) {
                    return shade(skin, 0.9f);
                }
                if (l.beard && y >= 5) {
                    if (y == 6 && (x == 3 || x == 4)) {
                        return shade(hair, 0.6f);
                    }
                    return (x >= 1 && x <= 6) || y == 7 ? hair : skin;
                }
                if (y == 6 && (x == 3 || x == 4)) {
                    return l.girl ? 0xFFC75B6E : shade(skin, 0.72f);
                }
                return skin;
            }
        }, 0.05f);

        // the hat layer: hair volume and what is on the head
        final String head = l.head;
        t.box(32, 0, 8, 8, 8, new Painter() {
            public int at(int face, int x, int y, int w, int h) {
                switch (head) {
                case "hood": {
                    int c = l.top;
                    if (face == FRONT) {
                        if (y == 0 || x == 0 || x == 7 || y == 1 && (x <= 1 || x >= 6)) {
                            return shade(c, 0.8f);
                        }
                        return 0;
                    }
                    return face == BOTTOM ? 0 : c;
                }
                case "helmet": {
                    int c = l.metal;
                    if (face == BOTTOM) {
                        return 0;
                    }
                    if (face == FRONT) {
                        if (y == 3 && x >= 1 && x <= 6) {
                            return 0xFF101014;  // the visor slit
                        }
                        if (y == 5 && (x == 2 || x == 5) || y == 6 && (x == 3 || x == 4)) {
                            return shade(c, 0.5f);
                        }
                    }
                    if (face == TOP && (x == 3 || x == 4)) {
                        return l.accent;  // the plume
                    }
                    return (x + y) % 4 == 0 ? shade(c, 1.15f) : y == 0 && face != TOP ? shade(c, 1.2f) : c;
                }
                case "bandana": {
                    if (face == TOP) {
                        return 0xFFC62828;
                    }
                    if (face == BOTTOM) {
                        return 0;
                    }
                    if (y <= 1) {
                        return (x + y) % 3 == 0 ? 0xFFF4F4F4 : 0xFFC62828;
                    }
                    if (face == BACK && y <= 4 && (x == 3 || x == 4)) {
                        return 0xFFA81F1F;
                    }
                    return 0;
                }
                case "crown":
                    if (face == TOP || face == BOTTOM) {
                        return 0;
                    }
                    if (y == 0) {
                        return x % 2 == 0 ? 0xFFF2C230 : 0;
                    }
                    if (y == 1) {
                        if (face == FRONT && (x == 2 || x == 5)) {
                            return x == 2 ? 0xFFE53935 : 0xFF3B6FD6;
                        }
                        return 0xFFE0A526;
                    }
                    return "long".equals(l.hairStyle) && face == BACK && y >= 2 ? l.hair : 0;
                case "mask":
                    if (face == TOP || face == BOTTOM) {
                        return 0;
                    }
                    if (y == 2) {
                        return l.accent;  // the headband
                    }
                    if (face == BACK && y >= 3 && y <= 5 && (x == 3 || x == 4)) {
                        return shade(l.accent, 0.8f);  // its knot
                    }
                    if (y >= 5) {
                        return (x + y) % 3 == 0 ? shade(l.top, 1.3f) : shade(l.top, 1.1f);
                    }
                    return 0;
                case "tiara":
                    if (face == FRONT && y == 1 && x >= 2 && x <= 5) {
                        return 0xFFF2C230;
                    }
                    if (face == FRONT && y == 0 && (x == 3 || x == 4)) {
                        return x == 3 ? 0xFFEC6FA8 : 0xFFF2C230;
                    }
                    break;
                case "cap":
                    if (face == TOP) {
                        return l.top;
                    }
                    if (face != BOTTOM && y <= 1) {
                        return face == FRONT && y == 1 ? shade(l.top, 0.7f) : l.top;
                    }
                    return 0;
                case "visor":
                    if ((face == FRONT || face == LEFT || face == RIGHT) && y == 4) {
                        return (x % 3 == 0) ? shade(l.accent, 1.2f) : l.accent;
                    }
                    break;
                case "viking":
                    if (face == BOTTOM) {
                        return 0;
                    }
                    if (face == TOP || y <= 2) {
                        return (x + y) % 3 == 0 ? shade(0xFF9AA3AE, 1.15f) : 0xFF9AA3AE;
                    }
                    if (face == FRONT && (x == 3 || x == 4) && y <= 4) {
                        return 0xFF7D858F;
                    }
                    if ((face == RIGHT && x >= 5 || face == LEFT && x <= 2) && y == 3) {
                        return 0xFFEDE3C8;  // the horns' roots
                    }
                    return 0;
                case "space":
                    if (face == BOTTOM) {
                        return 0;
                    }
                    if (face == FRONT && y >= 2 && y <= 6 && x >= 1 && x <= 6) {
                        return x == 2 && y == 3 ? 0xFFBFE6FF : y == 2 || x == 1 ? 0xFF2A3A6A : 0xFF16203F;
                    }
                    return (x + y) % 5 == 0 ? 0xFFDADADA : 0xFFF4F4F4;
                case "headband":
                    if (face != TOP && face != BOTTOM && y == 2) {
                        return l.accent;
                    }
                    break;
                case "horns":
                    if (face == FRONT && y == 0 && (x == 0 || x == 7)) {
                        return 0xFF2B1B12;
                    }
                    if ((face == RIGHT && x == 7 || face == LEFT && x == 0) && y <= 1) {
                        return 0xFF2B1B12;
                    }
                    break;
                case "halo":
                    if (face == TOP && (x == 0 || x == 7 || y == 0 || y == 7)) {
                        return 0xFFF2D46A;
                    }
                    break;
                default:
                    break;
                }
                // hair volume on the hat layer
                if ("long".equals(l.hairStyle) && (face == BACK || face == RIGHT || face == LEFT) && y >= 3 && !head.equals("hood") && !head.equals("helmet")
                    && !head.equals("space")) {
                    int nearFace = face == RIGHT ? x : face == LEFT ? 7 - x : 0;
                    return nearFace <= 5 ? ((x + y) % 4 == 0 ? shade(l.hair, 0.85f) : l.hair) : 0;
                }
                if ("spiky".equals(l.hairStyle) && face == FRONT && y == 0 && x % 2 == 1 && head.isEmpty()) {
                    return shade(l.hair, 1.1f);
                }
                if ("bun".equals(l.hairStyle) && face == BACK && y >= 1 && y <= 3 && x >= 2 && x <= 5 && head.isEmpty()) {
                    return shade(l.hair, 1.1f);
                }
                return 0;
            }
        }, 0.04f);
    }

    /** the clothes of the body, by outfit */
    private static int cloth(Look l, int face, int x, int y, int w, int h, boolean torso) {
        int top = l.top, acc = l.accent;
        switch (l.outfit) {
        case "armor": {
            int m = l.metal;
            if (torso && face == FRONT && (x == 3 || x == 4) && y <= 9) {
                return shade(m, 1.18f);  // the ridge
            }
            if (torso && y == 10) {
                return face == FRONT && (x == 3 || x == 4) ? 0xFFE0A526 : 0xFF4A2C1A;  // the belt and its buckle
            }
            if ((x + y * 2) % 7 == 0) {
                return shade(m, 1.25f);  // glints
            }
            return y == 0 ? shade(m, 1.12f) : m;
        }
        case "ninja":
            if (torso && y == 7) {
                return acc;  // the belt
            }
            return ((x + y) % 4 == 0) ? shade(top, 1.35f) : top;
        case "robe":
            if (torso && face == FRONT && (x == 3 || x == 4)) {
                return acc;  // the trim down the front
            }
            if (torso && (y == 6 || y == 7)) {
                return shade(acc, 0.85f);  // the sash
            }
            if ((x * 3 + y * 5) % 17 == 0) {
                return shade(acc, 1.1f);
            }
            return top;
        case "suit":
            if (torso && face == FRONT) {
                if ((x == 3 || x == 4) && y <= 8) {
                    return y == 0 ? 0xFFF4F4F4 : acc;  // the tie
                }
                if ((x == 2 || x == 5) && y <= 3) {
                    return 0xFFF4F4F4;  // the shirt collar
                }
                if (x == 2 && y >= 5 && y % 3 == 2) {
                    return shade(top, 1.6f);  // buttons
                }
            }
            return top;
        case "pirate":
            if (torso && face == FRONT && x >= 2 && x <= 5 && y <= 6) {
                return x == 2 || x == 5 ? shade(top, 1.2f) : 0xFFF1EEE6;  // the shirt under the coat
            }
            if (torso && (y == 8 || y == 9)) {
                return 0xFFC62828;  // the sash
            }
            if (torso && face == FRONT && (x == 1 || x == 6) && y % 3 == 1) {
                return 0xFFE0A526;  // gold buttons
            }
            return top;
        case "space":
            if (torso && face == FRONT && y >= 3 && y <= 5 && x >= 2 && x <= 5) {
                return new int[] { 0xFFE53935, 0xFF39FF88, 0xFF4FA3E0, 0xFFF2C230 }[(x + y) % 4];  // the control panel
            }
            if (y == 1 || torso && y == 9) {
                return acc;
            }
            return (x + y) % 6 == 0 ? shade(top, 0.93f) : top;
        case "skeleton":
            if (torso) {
                if (face == FRONT && (x == 3 || x == 4)) {
                    return l.skin;  // the spine
                }
                if (face == FRONT && y >= 1 && y <= 8 && y % 2 == 1 && x >= 1 && x <= 6) {
                    return l.skin;  // the ribs
                }
                if (face == BACK && (x == 3 || x == 4)) {
                    return l.skin;
                }
            }
            return top;
        case "zombie":
            if ((x * 7 + y * 3) % 11 == 0) {
                return l.skin;  // holes in the shirt
            }
            if (y == h - 1 && x % 2 == 0) {
                return l.skin;  // a torn hem
            }
            return top;
        case "fur":
            if (torso && face == FRONT && x >= 2 && x <= 5) {
                return 0xFF3A2E26;  // the tunic under the fur
            }
            if (torso && y == 9) {
                return face == FRONT && (x == 3 || x == 4) ? 0xFFC0C6CE : 0xFF2B1B12;
            }
            return (x * 5 + y * 3) % 4 == 0 ? shade(acc, 1.15f) : (x + y) % 3 == 0 ? shade(acc, 0.85f) : acc;
        case "camo": {
            int[] camo = { top, shade(top, 0.75f), 0xFF6B5A3A, shade(top, 1.2f) };
            return camo[((x / 2) * 3 + (y / 2) * 5 + x * y) % 4];
        }
        case "coat":
            if (torso && face == FRONT && (x == 3 || x == 4)) {
                return y <= 1 ? acc : shade(top, 0.9f);
            }
            if (torso && face == FRONT && x == 2 && y % 3 == 1) {
                return 0xFF8A8F98;
            }
            return top;
        case "jersey":
            if (face == BACK && torso && y >= 3 && y <= 7) {
                String[] number = { "##.##", ".#..#", ".#.##", ".#.#.", "###.#" };  // a big 10
                int nx = x - 1;
                if (nx >= 0 && nx < 5 && number[y - 3].charAt(nx) == '#') {
                    return 0xFFF4F4F4;
                }
            }
            if (y == 0 || !torso && y == 3) {
                return 0xFFF4F4F4;
            }
            return top;
        case "dress":
            if (torso && y == 6) {
                return acc;  // the waist
            }
            if (torso && face == FRONT && y <= 1 && x >= 2 && x <= 5) {
                return l.skin;  // the neckline
            }
            return (x + y) % 5 == 0 ? shade(top, 1.12f) : top;
        case "hoodie":
            if (torso && face == FRONT && (x == 3 || x == 4) && y >= 1 && y <= 3) {
                return 0xFFF1EEE6;  // the strings
            }
            if (torso && face == FRONT && y >= 7 && y <= 9 && x >= 1 && x <= 6) {
                return shade(top, 0.82f);  // the pocket
            }
            return top;
        case "jacket":
            if (torso && face == FRONT && (x == 3 || x == 4)) {
                return acc == 0 ? 0xFFF1EEE6 : mix(acc, 0xFFFFFFFF, 0.3f);  // the shirt inside
            }
            if (torso && face == FRONT && (x == 2 || x == 5) && y <= 2) {
                return shade(top, 1.25f);  // lapels
            }
            if (l.belt && torso && y == 10) {
                return face == FRONT && (x == 3 || x == 4) ? 0xFFE0A526 : 0xFF3B2A1E;
            }
            return top;
        default:  // a t-shirt
            if (torso && face == FRONT && y == 0 && (x == 3 || x == 4)) {
                return l.skin;  // the neck
            }
            if (torso && y == 11 && face != TOP && face != BOTTOM) {
                return shade(top, 0.85f);  // the hem
            }
            return top;
        }
    }

    private static void paintBody(final Tex t, final Look l) {
        t.box(16, 16, 8, 12, 4, new Painter() {
            public int at(int face, int x, int y, int w, int h) {
                if (face == TOP) {
                    return x >= 3 && x <= 4 && !"armor".equals(l.outfit) && !"space".equals(l.outfit) ? l.skin : cloth(l, face, x, y, w, h, true);
                }
                return cloth(l, face, x, y, w, h, true);
            }
        }, "armor".equals(l.outfit) ? 0.03f : 0.07f);
        // the emblem on the chest, with a darker shadow under it
        String[] rows = emblemRows(l.emblem);
        if (rows != null) {
            int ex = 16 + 4 + 2, ey = 16 + 4 + 3;  // the front of the body is at (20, 20), 8x12
            int c = "armor".equals(l.outfit) ? 0xFFC62828 : contrast(l.top, l.accent);
            for (int j = 0; j < 5; j++) {
                for (int i = 0; i < 5; i++) {
                    if (rows[j].charAt(i) == '#') {
                        t.set(ex + i - 1, ey + j, j == 0 ? shade(c, 1.15f) : c);
                    }
                }
            }
        }
        // the jacket layer: depth for open coats, robes and fur
        t.box(16, 32, 8, 12, 4, new Painter() {
            public int at(int face, int x, int y, int w, int h) {
                if (face == TOP || face == BOTTOM) {
                    return 0;
                }
                switch (l.outfit) {
                case "jacket":
                case "pirate":
                case "coat":
                    if (face == FRONT && (x <= 1 || x >= 6)) {
                        return shade(l.top, 1.08f);
                    }
                    if (face == RIGHT || face == LEFT || face == BACK) {
                        return y >= 10 ? shade(l.top, 0.95f) : 0;
                    }
                    return 0;
                case "fur":
                    if (face == FRONT && (x <= 1 || x >= 6) || face != FRONT && y <= 3) {
                        return (x + y) % 2 == 0 ? shade(l.accent, 1.2f) : l.accent;
                    }
                    return 0;
                case "robe":
                case "dress":
                    return y >= 9 ? shade(l.top, 1.05f) : 0;
                case "hoodie":
                    return face == BACK && y <= 2 ? shade(l.top, 0.85f) : 0;  // the hood down the back
                case "armor":
                    return face == FRONT && y <= 1 ? shade(l.metal, 1.3f) : 0;
                default:
                    return 0;
                }
            }
        }, 0.05f);
    }

    /** a colour that stands out on the cloth */
    private static int contrast(int top, int accent) {
        float lt = luma(top), la = luma(accent);
        if (Math.abs(lt - la) > 60) {
            return accent;
        }
        return lt > 128 ? 0xFF1E1E24 : 0xFFF4F4F4;
    }

    private static float luma(int c) {
        return ((c >> 16) & 0xFF) * 0.3f + ((c >> 8) & 0xFF) * 0.59f + (c & 0xFF) * 0.11f;
    }

    private static void paintArm(final Tex t, final Look l, int u, int v, int ou, int ov, final int armW, final boolean right) {
        final boolean shortSleeves = "tshirt".equals(l.outfit) && !l.longSleeves || "jersey".equals(l.outfit) || "fur".equals(l.outfit);
        t.box(u, v, armW, 12, 4, new Painter() {
            public int at(int face, int x, int y, int w, int h) {
                if ("skeleton".equals(l.outfit)) {
                    boolean bone = face == FRONT || face == BACK ? x == w / 2 || x == w / 2 - 1 && w > 3 : x == 1 || x == 2;
                    return bone || face == BOTTOM || face == TOP ? l.skin : l.top;
                }
                if (face == BOTTOM) {
                    return l.gloves || "armor".equals(l.outfit) ? shade(l.metal, 0.8f) : shade(l.skin, 0.9f);
                }
                if (face == TOP) {
                    return "armor".equals(l.outfit) ? shade(l.metal, 1.1f) : l.top;
                }
                int hand = 9;  // the hand: the last three rows
                if (y >= hand + 1) {
                    if ("armor".equals(l.outfit) || "space".equals(l.outfit)) {
                        return "space".equals(l.outfit) ? 0xFF8A8F98 : shade(l.metal, 0.85f);  // gauntlets, gloves
                    }
                    return y == 11 ? shade(l.skin, 0.92f) : l.skin;
                }
                if (shortSleeves && y >= 4) {
                    if ("fur".equals(l.outfit) && y >= 7) {
                        return y == 7 || y == 9 ? 0xFF2B1B12 : 0xFF5B3A26;  // leather bracers
                    }
                    return l.skin;
                }
                if (y == hand) {
                    return "armor".equals(l.outfit) ? shade(l.metal, 1.2f) : shade(cloth(l, face, x, y, w, h, false), 0.85f);  // the cuff
                }
                if ("armor".equals(l.outfit)) {
                    return y <= 3 ? cloth(l, face, x, y, w, h, false) : ((x + y) % 2 == 0 ? 0xFF6F7680 : 0xFF525860);  // chain mail
                }
                if ("ninja".equals(l.outfit) && y % 3 == 2) {
                    return shade(l.top, 1.5f);  // the wraps
                }
                if ("space".equals(l.outfit) && y == 5) {
                    return l.accent;
                }
                return cloth(l, face, x, y, w, h, false);
            }
        }, 0.06f);
        // the sleeve layer: shoulder plates for armour, wide cuffs for robes
        t.box(ou, ov, armW, 12, 4, new Painter() {
            public int at(int face, int x, int y, int w, int h) {
                if (face == BOTTOM) {
                    return 0;
                }
                if ("armor".equals(l.outfit) && y <= 3) {
                    return y == 0 || face == TOP ? shade(l.metal, 1.3f) : shade(l.metal, 1.1f);
                }
                if (("robe".equals(l.outfit) || "dress".equals(l.outfit)) && y >= 6 && y <= 9 && face != TOP) {
                    return y == 9 ? l.accent : shade(l.top, 1.05f);
                }
                if ("fur".equals(l.outfit) && y <= 2 && face != TOP) {
                    return (x + y) % 2 == 0 ? shade(l.accent, 1.2f) : l.accent;
                }
                return 0;
            }
        }, 0.04f);
    }

    private static void paintLeg(final Tex t, final Look l, int u, int v, int ou, int ov, final boolean right) {
        t.box(u, v, 4, 12, 4, new Painter() {
            public int at(int face, int x, int y, int w, int h) {
                if ("skeleton".equals(l.outfit)) {
                    boolean bone = x == 1 || x == 2;
                    return bone || face == BOTTOM ? l.skin : l.top;
                }
                int shoe = "dress".equals(l.outfit) ? 10 : 9;
                if (face == BOTTOM) {
                    return shade(l.shoes, 0.6f);
                }
                if (face == TOP) {
                    return l.pants;
                }
                if (y >= shoe + 1 || y == shoe && "pirate".equals(l.outfit) || "viking".equals(l.theme) && y >= 8) {
                    // the shoes, a lighter toe in front
                    if (face == FRONT && y == 11) {
                        return mix(l.shoes, 0xFFFFFFFF, 0.25f);
                    }
                    return y == 11 ? shade(l.shoes, 0.8f) : l.shoes;
                }
                if ("pirate".equals(l.outfit) && y >= 6) {
                    return y == 6 ? shade(l.shoes, 1.3f) : l.shoes;  // tall boots
                }
                if ("dress".equals(l.outfit)) {
                    return y <= 4 ? l.top : l.skin;
                }
                if ("armor".equals(l.outfit)) {
                    return y == 4 || y == 5 ? shade(l.metal, 1.2f) : (x + y) % 5 == 0 ? shade(l.metal, 1.15f) : l.metal;
                }
                if ("camo".equals(l.outfit) || "space".equals(l.outfit) || "ninja".equals(l.outfit)) {
                    return cloth(l, face, x, y, w, h, false);
                }
                if ("robe".equals(l.outfit)) {
                    return y <= 7 ? l.top : shade(l.top, 0.8f);
                }
                int p = l.pants;
                // the inner side of each leg a bit darker, a seam, the knees
                boolean inner = right ? face == LEFT : face == RIGHT;
                if (inner) {
                    p = shade(p, 0.88f);
                }
                if (face == FRONT && (right ? x == 3 : x == 0)) {
                    p = shade(p, 0.9f);
                }
                if (y == 5 && face == FRONT) {
                    p = shade(p, 1.08f);
                }
                return p;
            }
        }, 0.07f);
        // the trousers layer: skirts and robes reach over the legs
        t.box(ou, ov, 4, 12, 4, new Painter() {
            public int at(int face, int x, int y, int w, int h) {
                if (face == TOP || face == BOTTOM) {
                    return 0;
                }
                if ("dress".equals(l.outfit) && y <= 6) {
                    return y == 6 ? l.accent : shade(l.top, 1.06f);
                }
                if ("robe".equals(l.outfit) && y <= 9) {
                    return y == 9 ? l.accent : l.top;
                }
                return 0;
            }
        }, 0.05f);
    }

    /* ------------------------------------------------------------ capes */

    private static final String[] CAPE_STYLES = { "gradient", "emblem", "stripes", "galaxy", "flames", "checker", "border", "diagonal" };

    /** capes (64x32, the Minecraft cape layout) from words: colours, a picture, a pattern */
    static List<Bitmap> capes(String prompt, int count, long seed) {
        return capes(prompt, count, seed, null);
    }

    /** the patterns of a cape a button can choose */
    static final String[] CAPE_PATTERNS = { "gradient", "stripes", "checker", "diagonal", "border", "galaxy", "flames", "ice", "leaves", "rainbow" };

    static List<Bitmap> capes(String prompt, int count, long seed, Options o) {
        String[] w = words(prompt == null ? "" : prompt);
        List<Integer> said = clothesColors(w);
        String emblem = "";
        for (String[] e : EMBLEMS) {
            if (has(w, e[0])) {
                emblem = e[1];
                break;
            }
        }
        String style = "";
        if (has(w, "космос", "галакт", "galaxy", "space", "звезд", "stars", "ночн", "night")) {
            style = "galaxy";
        } else if (has(w, "огон", "огн", "пламя", "fire", "flame", "лава", "lava")) {
            style = "flames";
        } else if (has(w, "полос", "stripe")) {
            style = "stripes";
        } else if (has(w, "клетк", "шахмат", "check")) {
            style = "checker";
        } else if (has(w, "радуг", "rainbow")) {
            style = "rainbow";
        } else if (has(w, "лед", "лёд", "снег", "ice", "snow", "зим", "frost")) {
            style = "ice";
        } else if (has(w, "лес", "природ", "лист", "forest", "leaf", "nature")) {
            style = "leaves";
        }
        List<Bitmap> out = new ArrayList<Bitmap>();
        for (int i = 0; i < count; i++) {
            Random rnd = new Random(seed * 131 + i * 104729L + (prompt == null ? 0 : prompt.hashCode()));
            int main = !said.isEmpty() ? said.get(0) : 0xFF000000 | Color.HUE[rnd.nextInt(Color.HUE.length)];
            if (i > 0 && said.isEmpty()) {
                main = 0xFF000000 | Color.HUE[(rnd.nextInt(Color.HUE.length) + i) % Color.HUE.length];
            }
            if (o != null && o.clothes != 0) {
                main = o.clothes;
            }
            // a second colour that goes with the first: a deeper or a lighter shade of it (or the one that was said)
            int second = said.size() > 1 ? said.get(1) : i % 2 == 0 ? shade(main, 0.55f) : mix(main, 0xFFFFFFFF, 0.35f);
            if (o != null && o.second != 0) {
                second = o.second;
            }
            String s = !style.isEmpty() ? style : CAPE_STYLES[(i * 3 + rnd.nextInt(CAPE_STYLES.length)) % CAPE_STYLES.length];
            if (o != null && o.pattern != null) {
                s = o.pattern;
            }
            String mark = !emblem.isEmpty() ? emblem : (s.equals("emblem") || s.equals("border")) ? EMBLEM_CHOICES[rnd.nextInt(EMBLEM_CHOICES.length - 1)] : "";
            if (o != null && o.emblem != null) {
                mark = o.emblem;
            } else if (o != null && o.pattern != null && emblem.isEmpty()) {
                // one pattern for all four: they differ by the picture on them
                mark = i == 0 ? "" : EMBLEM_CHOICES[(i * 2 + rnd.nextInt(3)) % (EMBLEM_CHOICES.length - 1)];
            }
            out.add(cape(main, second, s, mark, rnd));
        }
        return out;
    }

    private static Bitmap cape(int main, int second, String style, String emblem, Random rnd) {
        Tex t = new Tex(64, 32, rnd);
        int dark = shade(main, 0.55f);
        // the outer side: (1, 1), 10x16
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 10; x++) {
                float g = y / 15f;
                int c;
                switch (style) {
                case "stripes":
                    c = (x / 2) % 2 == 0 ? main : second;
                    break;
                case "checker":
                    c = ((x / 2) + (y / 2)) % 2 == 0 ? main : second;
                    break;
                case "diagonal":
                    c = ((x + y) / 3) % 2 == 0 ? main : second;
                    break;
                case "galaxy": {
                    int space = mix(0xFF0B0B2A, 0xFF2A0B3A, g);
                    c = mix(space, main, (float) (0.25 + 0.2 * Math.sin(x * 0.9 + y * 0.5)));
                    if (rnd.nextInt(14) == 0) {
                        c = rnd.nextInt(3) == 0 ? 0xFFFFF2B0 : 0xFFFFFFFF;  // stars
                    }
                    break;
                }
                case "flames": {
                    float heat = g + (float) Math.sin(x * 1.7 + y * 0.6) * 0.18f;
                    c = heat > 0.85f ? 0xFFFFE06A : heat > 0.6f ? 0xFFFF9A1E : heat > 0.35f ? 0xFFE53935 : mix(0xFF2A0B0B, main, 0.4f);
                    break;
                }
                case "rainbow": {
                    int[] rb = { 0xFFE53935, 0xFFEF7C1E, 0xFFF2C230, 0xFF3E9B47, 0xFF2C5BD6, 0xFF7B3FC4 };
                    c = rb[(y * rb.length) / 16];
                    break;
                }
                case "ice":
                    c = mix(0xFFE8F6FF, 0xFF5AB4F0, g);
                    if ((x * 7 + y * 3) % 11 == 0) {
                        c = 0xFFFFFFFF;
                    }
                    break;
                case "leaves":
                    c = new int[] { 0xFF2E7D32, 0xFF3E9B47, 0xFF66BB6A, 0xFF1B5E20 }[(x * 3 + y * 5 + rnd.nextInt(2)) % 4];
                    break;
                default:  // gradient, emblem, border
                    c = mix(main, shade(second, 0.7f), g * 0.8f);
                    break;
                }
                if ("border".equals(style) && (x == 0 || x == 9 || y == 0 || y == 15)) {
                    c = second;
                }
                float f = 1 + (rnd.nextFloat() - 0.5f) * 0.08f;
                t.set(1 + x, 1 + y, shade(c, f));
            }
        }
        String[] rows = emblemRows(emblem);
        if (rows != null) {
            int ec = contrast(main, second == main ? 0xFFF4F4F4 : second);
            if ("galaxy".equals(style) || "flames".equals(style)) {
                ec = 0xFFF4F4F4;
            }
            for (int j = 0; j < 5; j++) {
                for (int i = 0; i < 5; i++) {
                    if (rows[j].charAt(i) == '#') {
                        t.set(1 + 2 + i + 1, 1 + 5 + j + 1, shade(ec, 0.4f));  // its shadow
                    }
                }
            }
            for (int j = 0; j < 5; j++) {
                for (int i = 0; i < 5; i++) {
                    if (rows[j].charAt(i) == '#') {
                        t.set(1 + 2 + i, 1 + 5 + j, j == 0 ? shade(ec, 1.15f) : ec);
                    }
                }
            }
        }
        // the inner side (12, 1), the edges and the top and bottom
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 10; x++) {
                t.set(12 + x, 1 + y, shade(mix(main, 0xFF101014, 0.55f), 1 + (rnd.nextFloat() - 0.5f) * 0.06f));
            }
            t.set(0, 1 + y, dark);
            t.set(11, 1 + y, dark);
        }
        for (int x = 0; x < 10; x++) {
            t.set(1 + x, 0, shade(main, 1.1f));
            t.set(11 + x, 0, dark);
        }
        return t.bitmap();
    }
}
