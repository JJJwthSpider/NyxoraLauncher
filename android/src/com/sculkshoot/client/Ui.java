package com.sculkshoot.client;

import android.content.Context;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.util.TypedValue;
import android.view.Gravity;
import android.widget.Button;
import android.widget.TextView;

/** The look of the app in one place: night black and moon white, main buttons wear a see-through rainbow. */
final class Ui {
    private Ui() {}

    static final int BG = 0xFF0E0407, PANEL = 0xFF1C080D, PANEL_LIGHT = 0xFF2B0E15, ACCENT = 0xFFFFFFFF, ACCENT_DARK = 0xFF8A3442,
                     TEXT = 0xFFFBEFF1, HINT = 0xFFD1A3AA, USER_BUBBLE = 0xFF4A1420, LINE = 0xFF5A1E2A, GLASS = 0xE61C080D,
                     DANGER = 0xFFE5534B;
    static final int[] RAINBOW_PASTEL = { 0xFFFF5A6E, 0xFFE8364F, 0xFFC81F3A, 0xFFA8162E, 0xFFC81F3A, 0xFFFF8090 };
    static final int[] RAINBOW = { 0xFFFF3B4F, 0xFFE8364F, 0xFFFF6B7D, 0xFFC0213A, 0xFFA3182F, 0xFF7A1426 };

    static Typeface pixel;

    static Typeface pixel(Context context) {
        if (pixel == null) {
            try {
                pixel = Typeface.createFromAsset(context.getAssets(), "Tiny5-Regular.ttf");
            } catch (RuntimeException e) {
                pixel = Typeface.DEFAULT_BOLD;
            }
        }
        return pixel;
    }

    static int dp(Context context, float value) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, value, context.getResources().getDisplayMetrics());
    }

    static GradientDrawable rounded(Context context, int color, float radius, int stroke) {
        GradientDrawable d = new GradientDrawable();
        d.setColor(color);
        d.setCornerRadius(dp(context, radius));
        if (stroke != 0) {
            d.setStroke(dp(context, 1), stroke);
        }
        return d;
    }

    static GradientDrawable rainbow(Context context, float radius) {
        GradientDrawable d = new GradientDrawable(GradientDrawable.Orientation.LEFT_RIGHT, RAINBOW_PASTEL);
        d.setCornerRadius(dp(context, radius));
        d.setStroke(dp(context, 1), 0xFFFF8A98);
        return d;
    }

    /** a frame with a rainbow edge: the selected card */
    static GradientDrawable rainbowFrame(Context context, float radius) {
        GradientDrawable d = new GradientDrawable(GradientDrawable.Orientation.TL_BR, new int[] { 0x33FF3B4F, 0x33FF6B7D, 0x33A3182F, 0x337A1426 });
        d.setCornerRadius(dp(context, radius));
        d.setStroke(dp(context, 2), 0xFFFF5A6E);
        return d;
    }

    static Button button(Context context, String text, boolean accent) {
        Button b = new Button(context);
        b.setText(text);
        b.setAllCaps(false);
        b.setTypeface(pixel(context));
        b.setTextSize(15);
        b.setTextColor(accent ? Color.WHITE : TEXT);
        b.setBackground(accent ? rainbow(context, 10) : rounded(context, PANEL_LIGHT, 10, LINE));
        b.setPadding(dp(context, 14), dp(context, 6), dp(context, 14), dp(context, 6));
        b.setMinHeight(dp(context, 40));
        b.setMinimumHeight(dp(context, 40));
        return b;
    }

    /** switches a button between the rainbow (chosen) and the plain look */
    static void styleButton(Context context, Button b, boolean accent) {
        b.setTextColor(accent ? Color.WHITE : TEXT);
        b.setBackground(accent ? rainbow(context, 10) : rounded(context, PANEL_LIGHT, 10, LINE));
        b.setPadding(dp(context, 14), dp(context, 6), dp(context, 14), dp(context, 6));
    }

    static TextView label(Context context, String text, float size, int color, boolean pixelFont) {
        TextView t = new TextView(context);
        t.setText(text);
        t.setTextSize(size);
        t.setTextColor(color);
        if (pixelFont) {
            t.setTypeface(pixel(context));
        }
        t.setGravity(Gravity.START | Gravity.CENTER_VERTICAL);
        return t;
    }

    /** 1234567 -> "1,2 млн" */
    static String downloads(long count) {
        if (count >= 1000000) {
            return String.format(java.util.Locale.ROOT, "%.1f млн", count / 1e6).replace('.', ',');
        }
        if (count >= 1000) {
            return (count / 1000) + " тыс.";
        }
        return String.valueOf(count);
    }
}
