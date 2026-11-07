package com.sculkshoot.client;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.BitmapShader;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.graphics.Shader;
import android.os.Handler;
import android.os.Looper;
import android.util.LruCache;
import android.widget.ImageView;

import java.net.HttpURLConnection;
import java.net.URL;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Loads the icons of Modrinth projects in the background and keeps the recent ones in memory. They look like on
 * modrinth.com: the project's own picture in a rounded square, pixel art kept sharp, and Modrinth's grey box
 * for projects without an icon.
 */
final class Icons {
    private Icons() {}

    private static final LruCache<String, Bitmap> CACHE = new LruCache<String, Bitmap>(120);
    private static final ExecutorService POOL = Executors.newFixedThreadPool(4);
    private static final Handler MAIN = new Handler(Looper.getMainLooper());

    private static final int SIZE = 128;
    private static Bitmap placeholder;

    /** a mod, resource pack, shader or modpack icon: the Modrinth placeholder until it is there, or if there is none */
    static void load(ImageView view, String url) {
        load(view, url, 0);
    }

    /** shows the icon behind url in the view (the fallback until it is there, or if there is none; 0 = Modrinth's box) */
    static void load(final ImageView view, final String url, int fallback) {
        view.setTag(url);
        Bitmap cached = url == null || url.isEmpty() ? null : CACHE.get(url);
        if (cached != null) {
            view.setImageBitmap(cached);
            return;
        }
        if (fallback == 0) {
            view.setImageBitmap(placeholder());
        } else {
            view.setImageResource(fallback);
        }
        if (url == null || url.isEmpty()) {
            return;
        }
        POOL.execute(new Runnable() {
            public void run() {
                final Bitmap bitmap = fetch(url);
                if (bitmap == null) {
                    return;
                }
                CACHE.put(url, bitmap);
                MAIN.post(new Runnable() {
                    public void run() {
                        if (url.equals(view.getTag())) {
                            view.setImageBitmap(bitmap);
                        }
                    }
                });
            }
        });
    }

    private static Bitmap fetch(String url) {
        try {
            HttpURLConnection c = (HttpURLConnection) new URL(url).openConnection();
            c.setRequestProperty("User-Agent", Http.USER_AGENT);
            c.setConnectTimeout(15000);
            c.setReadTimeout(20000);
            try {
                BitmapFactory.Options options = new BitmapFactory.Options();
                options.inSampleSize = 1;
                Bitmap full = BitmapFactory.decodeStream(c.getInputStream(), null, options);
                if (full == null) {
                    return null;
                }
                return rounded(full);
            } finally {
                c.disconnect();
            }
        } catch (Exception e) {
            return null;
        }
    }

    private static float radius() {
        return SIZE * 0.2f;  // the corners of icons on modrinth.com
    }

    /** the picture fitted into a SIZE square with rounded corners; small pixel art is scaled without blurring */
    static Bitmap rounded(Bitmap full) {
        Bitmap out = Bitmap.createBitmap(SIZE, SIZE, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(out);
        float scale = Math.min(SIZE / (float) full.getWidth(), SIZE / (float) full.getHeight());
        float w = full.getWidth() * scale;
        float h = full.getHeight() * scale;
        Matrix m = new Matrix();
        m.setScale(scale, scale);
        m.postTranslate((SIZE - w) / 2f, (SIZE - h) / 2f);
        BitmapShader shader = new BitmapShader(full, Shader.TileMode.CLAMP, Shader.TileMode.CLAMP);
        shader.setLocalMatrix(m);
        Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        boolean pixelArt = Math.max(full.getWidth(), full.getHeight()) <= 64;
        paint.setFilterBitmap(!pixelArt);
        paint.setShader(shader);
        canvas.drawRoundRect(new RectF((SIZE - w) / 2f, (SIZE - h) / 2f, (SIZE + w) / 2f, (SIZE + h) / 2f), radius(), radius(), paint);
        return out;
    }

    /** Modrinth's icon for projects without one: a box outline on a grey rounded square */
    static synchronized Bitmap placeholder() {
        if (placeholder != null) {
            return placeholder;
        }
        Bitmap out = Bitmap.createBitmap(SIZE, SIZE, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(out);
        Paint fill = new Paint(Paint.ANTI_ALIAS_FLAG);
        fill.setColor(0xFF2D3036);
        canvas.drawRoundRect(new RectF(0, 0, SIZE, SIZE), radius(), radius(), fill);
        Paint line = new Paint(Paint.ANTI_ALIAS_FLAG);
        line.setStyle(Paint.Style.STROKE);
        line.setStrokeWidth(SIZE * 0.045f);
        line.setStrokeJoin(Paint.Join.ROUND);
        line.setStrokeCap(Paint.Cap.ROUND);
        line.setColor(0xFF96A2B0);
        float c = SIZE / 2f;
        float r = SIZE * 0.26f;   // half the width of the box
        float t = r * 0.55f;      // how far the top face leans
        Path box = new Path();
        box.moveTo(c, c - r - t * 0.2f);
        box.lineTo(c + r, c - t * 0.6f);
        box.lineTo(c + r, c + r * 0.75f);
        box.lineTo(c, c + r + t * 0.35f);
        box.lineTo(c - r, c + r * 0.75f);
        box.lineTo(c - r, c - t * 0.6f);
        box.close();
        box.moveTo(c - r, c - t * 0.6f);
        box.lineTo(c, c + t * 0.2f);
        box.lineTo(c + r, c - t * 0.6f);
        box.moveTo(c, c + t * 0.2f);
        box.lineTo(c, c + r + t * 0.35f);
        canvas.drawPath(box, line);
        placeholder = out;
        return out;
    }
}
