package com.sculkshoot.client;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RadialGradient;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Shader;
import android.os.SystemClock;
import android.view.View;

/**
 * Arkan, Vellora's AI pack builder: the burgundy wizard in a hood, only his head and shoulders (res/drawable-nodpi/wizard_body.png, drawn by
 * art/wizard.py) with a red tesseract turning in four dimensions by his shoulder. The tesseract is projected
 * 4D -> 3D -> 2D every frame with the same numbers as art/wizard.py; it pulses faster while Arkan is working.
 */
final class WizardView extends View {
    private static final float HAND_X = 53f / 64, HAND_Y = 16f / 64, SIZE = 30f / 64, GLOW = 10f / 64;
    private static Bitmap body;

    private final Paint pixels = new Paint();
    private final Paint line = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint dot = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint glow = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final float[] px = new float[16], py = new float[16];
    private final long start = SystemClock.uptimeMillis();
    private boolean busy;

    WizardView(Context context) {
        super(context);
        if (body == null) {
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inScaled = false;
            body = BitmapFactory.decodeResource(context.getResources(), R.drawable.wizard_body, options);
        }
        pixels.setFilterBitmap(false);  // sharp pixel art
        line.setColor(0xFFFF4654);
        line.setStrokeCap(Paint.Cap.ROUND);
        dot.setColor(0xFFFFC8CD);
    }

    /** Arkan is building: the tesseract turns faster and glows brighter */
    void setBusy(boolean busy) {
        this.busy = busy;
        invalidate();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        float side = Math.min(getWidth(), getHeight());
        if (side <= 0 || body == null) {
            return;
        }
        float left = (getWidth() - side) / 2, top = (getHeight() - side) / 2;
        canvas.drawBitmap(body, new Rect(0, 0, body.getWidth(), body.getHeight()), new RectF(left, top, left + side, top + side), pixels);

        float time = (SystemClock.uptimeMillis() - start) / 1000f;
        float turn = time * (busy ? 1.6f : 0.7f);
        float cx = left + side * HAND_X;
        float cy = top + side * HAND_Y + (float) Math.sin(time * 2) * side * 0.012f;  // floats a little
        float pulse = 0.75f + 0.25f * (float) Math.sin(time * (busy ? 7 : 3));
        float glowRadius = side * GLOW * (busy ? 1.3f : 1f);
        glow.setShader(new RadialGradient(cx, cy, glowRadius, new int[] { ((int) (110 * pulse) << 24) | 0xFF283C, 0x00FF283C },
                                          null, Shader.TileMode.CLAMP));
        canvas.drawCircle(cx, cy, glowRadius, glow);

        float size = side * SIZE;
        for (int i = 0; i < 16; i++) {
            float x = (i & 1) != 0 ? 1 : -1, y = (i & 2) != 0 ? 1 : -1, z = (i & 4) != 0 ? 1 : -1, w = (i & 8) != 0 ? 1 : -1;
            double a = turn, b = turn * 0.7;
            float x1 = (float) (x * Math.cos(a) - w * Math.sin(a)), w1 = (float) (x * Math.sin(a) + w * Math.cos(a));
            float y1 = (float) (y * Math.cos(b) - z * Math.sin(b)), z1 = (float) (y * Math.sin(b) + z * Math.cos(b));
            float x2 = (float) (x1 * Math.cos(0.5) - z1 * Math.sin(0.5)), z2 = (float) (x1 * Math.sin(0.5) + z1 * Math.cos(0.5));
            float k4 = 1 / (3 - w1);
            float k3 = 1 / (4 - z2 * k4);
            px[i] = cx + x2 * k4 * k3 * size;
            py[i] = cy + y1 * k4 * k3 * size;
        }
        line.setStrokeWidth(Math.max(1, side / 140));
        line.setAlpha((int) (200 + 55 * pulse));
        for (int i = 0; i < 16; i++) {
            for (int k = 0; k < 4; k++) {
                int j = i ^ (1 << k);
                if (i < j) {
                    canvas.drawLine(px[i], py[i], px[j], py[j], line);
                }
            }
        }
        float r = Math.max(1, side / 110);
        for (int i = 0; i < 16; i++) {
            canvas.drawCircle(px[i], py[i], r, dot);
        }
        postInvalidateDelayed(33);
    }
}
