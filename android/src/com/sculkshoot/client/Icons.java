package com.sculkshoot.client;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Handler;
import android.os.Looper;
import android.util.LruCache;
import android.widget.ImageView;

import java.net.HttpURLConnection;
import java.net.URL;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** Loads the icons of Modrinth projects in the background and keeps the recent ones in memory. */
final class Icons {
    private Icons() {}

    private static final LruCache<String, Bitmap> CACHE = new LruCache<String, Bitmap>(120);
    private static final ExecutorService POOL = Executors.newFixedThreadPool(4);
    private static final Handler MAIN = new Handler(Looper.getMainLooper());

    /** shows the icon behind url in the view (the fallback until it is there, or if there is none) */
    static void load(final ImageView view, final String url, int fallback) {
        view.setTag(url);
        Bitmap cached = url == null || url.isEmpty() ? null : CACHE.get(url);
        if (cached != null) {
            view.setImageBitmap(cached);
            return;
        }
        view.setImageResource(fallback);
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
                // icons are shown small: keep memory low
                int size = 128;
                return full.getWidth() > size ? Bitmap.createScaledBitmap(full, size, size * full.getHeight() / Math.max(1, full.getWidth()), true) : full;
            } finally {
                c.disconnect();
            }
        } catch (Exception e) {
            return null;
        }
    }
}
