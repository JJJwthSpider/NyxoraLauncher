package com.sculkshoot.client;

import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;

/**
 * Buttons and panels made of blocks, like Millida Launcher's look in Vellora's colours: corners cut by one "pixel" step,
 * a dark outline, a light line along the top edge and a thick dark edge at the bottom, so a button looks like a raised
 * block. Pressed, the bottom edge disappears and the face moves down, as if the block was pushed in.
 */
final class PixelDrawable extends Drawable {
    private final Paint paint = new Paint();
    private final int fill, light, shadow, outline;
    private final float step;     // the size of one pixel of the corner cut and the edges
    private final boolean raised; // a button: a thick bottom edge
    private boolean pressed;

    PixelDrawable(int fill, int light, int shadow, int outline, float step, boolean raised) {
        this.fill = fill;
        this.light = light;
        this.shadow = shadow;
        this.outline = outline;
        this.step = step;
        this.raised = raised;
    }

    /** the crimson main button */
    static PixelDrawable accent(float density) {
        return new PixelDrawable(0xFFE8364F, 0xFFFF7586, 0xFF8E1428, 0xFF1E0408, 2 * density, true);
    }

    /** an ordinary dark button */
    static PixelDrawable plain(float density) {
        return new PixelDrawable(0xFF2B0E15, 0xFF4A1A26, 0xFF12040A, 0xFF060103, 2 * density, true);
    }

    /** a panel or a card: no bottom edge, the outline in the given colour */
    static PixelDrawable panel(int fill, int border, float density) {
        int light = (fill & 0xFF000000) | (Math.min(255, ((fill >> 16) & 0xFF) + 22) << 16) | (Math.min(255, ((fill >> 8) & 0xFF) + 8) << 8)
                    | Math.min(255, (fill & 0xFF) + 12);
        return new PixelDrawable(fill, light, fill, border, 2 * density, false);
    }

    @Override
    public boolean isStateful() {
        return raised;
    }

    @Override
    protected boolean onStateChange(int[] state) {
        boolean now = false;
        for (int s : state) {
            now |= s == android.R.attr.state_pressed;
        }
        if (now != pressed) {
            pressed = now;
            invalidateSelf();
            return true;
        }
        return false;
    }

    /** a rectangle with every corner cut by `cut` */
    private void notched(Canvas canvas, float l, float t, float r, float b, float cut, int color) {
        if (r - l <= 2 * cut || b - t <= 2 * cut) {
            return;
        }
        paint.setColor(color);
        canvas.drawRect(l + cut, t, r - cut, b, paint);
        canvas.drawRect(l, t + cut, r, b - cut, paint);
    }

    @Override
    public void draw(Canvas canvas) {
        Rect b = getBounds();
        float s = Math.max(1, Math.round(step));
        float edge = raised ? 2 * s : 0;          // the bottom edge of a raised button
        float sink = raised && pressed ? edge : 0; // pressed: the face goes down
        float l = b.left, t = b.top + sink, r = b.right, bottom = b.bottom;
        notched(canvas, l, t, r, bottom, 2 * s, outline);                       // the outline
        notched(canvas, l + s, t + s, r - s, bottom - s, s, shadow);            // the dark bottom edge
        notched(canvas, l + s, t + s, r - s, bottom - s - (edge - sink), s, pressed ? shadow : fill);
        if (pressed) {
            notched(canvas, l + 2 * s, t + 2 * s, r - 2 * s, bottom - 2 * s, s, fill);
        }
        paint.setColor(light);                                                   // the light top edge
        canvas.drawRect(l + 2 * s, t + s, r - 2 * s, t + 2 * s, paint);
    }

    @Override
    public boolean getPadding(Rect padding) {
        int s = Math.round(step);
        padding.set(3 * s, 3 * s, 3 * s, (raised ? 5 : 3) * s);
        return true;
    }

    @Override
    public void setAlpha(int alpha) {
        paint.setAlpha(alpha);
    }

    @Override
    public void setColorFilter(ColorFilter filter) {
        paint.setColorFilter(filter);
    }

    @Override
    public int getOpacity() {
        return PixelFormat.TRANSLUCENT;
    }
}
