package com.sculkshoot.client;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Canvas;
import android.graphics.Paint;

import java.util.ArrayList;
import java.util.List;

/**
 * The wardrobe items of Vellora, like the cosmetics of Millida Launcher: things on the head, on the body, on the back,
 * pets and emotes. They are pixel art drawn in code on the character in the launcher (in the game only the real
 * Minecraft capes of the account are seen). The figure is 16x32 "skin pixels": the head is x 4..12, y 0..8, the body
 * x 4..12, y 8..20, the arms x 0..4 and 12..16, the legs y 20..32; items may draw outside it.
 */
final class Cosmetics {
    private Cosmetics() {}

    static final String HEAD = "head", BODY = "body", BACK = "back", PET = "pet", EMOTE = "emote";

    static final class Item {
        final String id, slot, title;

        Item(String id, String slot, String title) {
            this.id = id;
            this.slot = slot;
            this.title = title;
        }
    }

    static final Item[] ITEMS = {
        new Item("crown", HEAD, "Корона"), new Item("horns", HEAD, "Рога демона"), new Item("halo", HEAD, "Нимб"),
        new Item("wizard_hat", HEAD, "Шляпа мага"), new Item("cat_ears", HEAD, "Кошачьи уши"), new Item("headphones", HEAD, "Наушники"),
        new Item("shades", HEAD, "Тёмные очки"),
        new Item("scarf", BODY, "Шарф"), new Item("chain", BODY, "Золотая цепь"), new Item("tie", BODY, "Галстук"),
        new Item("bat_wings", BACK, "Крылья летучей мыши"), new Item("angel_wings", BACK, "Крылья ангела"),
        new Item("demon_wings", BACK, "Крылья демона"), new Item("crimson_cape", BACK, "Багровая мантия"),
        new Item("cube_pet", PET, "Тессеракт"), new Item("bat_pet", PET, "Летучая мышь"), new Item("slime_pet", PET, "Слизень"),
        new Item("ghost_pet", PET, "Призрак"),
        new Item("wave", EMOTE, "Помахать"), new Item("jump", EMOTE, "Прыжок"), new Item("dance", EMOTE, "Танец"),
    };

    /** the outfits: whole sets, like Millida's "Образы" */
    static final String[][] SETS = {
        { "Демон-повелитель", "horns", "demon_wings", "scarf", "" },
        { "Падший ангел", "halo", "angel_wings", "chain", "" },
        { "Маг тессеракта", "wizard_hat", "crimson_cape", "chain", "cube_pet" },
        { "Тень ночи", "shades", "bat_wings", "tie", "bat_pet" },
        { "Король", "crown", "crimson_cape", "chain", "slime_pet" },
    };

    static Item find(String id) {
        for (Item item : ITEMS) {
            if (item.id.equals(id)) {
                return item;
            }
        }
        return null;
    }

    static List<Item> inSlot(String slot) {
        List<Item> out = new ArrayList<Item>();
        for (Item item : ITEMS) {
            if (item.slot.equals(slot)) {
                out.add(item);
            }
        }
        return out;
    }

    /* ------------------------------------------------------------ what is worn */

    private static SharedPreferences prefs(Context context) {
        return context.getSharedPreferences("wardrobe", Context.MODE_PRIVATE);
    }

    static String worn(Context context, String slot) {
        return prefs(context).getString(slot, "");
    }

    static void wear(Context context, String slot, String id) {
        prefs(context).edit().putString(slot, id == null ? "" : id).apply();
    }

    static void wearSet(Context context, String[] set) {
        wear(context, HEAD, set[1]);
        wear(context, BACK, set[2]);
        wear(context, BODY, set[3]);
        wear(context, PET, set[4]);
    }

    /* ------------------------------------------------------------ drawing */

    /** draws in skin pixels: (x, y) is relative to the figure's top left corner, `u` is one pixel on screen */
    static final class Pen {
        final Canvas canvas;
        final float ox, oy, u;
        final Paint paint = new Paint();

        Pen(Canvas canvas, float ox, float oy, float u) {
            this.canvas = canvas;
            this.ox = ox;
            this.oy = oy;
            this.u = u;
        }

        void rect(float x, float y, float w, float h, int color) {
            paint.setColor(color);
            canvas.drawRect(ox + x * u, oy + y * u, ox + (x + w) * u, oy + (y + h) * u, paint);
        }

        /** a picture from strings: every character is a pixel; ' ' and '.' are empty, the others index `colors` */
        void sprite(float x, float y, String[] rows, String keys, int[] colors) {
            for (int j = 0; j < rows.length; j++) {
                for (int i = 0; i < rows[j].length(); i++) {
                    int k = keys.indexOf(rows[j].charAt(i));
                    if (k >= 0) {
                        rect(x + i, y + j, 1, 1, colors[k]);
                    }
                }
            }
        }

        void glow(float x, float y, float radius, int color) {
            paint.setColor(color);
            canvas.drawCircle(ox + x * u, oy + y * u, radius * u, paint);
        }
    }

    private static final int GOLD = 0xFFE6B43C, GOLD_DARK = 0xFF9C6E1E, RUBY = 0xFFE8364F, BLACK = 0xFF120306, WHITE = 0xFFF4EEF0;

    /** things behind the figure: wings and capes (`flap` 0..1 moves the wings) */
    static void drawBack(Pen p, String id, float flap, boolean fromBehind) {
        if (id.isEmpty()) {
            return;
        }
        if (id.equals("crimson_cape")) {
            if (fromBehind) {
                p.rect(3, 8, 10, 17, 0xFF7A0F22);
                p.rect(3, 8, 10, 1, 0xFFB01C38);
                p.rect(4, 23, 8, 2, 0xFF5A0A18);
                p.rect(7, 12, 2, 2, GOLD);
            } else {
                p.rect(2, 9, 1, 15, 0xFF7A0F22);   // the edges seen beside the body
                p.rect(13, 9, 1, 15, 0xFF7A0F22);
            }
            return;
        }
        int dark, mid, light;
        if (id.equals("angel_wings")) {
            dark = 0xFFC9C2C8;
            mid = 0xFFE8E2E6;
            light = WHITE;
        } else if (id.equals("demon_wings")) {
            dark = 0xFF5A0A18;
            mid = 0xFF9E1630;
            light = RUBY;
        } else {
            dark = 0xFF14070B;
            mid = 0xFF2E1018;
            light = 0xFF4A1A26;
        }
        float lift = flap * 2;
        String[] wing = {
            "....aa",
            "..aabb",
            ".abbbc",
            "abbbcc",
            "abbcc.",
            "abcc..",
            ".bc...",
            ".c....",
        };
        String[] mirrored = new String[wing.length];
        for (int i = 0; i < wing.length; i++) {
            mirrored[i] = new StringBuilder(wing[i]).reverse().toString();
        }
        int[] colors = { light, mid, dark };
        p.sprite(-5, 6 - lift, wing, "abc", colors);   // out from behind the shoulders
        p.sprite(15, 6 - lift, mirrored, "abc", colors);
    }

    static void drawBody(Pen p, String id) {
        if (id.equals("scarf")) {
            p.rect(4, 8, 8, 2, 0xFFB01C38);
            p.rect(4, 8, 8, 1, RUBY);
            p.rect(9, 10, 2, 4, 0xFFB01C38);
            p.rect(9, 13, 2, 1, 0xFF7A0F22);
        } else if (id.equals("chain")) {
            p.rect(5, 9, 1, 1, GOLD);
            p.rect(6, 10, 1, 1, GOLD);
            p.rect(7, 11, 2, 1, GOLD);
            p.rect(9, 10, 1, 1, GOLD);
            p.rect(10, 9, 1, 1, GOLD);
            p.rect(7, 12, 2, 2, RUBY);
        } else if (id.equals("tie")) {
            p.rect(7, 8, 2, 1, BLACK);
            p.rect(7, 9, 2, 5, 0xFF7A0F22);
            p.rect(7, 14, 2, 1, BLACK);
        }
    }

    static void drawHead(Pen p, String id, float time) {
        if (id.equals("crown")) {
            p.sprite(3, -3, new String[] { "a..a..a..a", "aaaaaaaaaa", "abaacaabaa" }, "abc", new int[] { GOLD, RUBY, 0xFF5CB6FF });
            p.rect(3, -1, 10, 1, GOLD_DARK);
        } else if (id.equals("horns")) {
            int h = 0xFF2A0A12, t = 0xFF7A0F22;
            p.sprite(1, -5, new String[] { "b.........b", "a.........a", "aa.......aa", ".aa.....aa." }, "ab", new int[] { h, t });
            p.rect(3, -1, 2, 1, h);
            p.rect(11, -1, 2, 1, h);
        } else if (id.equals("halo")) {
            float bob = (float) Math.sin(time * 2) * 0.4f;
            p.glow(8, -3 + bob, 5, 0x33FFE08A);
            p.rect(4, -4 + bob, 8, 1, GOLD);
            p.rect(3, -3 + bob, 1, 1, GOLD);
            p.rect(12, -3 + bob, 1, 1, GOLD);
            p.rect(4, -2 + bob, 8, 1, GOLD_DARK);
        } else if (id.equals("wizard_hat")) {
            int a = 0xFF5A0A18, b = 0xFF8E1428;
            p.sprite(2, -9, new String[] { "......ab....", ".....aab....", ".....aabb...", "....aaabb...", "....aaabbb..",
                                           "...aaaabbb..", "...aaaaabbb.", "..aaaaaabbb.", "cccccccccccc", "dddddddddddd" },
                     "abcd", new int[] { a, b, GOLD, BLACK });
        } else if (id.equals("cat_ears")) {
            int f = 0xFF1A0A0E, in = 0xFFE87A90;
            p.sprite(3, -3, new String[] { "a........a", "aa......aa", "aba....aba" }, "ab", new int[] { f, in });
        } else if (id.equals("headphones")) {
            p.rect(3, -1, 10, 1, BLACK);
            p.rect(3, 0, 1, 3, BLACK);
            p.rect(12, 0, 1, 3, BLACK);
            p.rect(2, 3, 2, 3, RUBY);
            p.rect(12, 3, 2, 3, RUBY);
        } else if (id.equals("shades")) {
            p.rect(4, 3, 8, 1, BLACK);
            p.rect(5, 4, 2, 1, BLACK);
            p.rect(9, 4, 2, 1, BLACK);
            p.rect(5, 4, 1, 1, 0xFF3A0D16);
        }
    }

    /** the pet floats by the right foot */
    static void drawPet(Pen p, String id, float time) {
        if (id.isEmpty()) {
            return;
        }
        float hop = (float) Math.abs(Math.sin(time * 3)) * 2;
        float x = 18, y = 24 - hop;
        if (id.equals("cube_pet")) {
            p.glow(x + 2.5f, y + 2.5f, 4, 0x44FF283C);
            p.sprite(x, y, new String[] { ".aaa.", "abbba", "abcba", "abbba", ".aaa." }, "abc", new int[] { RUBY, 0xFF7A0F22, 0xFFFFC8CD });
        } else if (id.equals("bat_pet")) {
            float flap = (float) Math.sin(time * 12);
            y = 14 + (float) Math.sin(time * 2) * 2;
            p.sprite(x, y, new String[] { flap > 0 ? "a.....a" : ".......", flap > 0 ? "aa.b.aa" : "aaabaaa", ".abbba.", "..bcb.." }, "abc",
                     new int[] { 0xFF2E1018, BLACK, RUBY });
        } else if (id.equals("slime_pet")) {
            p.sprite(x, y + 2, new String[] { ".aaaa.", "abbbba", "abcbca", "abbbba", ".aaaa." }, "abc",
                     new int[] { 0xFF7A0F22, 0xFFC81F3A, BLACK });
        } else if (id.equals("ghost_pet")) {
            y = 16 + (float) Math.sin(time * 1.5) * 1.5f;
            p.sprite(x, y, new String[] { ".aaa.", "aaaaa", "abaab", "aaaaa", "aaaaa", "a.a.a" }, "ab", new int[] { 0xCCF4EEF0, BLACK });
        }
    }
}
