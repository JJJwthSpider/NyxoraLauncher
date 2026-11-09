package com.sculkshoot.client;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.net.Uri;

import net.kdt.pojavlaunch.Architecture;
import net.kdt.pojavlaunch.JavaGUILauncherActivity;
import net.kdt.pojavlaunch.JMinecraftVersionList;
import net.kdt.pojavlaunch.PojavProfile;
import net.kdt.pojavlaunch.Tools;
import net.kdt.pojavlaunch.extra.ExtraConstants;
import net.kdt.pojavlaunch.extra.ExtraCore;
import net.kdt.pojavlaunch.lifecycle.ContextAwareDoneListener;
import net.kdt.pojavlaunch.lifecycle.ContextExecutor;
import net.kdt.pojavlaunch.modloaders.FabriclikeDownloadTask;
import net.kdt.pojavlaunch.modloaders.FabriclikeUtils;
import net.kdt.pojavlaunch.modloaders.ModloaderDownloadListener;
import net.kdt.pojavlaunch.multirt.MultiRTUtils;
import net.kdt.pojavlaunch.multirt.Runtime;
import net.kdt.pojavlaunch.prefs.LauncherPreferences;
import net.kdt.pojavlaunch.progresskeeper.ProgressKeeper;
import net.kdt.pojavlaunch.progresskeeper.ProgressListener;
import net.kdt.pojavlaunch.tasks.AsyncMinecraftDownloader;
import net.kdt.pojavlaunch.tasks.AsyncVersionList;
import net.kdt.pojavlaunch.tasks.MinecraftDownloader;
import net.kdt.pojavlaunch.value.MinecraftAccount;
import net.kdt.pojavlaunch.value.launcherprofiles.LauncherProfiles;
import net.kdt.pojavlaunch.value.launcherprofiles.MinecraftProfile;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.atomic.AtomicReference;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

/**
 * Minecraft inside Lullaby, on PojavLauncher's engine (LGPL-3.0, github.com/PojavLauncherTeam/PojavLauncher):
 * Java for Android, LWJGL and the OpenGL bridge come from it. Lullaby prepares everything the way its menu
 * describes the installation and then asks the engine to download Minecraft and start it.
 */
public final class PojavEngine extends GameEngine {
    /** Java launchers built on PojavLauncher keep the same Java packages inside their APK (name, package) */
    private static final String[][] JAVA_APPS = {
        { "PojavLauncher", "net.kdt.pojavlaunch" },
        { "PojavLauncher (debug)", "net.kdt.pojavlaunch.debug" },
        { "Amethyst", "org.angelauramc.amethyst" },
        { "Zalith Launcher", "com.movtery.zalithlauncher" },
    };
    /** where those packages lie in such an APK, and the name PojavLauncher gives each one */
    private static final String[][] JAVA_PACKS = {
        { "assets/components/jre", "Internal" },
        { "assets/components/jre-new", "Internal-17" },
        { "assets/components/jre-21", "Internal-21" },
    };
    static final String CONTROLS_FILE = "Vellora_controls.json";

    @Override
    void attach(Activity activity) {
        if (Tools.CTRLMAP_PATH == null && Tools.checkStorageRoot(activity)) {
            // Android brought Lullaby back without the start screen that sets the engine up
            LauncherPreferences.loadPreferences(activity);
            net.kdt.pojavlaunch.tasks.AsyncAssetManager.unpackComponents(activity);
            net.kdt.pojavlaunch.tasks.AsyncAssetManager.unpackSingleFiles(activity);
        }
        ContextExecutor.setActivity(activity);
        if (ExtraCore.getValue(ExtraConstants.RELEASE_TABLE) == null) {
            // the list of Minecraft versions, needed to download a version
            new AsyncVersionList().getVersionList(versions -> ExtraCore.setValue(ExtraConstants.RELEASE_TABLE, versions), false);
        }
    }

    @Override
    void detach(Activity activity) {
        ContextExecutor.clearActivity();
    }

    private static final String GL4ES = "opengles2", LTW = "opengles3_ltw", ZINK = "vulkan_zink";

    @Override
    List<String[]> graphicsChoices(Activity activity) {
        List<String[]> out = new ArrayList<String[]>();
        out.add(new String[] { "auto", "Авто — Vellora выберет сам (рекомендуется)" });
        for (String id : Tools.getCompatibleRenderers(activity).rendererIds) {
            if (LTW.equals(id)) {
                out.add(new String[] { id, "LTW (OpenGL ES 3) — для 1.17 и новее, лучше всего на новых телефонах" });
            } else if (GL4ES.equals(id)) {
                out.add(new String[] { id, "Holy GL4ES — быстро, любые версии" });
            } else if (ZINK.equals(id)) {
                out.add(new String[] { id, "Zink (Vulkan) — самый полный OpenGL, медленнее" });
            }
        }
        return out;
    }

    /**
     * The graphics for this start. "Auto": LTW for 1.17+ (Minecraft needs OpenGL 3.2 there, and the newest Adreno
     * chips show only a black screen with GL4ES), GL4ES for older versions or phones without OpenGL ES 3.
     */
    static String rendererFor(Activity activity, Installation inst) {
        List<String> compatible = Tools.getCompatibleRenderers(activity).rendererIds;
        String chosen = chosenGraphics(activity);
        if (!"auto".equals(chosen) && compatible.contains(chosen)) {
            return chosen;
        }
        boolean modern = Plan.compareVersions(inst.minecraftVersion, "1.17") >= 0 || !inst.minecraftVersion.startsWith("1.");
        if (modern && compatible.contains(LTW)) {
            return LTW;
        }
        // old versions: GL4ES is fast, but it cannot translate the GLSL shaders big mod packs bring (the game dies
        // with SIGSEGV in GL4ES' shader converter); Zink runs them as real OpenGL on Vulkan
        if (compatible.contains(ZINK) && (bigModPack(inst) || gl4esCrashed(activity, inst))) {
            return ZINK;
        }
        return compatible.contains(GL4ES) || compatible.isEmpty() ? GL4ES : compatible.get(0);
    }

    private static boolean bigModPack(Installation inst) {
        int mods = 0;
        for (Mod mod : inst.mods) {
            mods += mod.enabled && Mod.MOD.equals(mod.kind) ? 1 : 0;
        }
        return mods >= 40;
    }

    /** the last start of this installation died in GL4ES (remembered, so it uses Zink from now on) */
    private static boolean gl4esCrashed(Activity activity, Installation inst) {
        android.content.SharedPreferences prefs = activity.getSharedPreferences("graphics", Activity.MODE_PRIVATE);
        if (prefs.getBoolean("zink_" + inst.id, false)) {
            return true;
        }
        try {
            File log = new File(Tools.DIR_GAME_HOME, "latestlog.txt");
            if (!log.isFile()) {
                return false;
            }
            String text = Tools.read(log.getAbsolutePath());
            boolean ours = text.contains("custom_instances/lullaby-" + inst.id);
            boolean crashed = text.contains("A fatal error has been detected") || text.contains("compilation errors");
            if (ours && crashed && text.contains("Initialising gl4es")) {
                prefs.edit().putBoolean("zink_" + inst.id, true).apply();
                return true;
            }
        } catch (IOException ignored) {
        }
        return false;
    }

    @Override
    java.io.File gameLog() {
        return Tools.DIR_GAME_HOME == null ? null : new File(Tools.DIR_GAME_HOME, "latestlog.txt");
    }

    /* ------------------------------------------------------------ Java */

    @Override
    boolean hasJava(int major) {
        for (Runtime r : MultiRTUtils.getRuntimes()) {
            if (r.javaVersion >= major) {
                return true;
            }
        }
        return false;
    }

    @Override
    String installedJava() {
        StringBuilder out = new StringBuilder();
        for (Runtime r : MultiRTUtils.getRuntimes()) {
            if (r.javaVersion > 0) {
                out.append(out.length() == 0 ? "" : ", ").append("Java ").append(r.javaVersion);
            }
        }
        return out.toString();
    }

    @Override
    List<String> javaSources(Activity activity) {
        List<String> names = new ArrayList<String>();
        for (String[] app : JAVA_APPS) {
            if (apkOf(activity, app[1]) != null) {
                names.add(app[0]);
            }
        }
        return names;
    }

    private static File apkOf(Activity activity, String packageName) {
        try {
            ApplicationInfo info = activity.getPackageManager().getApplicationInfo(packageName, 0);
            File apk = new File(info.sourceDir);
            return apk.canRead() ? apk : null;
        } catch (PackageManager.NameNotFoundException e) {
            return null;
        }
    }

    @Override
    String importJavaFromApp(Activity activity, String appName) throws Exception {
        for (String[] app : JAVA_APPS) {
            if (app[0].equals(appName)) {
                File apk = apkOf(activity, app[1]);
                if (apk == null) {
                    throw new IOException(appName + " не найден");
                }
                return importFromApk(apk);
            }
        }
        throw new IOException(appName + " не найден");
    }

    @Override
    String importJavaFromFile(Activity activity, Intent pickResult) throws Exception {
        Uri uri = pickResult.getData();
        if (uri == null) {
            throw new IOException("файл не выбран");
        }
        // the picked file is copied first: a zip (APK) needs random access
        File copy = new File(activity.getCacheDir(), "java-import.bin");
        InputStream in = activity.getContentResolver().openInputStream(uri);
        FileOutputStream out = new FileOutputStream(copy);
        try {
            byte[] buffer = new byte[65536];
            int n;
            while ((n = in.read(buffer)) > 0) {
                out.write(buffer, 0, n);
            }
        } finally {
            in.close();
            out.close();
        }
        try {
            if (isZip(copy)) {
                return importFromApk(copy);
            }
            // a single runtime archive (.tar.xz), like PojavLauncher's "import runtime"
            String name = "Vellora-" + System.currentTimeMillis() / 1000;
            FileInputStream archive = new FileInputStream(copy);
            MultiRTUtils.installRuntimeNamed(Tools.NATIVE_LIB_DIR, archive, name);
            MultiRTUtils.postPrepare(name);
            Runtime r = MultiRTUtils.forceReread(name);
            if (r.javaVersion <= 0) {
                MultiRTUtils.removeRuntimeNamed(name);
                throw new IOException("в архиве нет Java для Android");
            }
            useAsDefault(name);
            return "Java " + r.javaVersion;
        } finally {
            copy.delete();
        }
    }

    private static boolean isZip(File file) throws IOException {
        FileInputStream in = new FileInputStream(file);
        try {
            return in.read() == 'P' && in.read() == 'K';
        } finally {
            in.close();
        }
    }

    /** the Java packages inside a PojavLauncher-like APK: a common part and the part for this phone's processor */
    private static String importFromApk(File apk) throws Exception {
        String arch = Architecture.archAsString(Tools.DEVICE_ARCHITECTURE);
        List<String> installed = new ArrayList<String>();
        ZipFile zip = new ZipFile(apk);
        try {
            for (String[] pack : JAVA_PACKS) {
                ZipEntry universal = zip.getEntry(pack[0] + "/universal.tar.xz");
                ZipEntry bin = zip.getEntry(pack[0] + "/bin-" + arch + ".tar.xz");
                ZipEntry version = zip.getEntry(pack[0] + "/version");
                if (universal == null || bin == null) {
                    continue;
                }
                String v = version != null ? Tools.read(zip.getInputStream(version)) : "lullaby";
                MultiRTUtils.installRuntimeNamedBinpack(zip.getInputStream(universal), zip.getInputStream(bin), pack[1], v);
                MultiRTUtils.postPrepare(pack[1]);
                Runtime r = MultiRTUtils.forceReread(pack[1]);
                installed.add("Java " + (r.javaVersion > 0 ? r.javaVersion : "?"));
                useAsDefault(pack[1]);
            }
        } finally {
            zip.close();
        }
        if (installed.isEmpty()) {
            throw new IOException("в этом приложении нет Java для процессора телефона (" + arch + ")");
        }
        return Util.join(installed, ", ");
    }

    private static void useAsDefault(String name) {
        if (LauncherPreferences.PREF_DEFAULT_RUNTIME == null || LauncherPreferences.PREF_DEFAULT_RUNTIME.isEmpty()
            || MultiRTUtils.read(LauncherPreferences.PREF_DEFAULT_RUNTIME).javaVersion <= 0) {
            LauncherPreferences.PREF_DEFAULT_RUNTIME = name;
            LauncherPreferences.DEFAULT_PREF.edit().putString("defaultRuntime", name).apply();
        }
    }

    /* ------------------------------------------------------------ Java, downloaded by itself */

    /**
     * Java for Android, built from OpenJDK by the PojavLauncher team's android-openjdk-build-multiarch and kept in
     * Fold Craft Launcher's repository (GPL-3.0) as plain files: Java 8, 17, 21 and 25 for arm64, arm, x86 and x86_64,
     * in PojavLauncher's own format (a common part and a part for the processor). The commit is pinned, so the
     * files never change under us; jsDelivr serves the same files if GitHub is not reachable.
     */
    private static final String JAVA_COMMIT = "9ea040443effea5a78bd8aeed260e829ae349c2d";
    private static final String[] JAVA_MIRRORS = {
        "https://raw.githubusercontent.com/FCL-Team/FoldCraftLauncher/" + JAVA_COMMIT + "/FCL/src/main/jreAssets/app_runtime/java/",
        "https://cdn.jsdelivr.net/gh/FCL-Team/FoldCraftLauncher@" + JAVA_COMMIT + "/FCL/src/main/jreAssets/app_runtime/java/",
    };

    /** the name of an installed Java of exactly this major version, or null */
    private static String exactJava(int major) {
        String found = null;
        for (Runtime r : MultiRTUtils.getRuntimes()) {
            if (r.javaVersion == major && (found == null || r.name.startsWith("Vellora-"))) {
                found = r.name;
            }
        }
        return found;
    }

    /** makes sure Java of exactly this version is installed (downloading it the first time); returns its name */
    static String ensureJava(int major, Builder.Listener log) throws IOException {
        String installed = exactJava(major);
        if (installed != null) {
            return installed;
        }
        String arch = Architecture.archAsString(Tools.DEVICE_ARCHITECTURE);
        File dir = new File(Tools.DIR_DATA, "lullaby-java");
        dir.mkdirs();
        File universal = new File(dir, "jre" + major + "-universal.tar.xz");
        File bin = new File(dir, "jre" + major + "-bin-" + arch + ".tar.xz");
        IOException last = null;
        for (String mirror : JAVA_MIRRORS) {
            try {
                download(mirror + "jre" + major + "/universal.tar.xz", universal, "Скачиваю Java " + major + " (общая часть)", log);
                download(mirror + "jre" + major + "/bin-" + arch + ".tar.xz", bin, "Скачиваю Java " + major + " для процессора " + arch, log);
                last = null;
                break;
            } catch (IOException e) {
                last = e;
            }
        }
        if (last != null) {
            throw last;
        }
        log.log("Распаковываю Java " + major + "...");
        String name = "Vellora-Java-" + major;
        FileInputStream universalIn = new FileInputStream(universal);
        FileInputStream binIn = new FileInputStream(bin);
        try {
            MultiRTUtils.installRuntimeNamedBinpack(universalIn, binIn, name, "lullaby-" + JAVA_COMMIT.substring(0, 7));
        } finally {
            universalIn.close();
            binIn.close();
        }
        MultiRTUtils.postPrepare(name);
        Runtime r = MultiRTUtils.forceReread(name);
        universal.delete();
        bin.delete();
        if (r.javaVersion != major) {
            MultiRTUtils.removeRuntimeNamed(name);
            throw new IOException("скачанная Java не та (" + r.javaVersion + " вместо " + major + ")");
        }
        useAsDefault(name);
        log.log("Java " + major + " установлена.");
        return name;
    }

    /** a download with progress in the log, resumable only by starting over (the files are 5-35 MB) */
    private static void download(String url, File target, String what, Builder.Listener log) throws IOException {
        java.net.HttpURLConnection c = (java.net.HttpURLConnection) new java.net.URL(url).openConnection();
        c.setRequestProperty("User-Agent", Http.USER_AGENT);
        c.setConnectTimeout(20000);
        c.setReadTimeout(60000);
        try {
            if (c.getResponseCode() != 200) {
                throw new IOException("HTTP " + c.getResponseCode() + " (" + url + ")");
            }
            long total = c.getContentLength();
            File part = new File(target.getPath() + ".part");
            InputStream in = c.getInputStream();
            FileOutputStream out = new FileOutputStream(part);
            try {
                byte[] buffer = new byte[65536];
                long done = 0;
                int lastPercent = -1;
                int n;
                while ((n = in.read(buffer)) > 0) {
                    out.write(buffer, 0, n);
                    done += n;
                    int percent = total > 0 ? (int) (done * 100 / total) : -1;
                    if (percent / 10 != lastPercent / 10) {
                        lastPercent = percent;
                        log.log(what + (percent >= 0 ? ": " + percent + "%" : ": " + done / (1024 * 1024) + " МБ"));
                    }
                }
            } finally {
                in.close();
                out.close();
            }
            if (total > 0 && part.length() != total) {
                part.delete();
                throw new IOException("файл скачался не полностью");
            }
            if (!part.renameTo(target)) {
                throw new IOException("не удалось сохранить " + target.getName());
            }
        } finally {
            c.disconnect();
        }
    }

    /* ------------------------------------------------------------ playing */

    @Override
    void play(final MainActivity activity, final Installation inst, final Account account, final Builder.Listener log, final Done done) {
        new Thread(() -> {
            try {
                String problem = prepare(activity, inst, account, log, done);
                activity.runOnUiThread(() -> done.finished(problem == null, problem));
            } catch (Throwable e) {
                activity.runOnUiThread(() -> done.finished(false, "Не получилось подготовить игру: " + e.getMessage()));
            }
        }, "VelloraPlay").start();
    }

    /**
     * The extra Java arguments of every start. Sodium 0.5 and newer stop the game when LWJGL is not exactly the version
     * of that Minecraft ("Installed version: 3.3.3-snapshot, Required version: 3.3.1"); the engine brings its own
     * LWJGL 3.3.3 for every version, which works, so this check is switched off with Sodium's own setting.
     */
    static String javaArgs() {
        String own = "-Dsodium.checks.issue2561=false";
        String player = LauncherPreferences.PREF_CUSTOM_JAVA_ARGS;
        return player == null || player.trim().isEmpty() ? own : player.trim() + " " + own;
    }

    @Override
    java.io.File gameDir(Installation inst) {
        return new File(Tools.DIR_GAME_HOME, "custom_instances/lullaby-" + inst.id);
    }

    /** everything before the engine takes over; returns a message for the user when the game cannot start yet */
    private String prepare(final MainActivity activity, final Installation inst, Account account, final Builder.Listener log, final Done done) throws Exception {
        final boolean demo = account.guest;
        if (!demo && !account.ownsJava) {
            return "На этом аккаунте Microsoft нет Minecraft: Java Edition, поэтому игра не запустится. Её можно купить на minecraft.net.";
        }
        if (demo && !demoWorks(inst.minecraftVersion)) {
            return "В Minecraft " + inst.minecraftVersion + " нет демо-режима (он появился в 1.3.1), поэтому гостю эта версия не запускается. "
                + "Выбери сборку на версии новее или войди аккаунтом, на котором куплен Minecraft.";
        }
        if (demo) {
            log.log("Гость: демо-режим Minecraft, сборка с модами как есть.");
        }
        // the Java this Minecraft version needs: downloaded once, then used for every installation of such versions
        int java = javaFor(inst.minecraftVersion);
        String runtime;
        try {
            runtime = ensureJava(java, log);
        } catch (IOException e) {
            if (exactJava(java) == null) {
                log.log("Не удалось скачать Java " + java + ": " + e.getMessage());
                return NEED_JAVA + java;  // offline: the dialog to take it from PojavLauncher or a file
            }
            runtime = exactJava(java);
        }

        MinecraftAccount mc = new MinecraftAccount();
        if (demo) {
            // the engine's demo account ("Demo." before the name): the game gets "--demo" and the demo has its own copy
            // of Minecraft
            mc.username = "Demo." + account.name;
            mc.profileId = UUID.nameUUIDFromBytes(("OfflinePlayer:" + account.name).getBytes("UTF-8")).toString();
            mc.accessToken = "0";
            mc.isMicrosoft = false;
            mc.msaRefreshToken = "0";
            mc.xuid = "";
        } else {
            signIn(activity, account, log, mc);
        }
        mc.save();
        PojavProfile.setCurrentProfile(activity, mc.username);
        Tools.switchDemo(demo);  // the folders of Minecraft itself: the demo's or the full game's
        return startGame(activity, inst, java, runtime, log, done);
    }

    /** the Microsoft account for the engine, with a fresh Minecraft token */
    private static void signIn(MainActivity activity, Account account, Builder.Listener log, MinecraftAccount mc) throws Exception {
        if (account.minecraftTokenExpires - System.currentTimeMillis() < 30 * 60 * 1000L) {
            log.log("Обновляю вход в аккаунт...");
            try {
                account.refresh();
                account.save(activity);
            } catch (Account.SessionExpired e) {
                throw e;
            } catch (Exception e) {
                // no internet or a busy server: the game starts with the saved sign-in (the skin comes by the player's id)
                if (account.minecraftToken.isEmpty()) {
                    throw e;
                }
                log.log("Вход не обновился (" + e.getMessage() + "), запускаю с сохранённым.");
            }
        }
        mc.username = account.name;
        mc.profileId = account.uuid;
        mc.accessToken = account.minecraftToken;
        mc.isMicrosoft = true;
        mc.msaRefreshToken = account.refreshToken;
        mc.expiresAt = account.minecraftTokenExpires;
        mc.xuid = "";
    }

    /** the files, the loader, the profile, then the engine downloads Minecraft and starts it */
    private String startGame(final MainActivity activity, final Installation inst, int java, String runtime, final Builder.Listener log, final Done done)
        throws Exception {
        // the files: mods, resource packs, shaders and the modpack's own files
        File gameDir = new File(Tools.DIR_GAME_HOME, "custom_instances/lullaby-" + inst.id);
        installFiles(activity, inst, gameDir, log);
        // "Буст FPS": lighter graphics, or exactly the player's own settings back
        if (FpsBoost.apply(gameDir, inst.fpsBoost) && inst.fpsBoost) {
            log.log("Буст FPS: графика полегче.");
        }
        // mods that close the game on purpose on a phone (Sodium): their check is switched off
        for (String fixed : ModFixes.fixMods(new File(gameDir, "mods"))) {
            log.log("Исправил для телефона: " + fixed);
        }
        // the player's look in the game: the skin, the cape and the accessories from Vellora (Fabric and Quilt, 1.20.1)
        GameLook.prepare(activity, inst, gameDir);

        // the loader
        String versionId = versionId(inst);
        File versionJson = new File(Tools.DIR_HOME_VERSION, versionId + "/" + versionId + ".json");
        if (!versionJson.isFile() && !"vanilla".equals(inst.loader) && !inst.loader.isEmpty()) {
            String waiting = installLoader(activity, inst, log);
            if (waiting != null) {
                return waiting;
            }
        }

        // the controls Lullaby's "Управление" made
        File controls = new File(Tools.CTRLMAP_PATH, CONTROLS_FILE);
        controls.getParentFile().mkdirs();
        FileOutputStream out = new FileOutputStream(controls);
        try {
            out.write(Controls.load(activity).toPojavJson().getBytes("UTF-8"));
        } finally {
            out.close();
        }

        // the profile the engine starts
        LauncherProfiles.load();
        // the engine renames every profile whose key is not a UUID the next time it reads the file (and the game
        // then cannot find "the current profile"), so the key is the installation's id, which is a UUID
        String key = profileKey(inst);
        MinecraftProfile profile = LauncherProfiles.mainProfileJson.profiles.get(key);
        if (profile == null) {
            // an older Lullaby saved it under "lullaby-<id>", and the engine renamed that one to a random UUID
            String ourDir = Tools.LAUNCHERPROFILES_RTPREFIX + "custom_instances/lullaby-" + inst.id;
            for (java.util.Iterator<java.util.Map.Entry<String, MinecraftProfile>> it = LauncherProfiles.mainProfileJson.profiles.entrySet().iterator(); it.hasNext();) {
                if (ourDir.equals(it.next().getValue().gameDir)) {
                    it.remove();
                }
            }
        }
        if (profile == null) {
            profile = new MinecraftProfile();
        }
        profile.name = inst.name;
        profile.lastVersionId = versionId;
        profile.gameDir = Tools.LAUNCHERPROFILES_RTPREFIX + "custom_instances/lullaby-" + inst.id;
        profile.controlFile = CONTROLS_FILE;
        profile.javaDir = Tools.LAUNCHERPROFILES_RTPREFIX + runtime;  // exactly the Java this version needs
        profile.pojavRendererName = rendererFor(activity, inst);
        profile.javaArgs = javaArgs();
        // the Java heap for this start (the engine's default is 2 GB whatever the pack; big packs need more)
        int memory = memoryFor(activity, inst);
        LauncherPreferences.PREF_RAM_ALLOCATION = memory;
        LauncherPreferences.DEFAULT_PREF.edit().putInt("allocation", memory).commit();
        log.log("Память для игры: " + (memory >= 1024 ? String.format(Locale.ROOT, "%.1f ГБ", memory / 1024f) : memory + " МБ") + ".");
        if (LTW.equals(profile.pojavRendererName)) {
            // with LTW on Adreno the engine stops before the start to ask about the render distance (more than 7
            // makes too many buffers there); Lullaby sets it beforehand, so nothing waits for an answer
            limitRenderDistance(new File(Tools.DIR_GAME_HOME, "custom_instances/lullaby-" + inst.id), 7);
        }
        if (ZINK.equals(profile.pojavRendererName)) {
            // the newest Adreno GPUs (8xx) are newer than the Turnip Vulkan driver inside the engine: use the phone's own
            net.kdt.pojavlaunch.utils.GLInfoUtils.GLInfo gl = net.kdt.pojavlaunch.utils.GLInfoUtils.getGlInfo();
            boolean newAdreno = gl.isAdreno() && gl.renderer.matches(".*Adreno \\(TM\\) [89]\\d\\d.*");
            LauncherPreferences.PREF_ZINK_PREFER_SYSTEM_DRIVER = newAdreno;
            LauncherPreferences.DEFAULT_PREF.edit().putBoolean("zinkPreferSystemDriver", newAdreno).commit();
        }
        log.log("Графика: " + (LTW.equals(profile.pojavRendererName) ? "LTW" : ZINK.equals(profile.pojavRendererName) ? "Zink" : "Holy GL4ES")
                + ", Java " + java + ".");
        LauncherProfiles.mainProfileJson.profiles.put(key, profile);
        LauncherProfiles.write();
        LauncherPreferences.DEFAULT_PREF.edit().putString(LauncherPreferences.PREF_KEY_CURRENT_PROFILE, key).commit();
        LauncherProfiles.load();  // read it back the way the engine and the game will
        if (!LauncherProfiles.mainProfileJson.profiles.containsKey(key)) {
            return "Не удалось сохранить профиль игры (ключ " + key + "). Попробуй ещё раз.";
        }

        // the engine downloads Minecraft itself and starts it
        log.log("Скачиваю Minecraft " + inst.minecraftVersion + " (в первый раз это долго)...");
        waitForVersionList();
        final String normalized = AsyncMinecraftDownloader.normalizeVersionId(versionId);
        final JMinecraftVersionList.Version listed = AsyncMinecraftDownloader.getListedVersion(normalized);
        final long[] lastProgress = { System.currentTimeMillis() };
        ProgressListener progress = new ProgressListener() {
            public void onProgressStarted() {
                lastProgress[0] = System.currentTimeMillis();
            }

            public void onProgressUpdated(int value, int resid, Object... va) {
                lastProgress[0] = System.currentTimeMillis();
                if (resid != 0) {
                    try {
                        log.log(activity.getString(resid, va) + (value > 0 ? " (" + value + "%)" : ""));
                    } catch (Exception ignored) {
                        log.log("Скачивание: " + value + "%");
                    }
                }
            }

            public void onProgressEnded() {}
        };
        ProgressKeeper.addListener(com.kdt.mcgui.ProgressLayout.DOWNLOAD_MINECRAFT, progress);
        // the engine's own error dialogs need an AppCompat window; Lullaby shows the outcome itself
        final ContextAwareDoneListener startGame = new ContextAwareDoneListener(activity, normalized);
        final boolean[] finished = { false };
        final AsyncMinecraftDownloader.DoneListener outcome = new AsyncMinecraftDownloader.DoneListener() {
            public void onDownloadDone() {
                finished[0] = true;
                log.log("Minecraft скачан. Открываю игру...");
                startGame.onDownloadDone();
            }

            public void onDownloadFailed(Throwable error) {
                finished[0] = true;
                android.util.Log.e("Vellora", "Minecraft download failed", error);
                final String why = describe(error);
                activity.runOnUiThread(() -> done.finished(false, "Не удалось скачать Minecraft " + inst.minecraftVersion + ":\n" + why
                                                                       + "\n\nПроверь интернет и нажми «Играть» ещё раз — уже скачанное не скачивается заново."));
            }
        };
        final CountDownLatch started = new CountDownLatch(1);
        activity.runOnUiThread(() -> {
            new MinecraftDownloader().start(activity, listed, normalized, outcome);
            started.countDown();
        });
        started.await();
        // a download that stands still for long: say so instead of spinning silently
        new Thread(() -> {
            boolean warned = false;
            while (!finished[0]) {
                try {
                    Thread.sleep(5000);
                } catch (InterruptedException e) {
                    return;
                }
                long quiet = System.currentTimeMillis() - lastProgress[0];
                if (!warned && quiet > 90_000) {
                    warned = true;
                    log.log("Скачивание стоит уже " + quiet / 1000 + " с. Если так и останется — проверь интернет и запусти ещё раз.");
                }
            }
        }, "VelloraDownloadWatch").start();
        return null;
    }

    private static void waitForVersionList() throws InterruptedException {
        for (int i = 0; i < 300 && ExtraCore.getValue(ExtraConstants.RELEASE_TABLE) == null; i++) {
            Thread.sleep(100);
        }
    }

    /** options.txt of the game folder with renderDistance at most this (created if there is none yet) */
    static void limitRenderDistance(File gameDir, int max) throws IOException {
        File options = new File(gameDir, "options.txt");
        List<String> lines = new ArrayList<String>();
        boolean found = false;
        if (options.isFile()) {
            for (String line : Tools.read(options.getAbsolutePath()).split("\n")) {
                if (line.startsWith("renderDistance:")) {
                    found = true;
                    int value;
                    try {
                        value = Integer.parseInt(line.substring("renderDistance:".length()).trim());
                    } catch (NumberFormatException e) {
                        value = 12;
                    }
                    line = "renderDistance:" + Math.min(value, max);
                }
                lines.add(line);
            }
        }
        if (!found) {
            lines.add("renderDistance:" + max);
        }
        gameDir.mkdirs();
        StringBuilder out = new StringBuilder();
        for (String line : lines) {
            if (!line.isEmpty()) {
                out.append(line).append('\n');
            }
        }
        Tools.write(options.getAbsolutePath(), out.toString());
    }

    /** the engine's profile key for an installation: a UUID, as the engine requires */
    static String profileKey(Installation inst) {
        try {
            return java.util.UUID.fromString(inst.id).toString();
        } catch (IllegalArgumentException e) {
            return java.util.UUID.nameUUIDFromBytes(("lullaby:" + inst.id).getBytes()).toString();
        }
    }

    /** an error and its causes, short enough for a dialog */
    static String describe(Throwable error) {
        StringBuilder out = new StringBuilder();
        Throwable t = error;
        for (int depth = 0; t != null && depth < 4; depth++, t = t.getCause()) {
            String message = t.getMessage();
            out.append(depth == 0 ? "" : "\n← ").append(t.getClass().getSimpleName()).append(message == null ? "" : ": " + message);
        }
        return out.toString();
    }

    /** the name of the version in versions/ the engine starts */
    static String versionId(Installation inst) {
        String mc = inst.minecraftVersion;
        String lv = inst.loaderVersion;
        if ("fabric".equals(inst.loader)) {
            return "fabric-loader-" + lv + "-" + mc;
        }
        if ("quilt".equals(inst.loader)) {
            return "quilt-loader-" + lv + "-" + mc;
        }
        if ("forge".equals(inst.loader)) {
            return mc + "-forge-" + lv;
        }
        if ("neoforge".equals(inst.loader)) {
            return "neoforge-" + lv;
        }
        return mc;
    }

    /** Fabric and Quilt install right away; Forge and NeoForge run their installer once (returns what to tell the user) */
    private String installLoader(Activity activity, Installation inst, Builder.Listener log) throws Exception {
        if ("fabric".equals(inst.loader) || "quilt".equals(inst.loader)) {
            log.log("Ставлю " + Plan.loaderName(inst.loader) + " " + inst.loaderVersion + "...");
            final AtomicReference<String> error = new AtomicReference<String>();
            new FabriclikeDownloadTask(new ModloaderDownloadListener() {
                public void onDownloadFinished(File downloadedFile) {}

                public void onDataNotAvailable() {
                    error.set(Plan.loaderName(inst.loader) + " " + inst.loaderVersion + " не найден для Minecraft " + inst.minecraftVersion);
                }

                public void onDownloadError(Exception e) {
                    error.set(e.getMessage());
                }
            }, "fabric".equals(inst.loader) ? FabriclikeUtils.FABRIC_UTILS : FabriclikeUtils.QUILT_UTILS, inst.minecraftVersion,
                inst.loaderVersion, false).run();
            return error.get() == null ? null : "Не удалось поставить " + Plan.loaderName(inst.loader) + ": " + error.get();
        }
        // Forge and NeoForge: their installer, without its window (--installClient)
        String url = "neoforge".equals(inst.loader)
                         ? "https://maven.neoforged.net/releases/net/neoforged/neoforge/" + inst.loaderVersion + "/neoforge-" + inst.loaderVersion
                               + "-installer.jar"
                         : "https://maven.minecraftforge.net/net/minecraftforge/forge/" + inst.minecraftVersion + "-" + inst.loaderVersion + "/forge-"
                               + inst.minecraftVersion + "-" + inst.loaderVersion + "-installer.jar";
        File dir = new File(Tools.DIR_DATA, "lullaby-installers");
        dir.mkdirs();
        File installer = new File(dir, url.substring(url.lastIndexOf('/') + 1));
        if (!installer.isFile() || installer.length() == 0) {
            log.log("Скачиваю установщик " + Plan.loaderName(inst.loader) + " " + inst.loaderVersion + "...");
            Http.download(url, installer);
        }
        final Intent intent = new Intent(activity, JavaGUILauncherActivity.class);
        intent.putExtra("javaArgs", "-jar " + installer.getAbsolutePath() + " --installClient " + Tools.DIR_GAME_NEW);
        activity.runOnUiThread(() -> activity.startActivity(intent));
        return "Ставлю " + Plan.loaderName(inst.loader) + " " + inst.loaderVersion + ". Когда установка закончится, вернись в Vellora и нажми «Играть» ещё раз.";
    }

    /* ------------------------------------------------------------ files of the installation */

    /** downloads what the installation has switched on into its game folder and removes what was switched off */
    private static void installFiles(Activity activity, Installation inst, File gameDir, Builder.Listener log) throws Exception {
        gameDir.mkdirs();
        File listFile = new File(gameDir, ".lullaby-files");
        Set<String> before = new HashSet<String>();
        if (listFile.isFile()) {
            for (String line : Tools.read(listFile.getAbsolutePath()).split("\n")) {
                if (!line.trim().isEmpty()) {
                    before.add(line.trim());
                }
            }
        }
        Installation.Store store = ((MainActivity) activity).store();

        // a modpack's own files (configs, worlds...) once, on the first start
        File marker = new File(gameDir, ".lullaby-overrides");
        File source = store.sourcePack(inst.id);
        if (inst.hasSourcePack && source.isFile() && !marker.isFile()) {
            log.log("Распаковываю файлы сборки...");
            extractOverrides(source, gameDir);
            new FileOutputStream(marker).close();
        }

        List<Mod> included = Builder.included(inst.mods);
        Set<String> now = new HashSet<String>();
        int i = 0;
        for (Mod mod : included) {
            i++;
            String path = mod.targetPath();
            now.add(path);
            File target = new File(gameDir, path);
            if (target.isFile() && (mod.sha1.isEmpty() || ModFixes.fixedUnchanged(target) || mod.sha1.equalsIgnoreCase(Installer.sha1(target)))) {
                continue;
            }
            log.log("Скачиваю " + i + "/" + included.size() + ": " + mod.title);
            target.getParentFile().mkdirs();
            File part = new File(target.getPath() + ".part");
            if (CurseForge.isCurseForge(mod.projectId)) {
                copy(Installer.cachedCurseForgeFile(store, mod), part);
            } else {
                Http.download(mod.url, part);
                if (!mod.sha1.isEmpty() && !mod.sha1.equalsIgnoreCase(Installer.sha1(part))) {
                    part.delete();
                    throw new IOException("файл «" + mod.title + "» скачался с ошибкой");
                }
            }
            if (!part.renameTo(target)) {
                throw new IOException("не удалось сохранить «" + mod.title + "»");
            }
        }
        // switched off or removed in the editor: out of the game (files the player added by hand stay)
        for (String old : before) {
            if (!now.contains(old)) {
                new File(gameDir, old).delete();
            }
        }
        StringBuilder list = new StringBuilder();
        for (String path : now) {
            list.append(path).append('\n');
        }
        Tools.write(listFile.getAbsolutePath(), list.toString());
    }

    private static void extractOverrides(File pack, File gameDir) throws IOException {
        ZipFile zip = new ZipFile(pack);
        try {
            java.util.Enumeration<? extends ZipEntry> entries = zip.entries();
            String root = gameDir.getCanonicalPath() + File.separator;
            while (entries.hasMoreElements()) {
                ZipEntry entry = entries.nextElement();
                String name = entry.getName();
                String rel = name.startsWith("overrides/") ? name.substring(10) : name.startsWith("client-overrides/") ? name.substring(17) : null;
                if (rel == null || rel.isEmpty() || entry.isDirectory()) {
                    continue;
                }
                File target = new File(gameDir, rel);
                if (!target.getCanonicalPath().startsWith(root)) {
                    continue;  // a path that leaves the game folder
                }
                target.getParentFile().mkdirs();
                InputStream in = zip.getInputStream(entry);
                FileOutputStream out = new FileOutputStream(target);
                try {
                    byte[] buffer = new byte[65536];
                    int n;
                    while ((n = in.read(buffer)) > 0) {
                        out.write(buffer, 0, n);
                    }
                } finally {
                    in.close();
                    out.close();
                }
            }
        } finally {
            zip.close();
        }
    }

    private static void copy(File from, File to) throws IOException {
        FileInputStream in = new FileInputStream(from);
        FileOutputStream out = new FileOutputStream(to);
        try {
            byte[] buffer = new byte[65536];
            int n;
            while ((n = in.read(buffer)) > 0) {
                out.write(buffer, 0, n);
            }
        } finally {
            in.close();
            out.close();
        }
    }
}
