package com.sculkshoot.client;

import android.app.Activity;
import android.content.Intent;

/**
 * The game engine inside Lullaby: in the full build (android/build-full-apk.sh) it is PojavLauncher's engine
 * (Java for Android, LWJGL, the OpenGL bridge) and "Играть" starts Minecraft right here. The light build has no
 * engine; "Играть" then hands the pack to a separate Java launcher as before.
 */
abstract class GameEngine {
    static final int REQUEST_JAVA_FILE = 41;
    /** play() reports this, followed by the Java version, when that Java is not installed yet */
    static final String NEED_JAVA = "NEED_JAVA:";

    private static GameEngine instance;
    private static boolean looked;

    /** the engine, or null in the light build */
    static synchronized GameEngine get() {
        if (!looked) {
            looked = true;
            try {
                instance = (GameEngine) Class.forName("com.sculkshoot.client.PojavEngine").newInstance();
            } catch (Throwable e) {
                instance = null;
            }
        }
        return instance;
    }

    /** the activity is on screen / left the screen (the engine shows its dialogs on it) */
    abstract void attach(Activity activity);

    abstract void detach(Activity activity);

    /** which Java the game of this Minecraft version needs (8, 17 or 21) */
    static int javaFor(String minecraftVersion) {
        if (!minecraftVersion.startsWith("1.")) {
            return 25;  // the new version numbers (26.x) need Java 25
        }
        if (Plan.compareVersions(minecraftVersion, "1.20.5") >= 0) {
            return 21;
        }
        return Plan.compareVersions(minecraftVersion, "1.17") >= 0 ? 17 : 8;
    }

    /** the graphics this phone can use: { id, name } ("auto" first) */
    abstract java.util.List<String[]> graphicsChoices(Activity activity);

    /** the chosen graphics ("auto" unless the player picked one) */
    static String chosenGraphics(Activity activity) {
        return activity.getSharedPreferences("graphics", Activity.MODE_PRIVATE).getString("renderer", "auto");
    }

    static void chooseGraphics(Activity activity, String id) {
        activity.getSharedPreferences("graphics", Activity.MODE_PRIVATE).edit().putString("renderer", id).apply();
    }

    /** the memory for the game in MB the player picked, 0 = automatic */
    static int chosenMemory(Activity activity) {
        return activity.getSharedPreferences("graphics", Activity.MODE_PRIVATE).getInt("memory", 0);
    }

    static void chooseMemory(Activity activity, int mb) {
        activity.getSharedPreferences("graphics", Activity.MODE_PRIVATE).edit().putInt("memory", mb).apply();
    }

    /** the phone's memory in MB */
    static int phoneMemory(Activity activity) {
        android.app.ActivityManager manager = (android.app.ActivityManager) activity.getSystemService(Activity.ACTIVITY_SERVICE);
        android.app.ActivityManager.MemoryInfo info = new android.app.ActivityManager.MemoryInfo();
        manager.getMemoryInfo(info);
        return (int) (info.totalMem / (1024 * 1024));
    }

    /**
     * The memory for one start: what the player picked, or by the size of the installation: 2 GB for vanilla and small
     * packs, about 12 MB more for every mod above 60 (big packs like Insane Craft get 4-6 GB), never more than about
     * half of the phone's memory, so Android does not close the game.
     */
    static int memoryFor(Activity activity, Installation inst) {
        int phone = phoneMemory(activity);
        int cap = Math.max(1024, Math.min(8192, phone * 55 / 100));
        int chosen = chosenMemory(activity);
        if (chosen > 0) {
            return Math.min(chosen, Math.max(cap, phone * 75 / 100));
        }
        int mods = 0;
        for (Mod mod : inst.mods) {
            mods += mod.enabled && Mod.MOD.equals(mod.kind) ? 1 : 0;
        }
        int mb = 2048 + Math.max(0, mods - 60) * 12;
        mb = Math.min(cap, mb);
        return Math.max(1024, mb / 256 * 256);
    }

    /** the log of the last game start (and the crash report, if there was one), or null */
    abstract java.io.File gameLog();

    /** whether Java of at least this version is installed in the app */
    abstract boolean hasJava(int major);

    /** the installed Java versions, for the user ("Java 17, Java 21"), or "" */
    abstract String installedJava();

    /** the Java launchers on the phone whose Java can be taken over (their names) */
    abstract java.util.List<String> javaSources(Activity activity);

    /** takes Java from an installed PojavLauncher-like app; returns what was installed */
    abstract String importJavaFromApp(Activity activity, String appName) throws Exception;

    /** takes Java from a file the user picked: a runtime .tar.xz or a PojavLauncher .apk */
    abstract String importJavaFromFile(Activity activity, Intent pickResult) throws Exception;

    /**
     * Prepares an installation (mods, the loader, the profile, the account, the controls) and starts Minecraft.
     * Runs its slow parts in the background and reports through the listener; returns at once.
     */
    abstract void play(MainActivity activity, Installation inst, Account account, Builder.Listener log, Done done);

    interface Done {
        /** the game is starting (the launcher closes), or an error happened (message for the user) */
        void finished(boolean started, String problem);
    }
}
