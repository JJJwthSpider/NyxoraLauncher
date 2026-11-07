package com.sculkshoot.client;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

/**
 * Touch controls for Minecraft: Java Edition on the phone: a joystick to walk and big buttons for the rest.
 * Written as a PojavLauncher control layout (format version 8, also read by Amethyst and other launchers built on
 * PojavLauncher), so the Java launcher that runs the game shows exactly these buttons.
 */
final class Controls {
    // PojavLauncher's special buttons (ControlData.SPECIALBTN_*) and GLFW key codes
    static final int KEYBOARD = -1, HIDE = -2, MOUSE_LEFT = -3, MOUSE_RIGHT = -4, VIRTUAL_MOUSE = -5, SCROLL_UP = -7, SCROLL_DOWN = -8;
    static final int SPACE = 32, KEY_E = 69, KEY_Q = 81, KEY_T = 84, ESCAPE = 256, F5 = 294, LEFT_SHIFT = 340, LEFT_CONTROL = 341;

    /** one button (or the joystick) in dp, measured from the left or right and the top or bottom edge of the screen */
    static final class Button {
        String name;
        int key;
        boolean right;
        boolean bottom;
        float x;
        float y;
        float width;
        float height;
        boolean toggle;
        boolean round;
        boolean inMenus;  // also shown in the inventory and menus
        boolean joystick;

        Button(String name, int key, boolean right, boolean bottom, float x, float y, float width, float height) {
            this.name = name;
            this.key = key;
            this.right = right;
            this.bottom = bottom;
            this.x = x;
            this.y = y;
            this.width = width;
            this.height = height;
        }

        Button round() {
            round = true;
            return this;
        }

        Button inMenus() {
            inMenus = true;
            return this;
        }

        Button toggle(boolean on) {
            toggle = on;
            return this;
        }

        /** the left edge on a screen of this width (dp), for the preview */
        float left(float screenWidth) {
            return right ? screenWidth - width - x : x;
        }

        float top(float screenHeight) {
            return bottom ? screenHeight - height - y : y;
        }
    }

    int size = 100;       // percent
    int opacity = 80;     // percent
    boolean leftHanded;   // joystick on the right, buttons on the left
    boolean sneakToggle = true;
    boolean sprintToggle;

    /** the buttons with the chosen size and side */
    List<Button> buttons() {
        float s = size / 100f;
        List<Button> b = new ArrayList<Button>();
        // walking: the joystick (swipe up past the edge to keep running forward) and sprint
        Button stick = new Button("Джойстик", 0, false, true, 26, 26, 140 * s, 140 * s);
        stick.joystick = true;
        b.add(stick);
        b.add(new Button("Бег", LEFT_CONTROL, false, true, 26 + 140 * s + 10, 26 + 90 * s, 64 * s, 42 * s).toggle(sprintToggle));
        // the main actions under the right thumb
        float big = 76 * s;
        float mid = 62 * s;
        float gap = 12;
        b.add(new Button("Прыжок", SPACE, true, true, 28, 28, big, big).round());
        b.add(new Button("ЛКМ", MOUSE_LEFT, true, true, 28 + big + gap, 28 + (big - mid) / 2, mid, mid).round().inMenus());
        b.add(new Button("ПКМ", MOUSE_RIGHT, true, true, 28 + (big - mid) / 2, 28 + big + gap, mid, mid).round().inMenus());
        b.add(new Button("Присесть", LEFT_SHIFT, true, true, 28 + big + gap, 28 + big + gap, mid, mid).round().toggle(sneakToggle));
        // switching the item in the hand
        b.add(new Button("◀ Вещь", SCROLL_UP, true, true, 28 + big + gap, 28 + big + gap + mid + gap, mid, 40 * s));
        b.add(new Button("Вещь ▶", SCROLL_DOWN, true, true, 28 + (big - mid) / 2, 28 + big + gap + mid + gap, mid, 40 * s));
        // the top row: pause, chat, camera on one side; inventory, drop, keyboard, mouse on the other.
        // It stays narrow enough for small phones (about 600 dp) even when the big buttons grow.
        float ts = Math.min(s, 1.1f);
        float w = 64 * ts;
        float h = 38 * ts;
        String[] leftNames = { "Пауза", "Чат", "Камера" };
        int[] leftKeys = { ESCAPE, KEY_T, F5 };
        for (int i = 0; i < leftNames.length; i++) {
            Button top = new Button(leftNames[i], leftKeys[i], false, false, 12 + i * (w + 6), 10, w, h);
            if (leftKeys[i] == ESCAPE) {
                top.inMenus();
            }
            b.add(top);
        }
        // hides every button for a clean screenshot; tap the same spot again to bring them back
        b.add(new Button("Скрыть", HIDE, false, false, 12, 10 + h + 6, w, h * 0.8f).inMenus());
        String[] rightNames = { "Инвентарь", "Выбросить", "Клавиатура", "Мышь" };
        int[] rightKeys = { KEY_E, KEY_Q, KEYBOARD, VIRTUAL_MOUSE };
        float rw = 80 * ts;
        for (int i = 0; i < rightNames.length; i++) {
            Button top = new Button(rightNames[i], rightKeys[i], true, false, 12 + i * (rw + 6), 10, rw, h);
            if (rightKeys[i] != KEY_Q) {
                top.inMenus();
            }
            b.add(top);
        }
        if (leftHanded) {
            for (Button button : b) {
                button.right = !button.right;
            }
        }
        return b;
    }

    /** the layout file for PojavLauncher and the launchers built on it */
    String toPojavJson() throws JSONException {
        JSONArray buttons = new JSONArray();
        JSONArray joysticks = new JSONArray();
        for (Button b : buttons()) {
            JSONObject o = new JSONObject();
            o.put("name", b.joystick ? "" : b.name);
            o.put("keycodes", new JSONArray().put(b.key).put(0).put(0).put(0));
            // positions are expressions PojavLauncher computes for each screen: ${right} = screen width - button width
            o.put("dynamicX", b.right ? "${right} - px(" + num(b.x) + ")" : "px(" + num(b.x) + ")");
            o.put("dynamicY", b.bottom ? "${bottom} - px(" + num(b.y) + ")" : "px(" + num(b.y) + ")");
            o.put("width", round1(b.width));
            o.put("height", round1(b.height));
            o.put("opacity", opacity / 100.0);
            o.put("bgColor", b.joystick ? 0x33000000 : 0x66000000);
            o.put("strokeColor", 0xFFFFFFFF);
            o.put("strokeWidth", 1.5);
            o.put("cornerRadius", b.round || b.joystick ? 50 : 30);
            o.put("isToggle", b.toggle);
            o.put("passThruEnabled", false);
            o.put("isSwipeable", b.key == MOUSE_LEFT || b.key == MOUSE_RIGHT);
            o.put("displayInGame", true);
            o.put("displayInMenu", b.inMenus);
            if (b.joystick) {
                o.put("forwardLock", true);
                o.put("absolute", false);
                joysticks.put(o);
            } else {
                buttons.put(o);
            }
        }
        JSONObject layout = new JSONObject();
        layout.put("version", 8);
        layout.put("scaledAt", 100);
        layout.put("mControlDataList", buttons);
        layout.put("mDrawerDataList", new JSONArray());
        layout.put("mJoystickDataList", joysticks);
        return layout.toString(2);
    }

    private static String num(float v) {
        return String.valueOf(round1(v));
    }

    private static double round1(float v) {
        return Math.round(v * 10) / 10.0;
    }

    /* ------------------------------------------------------------ the chosen options are remembered */

    static Controls load(android.content.Context context) {
        android.content.SharedPreferences p = context.getSharedPreferences("controls", android.content.Context.MODE_PRIVATE);
        Controls c = new Controls();
        c.size = p.getInt("size", c.size);
        c.opacity = p.getInt("opacity", c.opacity);
        c.leftHanded = p.getBoolean("leftHanded", c.leftHanded);
        c.sneakToggle = p.getBoolean("sneakToggle", c.sneakToggle);
        c.sprintToggle = p.getBoolean("sprintToggle", c.sprintToggle);
        return c;
    }

    void save(android.content.Context context) {
        context.getSharedPreferences("controls", android.content.Context.MODE_PRIVATE).edit()
            .putInt("size", size)
            .putInt("opacity", opacity)
            .putBoolean("leftHanded", leftHanded)
            .putBoolean("sneakToggle", sneakToggle)
            .putBoolean("sprintToggle", sprintToggle)
            .apply();
    }
}
