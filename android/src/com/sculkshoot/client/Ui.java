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

    static final int BG = 0xFF0A0B12, PANEL = 0xFF14151F, PANEL_LIGHT = 0xFF1F2130, ACCENT = 0xFFFFFFFF, ACCENT_DARK = 0xFF5D6276,
                     TEXT = 0xFFF4F5FB, HINT = 0xFFA3A7BB, USER_BUBBLE = 0xFF2C2F45, LINE = 0xFF3A3E52, GLASS = 0xE614151F,
                     DANGER = 0xFFE5534B;
    static final int[] RAINBOW_PASTEL = { 0xFFFFC4D0, 0xFFFFE0C0, 0xFFFFF4C2, 0xFFCDF3D6, 0xFFC4E4FF, 0xFFDCCFFF };
    static final int[] RAINBOW = { 0xFFFF5C78, 0xFFFFA84C, 0xFFFFE666, 0xFF6CE08C, 0xFF5CB6FF, 0xFFA880FF };

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
        d.setStroke(dp(context, 1), Color.WHITE);
        return d;
    }

    /** a frame with a rainbow edge: the selected card */
    static GradientDrawable rainbowFrame(Context context, float radius) {
        GradientDrawable d = new GradientDrawable(GradientDrawable.Orientation.TL_BR, new int[] { 0x33FF5C78, 0x33FFE666, 0x335CB6FF, 0x33A880FF });
        d.setCornerRadius(dp(context, radius));
        d.setStroke(dp(context, 2), Color.WHITE);
        return d;
    }

    static Button button(Context context, String text, boolean accent) {
        Button b = new Button(context);
        b.setText(text);
        b.setAllCaps(false);
        b.setTypeface(pixel(context));
        b.setTextSize(15);
        b.setTextColor(accent ? BG : TEXT);
        b.setBackground(accent ? rainbow(context, 10) : rounded(context, PANEL_LIGHT, 10, LINE));
        b.setPadding(dp(context, 14), dp(context, 6), dp(context, 14), dp(context, 6));
        b.setMinHeight(dp(context, 40));
        b.setMinimumHeight(dp(context, 40));
        return b;
    }

    /** switches a button between the rainbow (chosen) and the plain look */
    static void styleButton(Context context, Button b, boolean accent) {
        b.setTextColor(accent ? BG : TEXT);
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
