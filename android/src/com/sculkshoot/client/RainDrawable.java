package com.sculkshoot.client;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.RadialGradient;
import android.graphics.Rect;
import android.graphics.Shader;
import android.graphics.drawable.Drawable;
import android.os.SystemClock;

/**
 * The background of Nyxora Launcher: a dark burgundy night with red rain falling slantwise, small splashes on the
 * ground, a red glow from below and now and then a faint red flash. It animates by itself while it is on screen
 * (about 30 frames a second) and stops when its view is gone.
 */
final class RainDrawable extends Drawable {
    private static final long FRAME_MS = 33;
    private static final float WIND = 0.16f;  // how far a drop moves sideways per pixel of fall

    private final float density;
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final long start = SystemClock.uptimeMillis();
    private int shaderWidth = -1, shaderHeight = -1;
    private Shader sky, glow;
    private boolean scheduled;

    private final Runnable tick = new Runnable() {
        public void run() {
            scheduled = false;
            invalidateSelf();
        }
    };

    RainDrawable(float density) {
        this.density = density;
        paint.setStrokeCap(Paint.Cap.ROUND);
    }

    /** a small integer hash, so every drop keeps its own place, speed and length */
    private static int hash(int a, int b) {
        int h = a * 374761393 + b * 668265263 + 0x2545F491;
        h = (h ^ (h >>> 13)) * 1274126177;
        return (h ^ (h >>> 16)) & 0x7fffffff;
    }

    private static float unit(int a, int b) {
        return hash(a, b) % 10000 / 10000f;
    }

    @Override
    public void draw(Canvas canvas) {
        Rect bounds = getBounds();
        float w = bounds.width();
        float h = bounds.height();
        if (w <= 0 || h <= 0) {
            return;
        }
        if (shaderWidth != bounds.width() || shaderHeight != bounds.height()) {
            shaderWidth = bounds.width();
            shaderHeight = bounds.height();
            sky = new LinearGradient(0, 0, 0, h, new int[] { 0xFF030102, 0xFF0B0305, 0xFF1A050B }, new float[] { 0, 0.55f, 1 },
                                     Shader.TileMode.CLAMP);
            glow = new RadialGradient(w * 0.5f, h * 1.15f, Math.max(w, h) * 0.75f, new int[] { 0x40C8102E, 0x16800A1E, 0x00000000 },
                                      new float[] { 0, 0.5f, 1 }, Shader.TileMode.CLAMP);
        }
        float t = (SystemClock.uptimeMillis() - start) / 1000f;

        paint.setStyle(Paint.Style.FILL);
        paint.setColor(Color.WHITE);
        paint.setShader(sky);
        canvas.drawRect(bounds, paint);
        paint.setShader(glow);
        canvas.drawRect(bounds, paint);
        paint.setShader(null);

        // a faint red flash every few seconds, like far lightning
        float cycle = t % 9.5f;
        if (cycle < 0.35f) {
            int alpha = (int) (40 * (1 - cycle / 0.35f) * (cycle < 0.08f ? cycle / 0.08f : 1));
            canvas.drawColor(Color.argb(alpha, 255, 40, 60));
        }

        // the rain: far drops thin and dim, near drops long and bright
        int drops = (int) Math.min(300, w * h / (1300 * density * density));
        paint.setStyle(Paint.Style.STROKE);
        for (int i = 0; i < drops; i++) {
            boolean near = hash(i, 1) % 100 < 30;
            float speed = h * (near ? 1.5f : 0.95f) * (0.85f + unit(i, 2) * 0.3f);
            float length = (near ? 26 : 14) * density * (0.7f + unit(i, 3) * 0.6f);
            float span = h + length;
            float y = (t * speed + unit(i, 4) * span) % span - length;
            float x = unit(i, 5) * (w + h * WIND) - h * WIND + y * WIND;
            int alpha = near ? 90 + hash(i, 6) % 70 : 35 + hash(i, 6) % 45;  // black-burgundy: a calm rain
            paint.setStrokeWidth((near ? 1.8f : 1.0f) * density);
            paint.setColor(Color.argb(alpha, 255, near ? 52 : 70, near ? 70 : 88));
            canvas.drawLine(x, y, x + length * WIND, y + length, paint);

            // a splash where a near drop just hit the ground
            float hit = (y + length) - h;
            if (near && hit > 0 && hit < speed * 0.25f) {
                float k = hit / (speed * 0.25f);
                paint.setStrokeWidth(1.2f * density);
                paint.setColor(Color.argb((int) (170 * (1 - k)), 255, 90, 105));
                float r = (3 + 9 * k) * density;
                canvas.drawOval(x + length * WIND - r, h - 4 * density - r * 0.3f, x + length * WIND + r, h - 4 * density + r * 0.3f, paint);
            }
        }
        paint.setStyle(Paint.Style.FILL);

        if (!scheduled && getCallback() != null && isVisible()) {
            scheduled = true;
            scheduleSelf(tick, SystemClock.uptimeMillis() + FRAME_MS);
        }
    }

    @Override
    public boolean setVisible(boolean visible, boolean restart) {
        boolean changed = super.setVisible(visible, restart);
        if (!visible) {
            unscheduleSelf(tick);
            scheduled = false;
        } else if (changed) {
            invalidateSelf();
        }
        return changed;
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
        return PixelFormat.OPAQUE;
    }
}
