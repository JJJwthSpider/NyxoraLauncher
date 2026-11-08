package com.sculkshoot.client;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.SoundPool;

import java.util.HashMap;
import java.util.Map;

/**
 * The interface sounds, like Millida Launcher's: a blip when a tab changes ("nav"), a sweep when a screen opens or
 * closes, a tick, a sparkle when an item is put on, a chord on Play. The sounds are Vellora's own (art/sounds.py), short
 * and quiet; the 🔊 button in the lobby turns them off.
 */
final class Sounds {
    private Sounds() {}

    static final String NAV = "nav", OPEN = "open", CLOSE = "close", CLICK = "click", TOGGLE = "toggle", WEAR = "wear", LAUNCH = "launch";
    private static final String[] ALL = { NAV, OPEN, CLOSE, CLICK, TOGGLE, WEAR, LAUNCH };

    private static SoundPool pool;
    private static final Map<String, Integer> ids = new HashMap<String, Integer>();
    private static Context app;
    private static long lastPlay;
    private static String lastName = "";

    static synchronized void init(Context context) {
        app = context.getApplicationContext();
        if (pool != null) {
            return;
        }
        try {
            pool = new SoundPool.Builder()
                       .setMaxStreams(3)
                       .setAudioAttributes(new AudioAttributes.Builder()
                                               .setUsage(AudioAttributes.USAGE_ASSISTANCE_SONIFICATION)
                                               .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                                               .build())
                       .build();
            for (String name : ALL) {
                try {
                    ids.put(name, pool.load(app.getAssets().openFd("sounds/" + name + ".wav"), 1));
                } catch (Exception e) {
                    // a missing or compressed file: that sound stays silent
                }
            }
        } catch (RuntimeException e) {
            pool = null;  // no audio on this device
        }
    }

    static boolean enabled(Context context) {
        return context.getSharedPreferences("sound", Context.MODE_PRIVATE).getBoolean("on", true);
    }

    static void setEnabled(Context context, boolean on) {
        context.getSharedPreferences("sound", Context.MODE_PRIVATE).edit().putBoolean("on", on).apply();
    }

    static void play(String name) {
        if (pool == null || app == null || !enabled(app)) {
            return;
        }
        Integer id = ids.get(name);
        if (id == null) {
            return;
        }
        // the same sound twice within a moment (one tap that opens and switches) is played once
        long now = android.os.SystemClock.uptimeMillis();
        if (name.equals(lastName) && now - lastPlay < 60) {
            return;
        }
        lastName = name;
        lastPlay = now;
        float volume = LAUNCH.equals(name) ? 0.8f : 0.6f;
        pool.play(id, volume, volume, 1, 0, 1f);
    }
}
