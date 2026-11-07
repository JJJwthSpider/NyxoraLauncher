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
        if (!minecraftVersion.startsWith("1.") || Plan.compareVersions(minecraftVersion, "1.20.5") >= 0) {
            return 21;  // 1.20.5+ and the new version numbers (26.x) need Java 21
        }
        return Plan.compareVersions(minecraftVersion, "1.17") >= 0 ? 17 : 8;
    }

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
