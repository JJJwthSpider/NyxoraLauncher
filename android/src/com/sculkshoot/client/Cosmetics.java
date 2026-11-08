package com.sculkshoot.client;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

/**
 * The wardrobe of Vellora, like the cosmetics of Millida Launcher: head, body, back, pets, effects and emotes, every one
 * open for free. The items are pixel art described once in assets/cosmetics.json (written by art/cosmetics.py; the
 * desktop launcher draws the same file) and drawn on the character in the launcher. In the game only the real Minecraft
 * capes of the account are seen.
 */
final class Cosmetics {
    private Cosmetics() {}

    static final String HEAD = "head", BODY = "body", BACK = "back", PET = "pet", HAND = "hand", EFFECT = "effect", EMOTE = "emote";
    static final String[] WEARABLE = { HEAD, BODY, BACK, PET, HAND, EFFECT };

    static final class Layer {
        float x, y;
        String[] rows;
        int[] colors = new int[128];  // by character
        String view, anim;
    }

    static final class Item {
        final String id, slot, title, rarity;
        final List<Layer> layers = new ArrayList<Layer>();
        int[] particleColors;
        int particleCount;
        String motion = "", shape = "";

        Item(String id, String slot, String title, String rarity) {
            this.id = id;
            this.slot = slot;
            this.title = title;
            this.rarity = rarity;
        }
    }

    static final class Outfit {
        final String title, head, body, back, pet, effect, hand;

        Outfit(JSONObject o) {
            title = o.optString("title");
            head = o.optString("head");
            body = o.optString("body");
            back = o.optString("back");
            pet = o.optString("pet");
            effect = o.optString("effect");
            hand = o.optString("hand");
        }
    }

    private static List<Item> items;
    private static List<Outfit> outfits;

    private static synchronized void load(Context context) {
        if (items != null) {
            return;
        }
        items = new ArrayList<Item>();
        outfits = new ArrayList<Outfit>();
        try {
            InputStream in = context.getAssets().open("cosmetics.json");
            JSONObject data = new JSONObject(Http.read(in));
            JSONArray list = data.getJSONArray("items");
            for (int i = 0; i < list.length(); i++) {
                JSONObject o = list.getJSONObject(i);
                Item item = new Item(o.getString("id"), o.getString("slot"), o.getString("title"), o.optString("rarity", "common"));
                JSONArray layers = o.optJSONArray("layers");
                for (int k = 0; layers != null && k < layers.length(); k++) {
                    JSONObject l = layers.getJSONObject(k);
                    Layer layer = new Layer();
                    layer.x = (float) l.getDouble("x");
                    layer.y = (float) l.getDouble("y");
                    JSONArray rows = l.getJSONArray("rows");
                    layer.rows = new String[rows.length()];
                    for (int r = 0; r < rows.length(); r++) {
                        layer.rows[r] = rows.getString(r);
                    }
                    JSONObject colors = l.getJSONObject("colors");
                    for (java.util.Iterator<String> keys = colors.keys(); keys.hasNext();) {
                        String key = keys.next();
                        layer.colors[key.charAt(0)] = color(colors.getString(key));
                    }
                    layer.view = l.optString("view", "front");
                    layer.anim = l.optString("anim", "");
                    item.layers.add(layer);
                }
                JSONObject particles = o.optJSONObject("particles");
                if (particles != null) {
                    JSONArray colors = particles.getJSONArray("colors");
                    item.particleColors = new int[colors.length()];
                    for (int c = 0; c < colors.length(); c++) {
                        item.particleColors[c] = color(colors.getString(c));
                    }
                    item.particleCount = particles.optInt("count", 10);
                    item.motion = particles.optString("motion");
                    item.shape = particles.optString("shape");
                }
                items.add(item);
            }
            JSONArray sets = data.getJSONArray("outfits");
            for (int i = 0; i < sets.length(); i++) {
                outfits.add(new Outfit(sets.getJSONObject(i)));
            }
        } catch (Exception e) {
            // without the file the wardrobe is just empty
        }
    }

    /** #RRGGBB or #RRGGBBAA */
    private static int color(String hex) {
        int rgb = (int) Long.parseLong(hex.substring(1, 7), 16);
        int alpha = hex.length() >= 9 ? Integer.parseInt(hex.substring(7, 9), 16) : 255;
        return (alpha << 24) | rgb;
    }

    static List<Item> items(Context context) {
        load(context);
        return items;
    }

    static List<Outfit> outfits(Context context) {
        load(context);
        return outfits;
    }

    static Item find(Context context, String id) {
        if (id == null || id.isEmpty()) {
            return null;
        }
        for (Item item : items(context)) {
            if (item.id.equals(id)) {
                return item;
            }
        }
        return null;
    }

    static List<Item> inSlot(Context context, String slot) {
        List<Item> out = new ArrayList<Item>();
        for (Item item : items(context)) {
            if (item.slot.equals(slot)) {
                out.add(item);
            }
        }
        return out;
    }

    /** the frame colour of a rarity, like Millida's: grey, blue, purple, gold */
    static int rarityColor(String rarity) {
        if ("legendary".equals(rarity)) {
            return 0xFFFFB43C;
        }
        if ("epic".equals(rarity)) {
            return 0xFFB04CFF;
        }
        if ("rare".equals(rarity)) {
            return 0xFF3C8CFF;
        }
        return 0xFF8A93A0;
    }

    static String rarityName(String rarity) {
        if ("legendary".equals(rarity)) {
            return "Легендарный";
        }
        if ("epic".equals(rarity)) {
            return "Эпический";
        }
        if ("rare".equals(rarity)) {
            return "Редкий";
        }
        return "Обычный";
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

    static void wearOutfit(Context context, Outfit outfit) {
        wear(context, HEAD, outfit.head);
        wear(context, BACK, outfit.back);
        wear(context, BODY, outfit.body);
        wear(context, PET, outfit.pet);
        wear(context, EFFECT, outfit.effect);
        wear(context, HAND, outfit.hand);
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
    }

    /**
     * Draws the layers of an item that show from this side: `behind` true draws the layers seen from the back.
     * time drives the animations (bob, flap, hop, float).
     */
    static void draw(Pen pen, Item item, boolean behind, float time) {
        if (item == null) {
            return;
        }
        for (Layer layer : item.layers) {
            if (!"both".equals(layer.view) && !(behind ? "back" : "front").equals(layer.view)) {
                continue;
            }
            float dy = 0;
            if ("bob".equals(layer.anim)) {
                dy = (float) Math.sin(time * 2) * 0.5f;
            } else if ("flap".equals(layer.anim)) {
                dy = -(float) (0.5 + 0.5 * Math.sin(time * 2.2)) * 2;
            } else if ("hop".equals(layer.anim)) {
                dy = -(float) Math.abs(Math.sin(time * 3)) * 2;
            } else if ("float".equals(layer.anim)) {
                dy = (float) Math.sin(time * 2) * 1.5f;
            }
            for (int j = 0; j < layer.rows.length; j++) {
                String row = layer.rows[j];
                for (int i = 0; i < row.length(); i++) {
                    char c = row.charAt(i);
                    if (c == '.' || c == ' ' || c >= 128 || layer.colors[c] == 0) {
                        continue;
                    }
                    pen.rect(layer.x + i, layer.y + j + dy, 1, 1, layer.colors[c]);
                }
            }
        }
    }

    /** {left, top, right, bottom} of the layers seen from this side, in skin pixels; null if there are none */
    static float[] bounds(Item item, boolean behind) {
        float[] box = null;
        for (Layer layer : item.layers) {
            if (!"both".equals(layer.view) && !(behind ? "back" : "front").equals(layer.view)) {
                continue;
            }
            int width = 0;
            for (String row : layer.rows) {
                width = Math.max(width, row.length());
            }
            float r = layer.x + width, b = layer.y + layer.rows.length;
            if (box == null) {
                box = new float[] { layer.x, layer.y, r, b };
            } else {
                box[0] = Math.min(box[0], layer.x);
                box[1] = Math.min(box[1], layer.y);
                box[2] = Math.max(box[2], r);
                box[3] = Math.max(box[3], b);
            }
        }
        if (box != null && "hop".equals(item.layers.get(0).anim)) {
            box[1] -= 2;  // room for the jump
        }
        return box;
    }

    /** how many pixels an item paints from the front (or from behind): a cape is mostly seen from behind */
    static int pixels(Item item, boolean behind) {
        int n = 0;
        for (Layer layer : item.layers) {
            if (!"both".equals(layer.view) && !(behind ? "back" : "front").equals(layer.view)) {
                continue;
            }
            for (String row : layer.rows) {
                for (int i = 0; i < row.length(); i++) {
                    if (row.charAt(i) != '.') {
                        n++;
                    }
                }
            }
        }
        return n;
    }

    private static float hash(int a, int b) {
        int h = a * 374761393 + b * 668265263 + 0x2545F491;
        h = (h ^ (h >>> 13)) * 1274126177;
        return ((h ^ (h >>> 16)) & 0x7fffffff) % 10000 / 10000f;
    }

    /** the particles of an effect around the figure (rise, fall, orbit, spark) */
    static void drawParticles(Pen pen, Item item, float time) {
        if (item == null || item.particleColors == null) {
            return;
        }
        for (int i = 0; i < item.particleCount; i++) {
            int color = item.particleColors[i % item.particleColors.length];
            float speed = 4 + hash(i, 1) * 5;
            float phase = hash(i, 2) * 40;
            float x, y, alpha;
            if ("orbit".equals(item.motion)) {
                double angle = time * 1.4 + i * Math.PI * 2 / item.particleCount;
                x = 8 + (float) Math.cos(angle) * 12;
                y = 16 + (float) Math.sin(angle) * 4 + (float) Math.sin(time * 2 + i) * 6;
                alpha = 0.6f + 0.4f * (float) Math.sin(angle);
            } else if ("fall".equals(item.motion)) {
                x = -6 + hash(i, 3) * 28;
                y = -8 + (time * speed + phase) % 44;
                alpha = 0.8f;
            } else if ("spark".equals(item.motion)) {
                x = -4 + hash(i, 3) * 24;
                y = -4 + hash(i, 4) * 38;
                alpha = (float) Math.max(0, Math.sin(time * (2 + hash(i, 5) * 3) + phase));
            } else {  // rise
                x = -2 + hash(i, 3) * 20;
                float travel = (time * speed + phase) % 40;
                y = 34 - travel;
                alpha = Math.min(1, travel / 6) * Math.max(0, 1 - travel / 40);
            }
            int a = Math.round(((color >>> 24) & 0xFF) * Math.max(0, Math.min(1, alpha)));
            int c = (a << 24) | (color & 0xFFFFFF);
            if ("heart".equals(item.shape)) {
                pen.rect(x, y, 1, 1, c);
                pen.rect(x + 2, y, 1, 1, c);
                pen.rect(x, y + 1, 3, 1, c);
                pen.rect(x + 1, y + 2, 1, 1, c);
            } else if ("note".equals(item.shape)) {
                pen.rect(x + 1, y, 1.6f, 0.6f, c);
                pen.rect(x + 1, y, 0.6f, 2.4f, c);
                pen.rect(x, y + 2, 1.4f, 1, c);
            } else if ("star".equals(item.shape)) {
                pen.rect(x + 0.6f, y, 0.8f, 2, c);
                pen.rect(x, y + 0.6f, 2, 0.8f, c);
            } else if ("drop".equals(item.shape)) {
                pen.rect(x, y, 0.6f, 1.6f, c);
            } else {
                pen.rect(x, y, 0.8f, 0.8f, c);
            }
        }
    }

    static int parseColor(String hex) {
        return Color.parseColor(hex);
    }
}
