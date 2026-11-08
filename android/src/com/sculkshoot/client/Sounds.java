package com.sculkshoot.client;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.SoundPool;

import java.util.HashMap;
import java.util.Map;

/**
 * The interface sounds, like Millida Launcher's: a blip when a tab changes ("nav"), a sweep when a screen opens or
 * closes, a tick, a sparkle when an item is put on, a chord on Play. The sounds are Vellora's own (art/sounds.py), short
 * and quiet; the 🔊 button in the lobby turns them off. The music ("Crimson Rain", a calm loop) plays in the menu and pauses
 * in the game and in the background; 🎵 turns it off.
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

    /* ------------------------------------------------------------ the music */

    private static android.media.MediaPlayer music;
    private static final android.os.Handler FADE = new android.os.Handler(android.os.Looper.getMainLooper());
    private static final float MUSIC_VOLUME = 0.32f;
    private static float musicLevel;

    static boolean musicEnabled(Context context) {
        return context.getSharedPreferences("sound", Context.MODE_PRIVATE).getBoolean("music", true);
    }

    static void setMusicEnabled(Context context, boolean on) {
        context.getSharedPreferences("sound", Context.MODE_PRIVATE).edit().putBoolean("music", on).apply();
        if (on) {
            startMusic(context);
        } else {
            pauseMusic();
        }
    }

    /** "Crimson Rain" (art/sounds.py), quiet, looping, faded in */
    static synchronized void startMusic(Context context) {
        if (!musicEnabled(context)) {
            return;
        }
        try {
            if (music == null) {
                android.content.res.AssetFileDescriptor fd = context.getApplicationContext().getAssets().openFd("music/vellora_theme.ogg");
                music = new android.media.MediaPlayer();
                music.setAudioAttributes(new AudioAttributes.Builder()
                                             .setUsage(AudioAttributes.USAGE_MEDIA)
                                             .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                                             .build());
                music.setDataSource(fd.getFileDescriptor(), fd.getStartOffset(), fd.getLength());
                fd.close();
                music.setLooping(true);
                music.prepare();
            }
            if (!music.isPlaying()) {
                musicLevel = 0;
                music.setVolume(0, 0);
                music.start();
                fadeIn();
            }
        } catch (Exception e) {
            music = null;  // no music on this device: the launcher works on in silence
        }
    }

    private static void fadeIn() {
        FADE.removeCallbacksAndMessages(null);
        FADE.post(new Runnable() {
            public void run() {
                if (music == null) {
                    return;
                }
                musicLevel = Math.min(MUSIC_VOLUME, musicLevel + MUSIC_VOLUME / 25);
                try {
                    music.setVolume(musicLevel, musicLevel);
                } catch (IllegalStateException e) {
                    return;
                }
                if (musicLevel < MUSIC_VOLUME) {
                    FADE.postDelayed(this, 80);
                }
            }
        });
    }

    /** the app goes to the background or the game starts */
    static synchronized void pauseMusic() {
        FADE.removeCallbacksAndMessages(null);
        try {
            if (music != null && music.isPlaying()) {
                music.pause();
            }
        } catch (IllegalStateException e) {
            music = null;
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
