package com.sculkshoot.client;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PixelFormat;
import android.graphics.RadialGradient;
import android.graphics.Rect;
import android.graphics.Shader;
import android.graphics.drawable.Drawable;

/**
 * The background of the app, like the one of the desktop launcher: the night sky with stars, a see-through
 * rainbow and a big moon with seas and craters.
 */
final class MoonSkyDrawable extends Drawable {
    private static final int[] RAINBOW = { 0xFFA880FF, 0xFF5CB6FF, 0xFF6CE08C, 0xFFFFE666, 0xFFFFA84C, 0xFFFF5C78 };

    private final float density;
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);

    MoonSkyDrawable(float density) {
        this.density = density;
    }

    /** the same small integer hash as the desktop launcher, so the sky looks the same on every start */
    private static int hash(int a, int b, int salt) {
        int h = a * 374761393 + b * 668265263 + salt * (int) 2246822519L;
        h = (h ^ (h >>> 13)) * 1274126177;
        return (h ^ (h >>> 16)) & 0x7fffffff;
    }

    @Override
    public void draw(Canvas canvas) {
        Rect bounds = getBounds();
        float w = bounds.width();
        float h = bounds.height();
        paint.setColor(Color.WHITE);
        paint.setShader(new LinearGradient(0, 0, 0, h, new int[] { 0xFF05060D, 0xFF0B0D1C, 0xFF14172C }, new float[] { 0, 0.6f, 1 },
                                           Shader.TileMode.CLAMP));
        canvas.drawRect(bounds, paint);
        paint.setShader(null);

        // stars
        int stars = (int) (w * h / (2600 * density * density));
        for (int i = 0; i < stars; i++) {
            float x = hash(i, 1, 77) % 10000 / 10000f * w;
            float y = hash(i, 2, 77) % 10000 / 10000f * h;
            paint.setColor(Color.argb(50 + hash(i, 3, 77) % 160, 255, 255, 255));
            canvas.drawCircle(x, y, (hash(i, 4, 77) % 100 < 85 ? 0.7f : 1.2f) * density, paint);
        }

        // a see-through rainbow arching over the bottom
        float cx = w * 0.42f;
        float cy = h * 1.08f;
        float radius = Math.max(w, h) * 0.72f;
        float band = radius * 0.03f;
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(band);
        for (int i = 0; i < RAINBOW.length; i++) {
            paint.setColor((RAINBOW[i] & 0x00FFFFFF) | 0x2E000000);
            canvas.drawCircle(cx, cy, radius - (RAINBOW.length - 1 - i) * band, paint);
        }
        paint.setStyle(Paint.Style.FILL);

        // the big moon with its glow, seas and craters; the paint must be opaque, its alpha multiplies the shaders
        paint.setColor(Color.WHITE);
        float r = Math.min(w, h) * 0.3f;
        float mx = w * 0.7f;
        float my = h * 0.3f;
        paint.setShader(new RadialGradient(mx, my, r * 2.3f, new int[] { 0x3CFFFFFF, 0x1ADCE4FF, 0x00DCE4FF }, new float[] { 0.4f, 0.6f, 1f },
                                           Shader.TileMode.CLAMP));
        canvas.drawCircle(mx, my, r * 2.3f, paint);
        paint.setShader(new RadialGradient(mx - r * 0.35f, my - r * 0.35f, r * 1.45f,
                                           new int[] { 0xFFFBFBFF, 0xFFE2E4EE, 0xFFC4C7D6, 0xFF9A9EB3 }, new float[] { 0, 0.55f, 0.85f, 1 },
                                           Shader.TileMode.CLAMP));
        canvas.drawCircle(mx, my, r, paint);
        paint.setShader(null);
        canvas.save();
        Path disk = new Path();
        disk.addCircle(mx, my, r, Path.Direction.CW);
        canvas.clipPath(disk);
        paint.setColor(Color.WHITE);
        for (int i = 0; i < 7; i++) {
            float sx = mx + (hash(i, 5, 91) % 140 - 70) / 100f * r;
            float sy = my + (hash(i, 6, 91) % 140 - 70) / 100f * r;
            float sr = r * (0.2f + hash(i, 7, 91) % 25 / 100f);
            paint.setShader(new RadialGradient(sx, sy, sr, new int[] { 0x968A8FA6, 0x5F8A8FA6, 0x008A8FA6 }, new float[] { 0, 0.6f, 1 },
                                               Shader.TileMode.CLAMP));
            canvas.drawCircle(sx, sy, sr, paint);
        }
        paint.setShader(null);
        for (int i = 0; i < 18; i++) {
            float x = mx + (hash(i, 8, 92) % 170 - 85) / 100f * r;
            float y = my + (hash(i, 9, 92) % 170 - 85) / 100f * r;
            float cr = r * (0.03f + hash(i, 10, 92) % 10 / 100f);
            paint.setColor(0x967E8298);
            canvas.drawCircle(x, y, cr, paint);
            paint.setColor(0x78FFFFFF);
            canvas.drawCircle(x - cr * 0.25f, y - cr * 0.25f, cr * 0.75f, paint);
            paint.setColor(0x8CA4A8BB);
            canvas.drawCircle(x + cr * 0.12f, y + cr * 0.12f, cr * 0.7f, paint);
        }
        canvas.restore();
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
