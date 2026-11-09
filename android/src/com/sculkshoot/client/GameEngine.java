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
     * packs (3 GB with mods on 1.18 and newer), about 12 MB more for every mod above 60 (big packs like Insane Craft get 4-6 GB), never more than about
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
        // the new loaders (NeoForge, Forge and Fabric on 1.18 and newer) start with more of their own code: 3 GB
        boolean modernModded = mods > 0 && (!inst.minecraftVersion.startsWith("1.") || Plan.compareVersions(inst.minecraftVersion, "1.18") >= 0);
        int mb = (modernModded ? 3072 : 2048) + Math.max(0, mods - 60) * 12;
        mb = Math.min(cap, mb);
        return Math.max(1024, mb / 256 * 256);
    }

    /**
     * The guest's game is the official Minecraft demo (Mojang's free trial: a world for five game days, about 100 minutes,
     * then a new one), with the pack as it is, mods and loader included, like the demo of Prism Launcher on a computer.
     * The demo came in Minecraft 1.3.1: older versions would ignore "--demo" and run the full game, so they get no demo.
     */
    static boolean demoWorks(String version) {
        if (version.startsWith("1.")) {
            return Plan.compareVersions(version, "1.3.1") >= 0;
        }
        java.util.regex.Matcher snapshot = java.util.regex.Pattern.compile("^(\\d\\d)w\\d\\d[a-z]$").matcher(version);
        if (snapshot.find()) {
            return Integer.parseInt(snapshot.group(1)) >= 13;
        }
        return version.matches("^\\d{2}\\.\\d+.*");  // the year-based versions (26.1 and newer)
    }

    /** the game folder of an installation (saves, screenshots, options.txt) */
    abstract java.io.File gameDir(Installation inst);

    /** whether the game is running now (its own process) */
    static boolean gameRunning(Activity activity) {
        android.app.ActivityManager manager = (android.app.ActivityManager) activity.getSystemService(Activity.ACTIVITY_SERVICE);
        java.util.List<android.app.ActivityManager.RunningAppProcessInfo> list = manager.getRunningAppProcesses();
        if (list != null) {
            for (android.app.ActivityManager.RunningAppProcessInfo info : list) {
                if (info.processName != null && info.processName.endsWith(":game")) {
                    return true;
                }
            }
        }
        return false;
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
