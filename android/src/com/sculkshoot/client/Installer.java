package com.sculkshoot.client;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.Locale;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipOutputStream;

/**
 * Installs things into an installation by hand, like the mod browser of the desktop launcher: a mod, resource pack or
 * shader with every library it needs, a whole Modrinth modpack, or a new empty installation. All methods block and run
 * on a background thread.
 */
final class Installer {
    private Installer() {}

    static final class InstallException extends Exception {
        InstallException(String message) {
            super(message);
        }
    }

    private static final int MAX_LIBRARIES = 40;

    /** which loaders' files fit a kind of project in this installation */
    static String loadersFor(Installation inst, String kind) {
        if (Mod.RESOURCE_PACK.equals(kind)) {
            return "[\"minecraft\"]";
        }
        if (Mod.SHADER.equals(kind)) {
            return "[\"iris\",\"optifine\"]";
        }
        return Modrinth.loadersJson(inst.loader);
    }

    /** the Modrinth search facets for a kind of project that fits the installation */
    static String facetsFor(Installation inst, String kind) {
        String version = "[\"versions:" + inst.minecraftVersion + "\"]";
        if (Mod.RESOURCE_PACK.equals(kind)) {
            return "[[\"project_type:resourcepack\"]," + version + "]";
        }
        if (Mod.SHADER.equals(kind)) {
            return "[[\"project_type:shader\"]," + version + "]";
        }
        String loaders = "quilt".equals(inst.loader) ? "[\"categories:quilt\",\"categories:fabric\"]" : "[\"categories:" + inst.loader + "\"]";
        return "[[\"project_type:mod\"]," + loaders + "," + version + "]";
    }

    /** the mod that loads shaders: Iris, or its port Oculus on Forge */
    static String shaderLoaderSlug(String loader) {
        return "forge".equals(loader) ? "oculus" : "iris";
    }

    /**
     * Adds a project with every required library; returns what was added (titles), in order.
     * Throws when the project has no file for this Minecraft version and loader or conflicts with an installed mod.
     */
    static List<String> install(Installation inst, String projectIdOrSlug, String kind) throws Exception {
        if (CurseForge.isCurseForge(projectIdOrSlug)) {
            return installCurseForge(inst, projectIdOrSlug, kind);
        }
        List<String> added = new ArrayList<String>();
        JSONObject project = Modrinth.project(projectIdOrSlug);
        if (project == null) {
            throw new InstallException("проект не найден на Modrinth");
        }
        String projectId = project.optString("id");
        if (inst.find(projectId) != null) {
            throw new InstallException("«" + project.optString("title") + "» уже установлен");
        }
        Modrinth.FileChoice file = Modrinth.bestFile(projectId, loadersFor(inst, kind), inst.minecraftVersion);
        if (!file.valid) {
            throw new InstallException("у «" + project.optString("title") + "» нет версии для " + inst.describe());
        }
        for (Mod other : inst.mods) {
            if (file.incompatible.contains("mr:" + other.projectId) || other.incompatible.contains("mr:" + projectId)) {
                throw new InstallException("«" + project.optString("title") + "» не работает вместе с «" + other.title + "»");
            }
        }
        final int before = inst.mods.size();
        inst.mods.add(toMod(project, file, kind, false));
        added.add(project.optString("title"));

        // every library the new file needs, and the libraries of those libraries
        List<String> queue = new ArrayList<String>(file.required);
        int rounds = 0;
        while (!queue.isEmpty() && rounds++ < MAX_LIBRARIES) {
            String required = queue.remove(0);
            String depId;
            if (required.startsWith("mrv:")) {
                JSONObject version = Modrinth.getObject(Builder.MODRINTH + "/version/" + required.substring(4));
                if (version == null) {
                    continue;
                }
                depId = version.optString("project_id");
            } else {
                depId = required.substring(3);
            }
            if (depId.isEmpty() || inst.find(depId) != null) {
                continue;
            }
            JSONObject depProject = Modrinth.project(depId);
            Modrinth.FileChoice depFile = Modrinth.bestFile(depId, Modrinth.loadersJson(inst.loader), inst.minecraftVersion);
            if (depProject == null || !depFile.valid) {
                // all or nothing: without the library the mod would crash the game
                while (inst.mods.size() > before) {
                    inst.mods.remove(inst.mods.size() - 1);
                }
                throw new InstallException("нужной библиотеки " + (depProject != null ? "«" + depProject.optString("title") + "» " : "")
                                           + "нет для " + inst.describe());
            }
            Mod library = toMod(depProject, depFile, Mod.MOD, true);
            library.neededBy.add(project.optString("title"));
            inst.mods.add(library);
            added.add(depProject.optString("title"));
            queue.addAll(depFile.required);
        }

        // shaders need a mod that loads them
        if (Mod.SHADER.equals(kind) && findBySlug(inst, shaderLoaderSlug(inst.loader)) == null
            && findBySlug(inst, "iris") == null && findBySlug(inst, "oculus") == null) {
            try {
                added.addAll(install(inst, shaderLoaderSlug(inst.loader), Mod.MOD));
            } catch (InstallException e) {
                added.add("(без " + ("forge".equals(inst.loader) ? "Oculus" : "Iris") + ": " + e.getMessage() + ")");
            }
        }
        return added;
    }

    /** like install(), for a CurseForge project ("cf:<id>"); its libraries come from CurseForge too, or from Modrinth if blocked */
    static List<String> installCurseForge(Installation inst, String projectId, String kind) throws Exception {
        List<String> added = new ArrayList<String>();
        JSONObject project = CurseForge.project(projectId);
        if (project == null) {
            throw new InstallException("проект не найден на CurseForge");
        }
        String title = project.optString("name");
        if (inst.find(projectId) != null || findBySlug(inst, project.optString("slug")) != null) {
            throw new InstallException("«" + title + "» уже установлен");
        }
        CurseForge.FileChoice choice = CurseForge.bestFile(projectId, inst, kind);
        if (choice.file == null) {
            throw new InstallException("у «" + title + "» на CurseForge нет версии для " + inst.describe());
        }
        if (choice.blocked) {
            throw new InstallException("автор «" + title + "» запретил скачивать его мимо приложения CurseForge. Поищи его на Modrinth.");
        }
        Mod mod = CurseForge.toMod(project, choice.file, kind, false);
        for (Mod other : inst.mods) {
            if (mod.incompatible.contains("mr:" + other.projectId) || other.incompatible.contains("mr:" + mod.projectId)) {
                throw new InstallException("«" + title + "» не работает вместе с «" + other.title + "»");
            }
        }
        final int before = inst.mods.size();
        inst.mods.add(mod);
        added.add(title);

        List<Mod> queue = new ArrayList<Mod>();
        queue.add(mod);
        int rounds = 0;
        while (!queue.isEmpty() && rounds++ < MAX_LIBRARIES) {
            Mod needs = queue.remove(0);
            for (String required : new ArrayList<String>(needs.required)) {
                String depId = required.substring(3);
                if (inst.find(depId) != null) {
                    continue;
                }
                JSONObject depProject = CurseForge.project(depId);
                if (depProject != null && findBySlug(inst, depProject.optString("slug")) != null) {
                    // the same library is already there from Modrinth
                    needs.required.remove(required);
                    needs.required.add("mr:" + findBySlug(inst, depProject.optString("slug")).projectId);
                    continue;
                }
                CurseForge.FileChoice depFile = depProject == null ? null : CurseForge.bestFile(depId, inst, Mod.MOD);
                Mod library = null;
                if (depFile != null && depFile.file != null && !depFile.blocked) {
                    library = CurseForge.toMod(depProject, depFile.file, Mod.MOD, true);
                } else if (depProject != null) {
                    // blocked on CurseForge: most libraries are on Modrinth under the same name
                    JSONObject mr = Modrinth.project(depProject.optString("slug"));
                    Modrinth.FileChoice mrFile = mr == null ? null : Modrinth.bestFile(mr.optString("id"), Modrinth.loadersJson(inst.loader), inst.minecraftVersion);
                    if (mrFile != null && mrFile.valid) {
                        library = toMod(mr, mrFile, Mod.MOD, true);
                        needs.required.remove(required);
                        needs.required.add("mr:" + library.projectId);
                    }
                }
                if (library == null) {
                    while (inst.mods.size() > before) {
                        inst.mods.remove(inst.mods.size() - 1);
                    }
                    throw new InstallException("нужную библиотеку " + (depProject != null ? "«" + depProject.optString("name") + "» " : "")
                                               + "не скачать для " + inst.describe());
                }
                library.neededBy.add(title);
                inst.mods.add(library);
                added.add(library.title);
                if (CurseForge.isCurseForge(library.projectId)) {
                    queue.add(library);
                }
            }
        }

        if (Mod.SHADER.equals(kind) && findBySlug(inst, shaderLoaderSlug(inst.loader)) == null
            && findBySlug(inst, "iris") == null && findBySlug(inst, "oculus") == null) {
            try {
                added.addAll(install(inst, shaderLoaderSlug(inst.loader), Mod.MOD));
            } catch (InstallException e) {
                added.add("(без " + ("forge".equals(inst.loader) ? "Oculus" : "Iris") + ": " + e.getMessage() + ")");
            }
        }
        return added;
    }

    static Mod findBySlug(Installation inst, String slug) {
        for (Mod mod : inst.mods) {
            if (slug.equals(mod.slug)) {
                return mod;
            }
        }
        return null;
    }

    private static Mod toMod(JSONObject project, Modrinth.FileChoice file, String kind, boolean dependency) {
        Mod mod = new Mod();
        mod.kind = kind;
        mod.projectId = project.optString("id");
        mod.slug = project.optString("slug");
        mod.title = project.optString("title");
        mod.description = project.optString("description");
        mod.iconUrl = project.isNull("icon_url") ? "" : project.optString("icon_url");
        mod.group = dependency ? Builder.GROUP_LIBRARIES : "";
        mod.dependency = dependency;
        mod.versionNumber = file.versionNumber;
        mod.fileName = file.fileName;
        mod.url = file.url;
        mod.sha1 = file.sha1;
        mod.sha512 = file.sha512;
        mod.size = file.size;
        mod.required = file.required;
        mod.incompatible = file.incompatible;
        return mod;
    }

    /**
     * Mods from modpacks or older versions may have no icon yet: looks them up on Modrinth (100 per request), so
     * every row shows the same picture as on modrinth.com. Returns true if anything changed.
     */
    static boolean fillIcons(Installation inst) throws IOException, JSONException {
        List<String> ids = new ArrayList<String>();
        for (Mod mod : inst.mods) {
            if (mod.iconUrl.isEmpty() && !mod.projectId.isEmpty() && !CurseForge.isCurseForge(mod.projectId) && !ids.contains(mod.projectId)) {
                ids.add(mod.projectId);
            }
        }
        boolean changed = false;
        for (int from = 0; from < ids.size(); from += 100) {
            JSONArray found = Modrinth.projects(ids.subList(from, Math.min(ids.size(), from + 100)));
            for (int i = 0; i < found.length(); i++) {
                JSONObject project = found.optJSONObject(i);
                if (project == null) {
                    continue;
                }
                String icon = project.isNull("icon_url") ? "" : project.optString("icon_url");
                for (Mod mod : inst.mods) {
                    if (mod.projectId.equals(project.optString("id"))) {
                        if (!icon.equals(mod.iconUrl)) {
                            mod.iconUrl = icon;
                            changed = true;
                        }
                        if (mod.title.isEmpty() || mod.fileName.startsWith(mod.title)) {  // named after its file until now
                            mod.title = project.optString("title", mod.title);
                            changed = true;
                        }
                        if (mod.description.isEmpty()) {
                            mod.description = project.optString("description");
                            changed = changed || !mod.description.isEmpty();
                        }
                    }
                }
            }
        }
        return changed;
    }

    /**
     * Like "Check for updates" of the desktop launcher: every mod, resource pack and shader that came from Modrinth or
     * CurseForge gets its newest file for this Minecraft version and loader; libraries a new version needs are added.
     * Returns the lines to show ("Sodium: 0.5.8 → 0.6.0").
     */
    static List<String> updateAll(Installation inst, Builder.Listener log) throws Exception {
        List<String> changes = new ArrayList<String>();
        List<Mod> mods = new ArrayList<Mod>(inst.mods);
        int i = 0;
        for (Mod mod : mods) {
            i++;
            if (mod.projectId.isEmpty() || Mod.FILE.equals(mod.kind) || (!mod.enabled && mod.url.isEmpty())) {
                continue;
            }
            log.log("Проверяю " + i + "/" + mods.size() + ": " + mod.title);
            String before = mod.versionNumber;
            try {
                if (CurseForge.isCurseForge(mod.projectId)) {
                    CurseForge.FileChoice choice = CurseForge.bestFile(mod.projectId, inst, mod.kind);
                    if (choice.file == null || choice.blocked || choice.file.optString("fileName").equals(mod.fileName)) {
                        continue;
                    }
                    Mod fresh = CurseForge.toMod(new JSONObject().put("id", Long.parseLong(mod.projectId.substring(3))), choice.file, mod.kind,
                                                 mod.dependency);
                    mod.versionNumber = fresh.versionNumber;
                    mod.fileName = fresh.fileName;
                    mod.url = fresh.url;
                    mod.sha1 = fresh.sha1;
                    mod.sha512 = "";
                    mod.size = fresh.size;
                    mod.required = fresh.required;
                    mod.incompatible = fresh.incompatible;
                } else {
                    Modrinth.FileChoice file = Modrinth.bestFile(mod.projectId, loadersFor(inst, mod.kind), inst.minecraftVersion);
                    if (!file.valid || file.sha1.equalsIgnoreCase(mod.sha1) || file.fileName.equals(mod.fileName)) {
                        continue;
                    }
                    mod.versionNumber = file.versionNumber;
                    mod.fileName = file.fileName;
                    mod.url = file.url;
                    mod.sha1 = file.sha1;
                    mod.sha512 = file.sha512;
                    mod.size = file.size;
                    mod.required = file.required;
                    mod.incompatible = file.incompatible;
                }
            } catch (IOException e) {
                continue;  // this one could not be checked now; the others still can
            }
            changes.add(mod.title + ": " + (before.isEmpty() ? "?" : before) + " → " + mod.versionNumber);
        }
        // libraries the new versions need
        for (Mod mod : new ArrayList<Mod>(inst.mods)) {
            for (String required : new ArrayList<String>(mod.required)) {
                String id = required.startsWith("mr:") ? required.substring(3) : "";
                if (id.isEmpty() || id.startsWith("mrv:") || inst.find(id) != null) {
                    continue;
                }
                try {
                    int sizeBefore = inst.mods.size();
                    List<String> added = install(inst, id, Mod.MOD);
                    for (int k = sizeBefore; k < inst.mods.size(); k++) {
                        Mod library = inst.mods.get(k);
                        library.dependency = true;
                        library.group = Builder.GROUP_LIBRARIES;
                        if (!library.neededBy.contains(mod.title)) {
                            library.neededBy.add(mod.title);
                        }
                    }
                    changes.add("+ " + Util.join(added, ", ") + " (нужно для " + mod.title + ")");
                } catch (InstallException e) {
                    changes.add("! " + mod.title + ": " + e.getMessage());
                }
            }
        }
        return changes;
    }

    /** removes a file and the libraries nobody needs any more */
    static void remove(Installation inst, Mod mod) {
        inst.mods.remove(mod);
        boolean changed = true;
        while (changed) {
            changed = false;
            for (int i = inst.mods.size() - 1; i >= 0; i--) {
                Mod library = inst.mods.get(i);
                if (!library.dependency) {
                    continue;
                }
                boolean needed = false;
                for (Mod other : inst.mods) {
                    needed = needed || (other != library && other.required.contains("mr:" + library.projectId));
                }
                if (!needed) {
                    inst.mods.remove(i);
                    changed = true;
                }
            }
        }
    }

    /** a new installation; with essentials it gets the performance and comfort mods Moony adds to every pack */
    static Installation create(String name, String minecraftVersion, String loader, boolean essentials, Builder.Listener log) throws Exception {
        Installation inst = new Installation();
        inst.name = name;
        inst.minecraftVersion = minecraftVersion;
        inst.loader = loader;
        if ("vanilla".equals(loader)) {
            // plain Minecraft, like Millida's "Vanilla": no loader and no mods
            inst.loaderVersion = "";
            return inst;
        }
        if (essentials) {
            Plan plan = new Plan();
            plan.name = name;
            plan.minecraftVersion = minecraftVersion;
            plan.loader = loader;
            plan.curated = Plan.essentialMods(loader, false, false);
            Builder builder = new Builder(plan, log);
            builder.run();
            inst.loaderVersion = builder.loaderVersion;
            for (Mod mod : builder.mods) {
                mod.group = "";
                inst.mods.add(mod);
            }
        } else {
            log.log("Ищу " + Plan.loaderName(loader) + " для Minecraft " + minecraftVersion + "...");
            inst.loaderVersion = Modrinth.loaderVersion(loader, minecraftVersion);
            if (inst.loaderVersion.isEmpty()) {
                throw new InstallException(Plan.loaderName(loader) + " нет для Minecraft " + minecraftVersion);
            }
        }
        return inst;
    }

    /* ------------------------------------------------------------ modpacks */

    /** Downloads a Modrinth modpack (its newest release) and makes an installation of it; the original file is kept. */
    static Installation importModpack(String projectId, Installation.Store store, Builder.Listener log) throws Exception {
        if (CurseForge.isCurseForge(projectId)) {
            return importCurseForgePack(projectId, store, log);
        }
        JSONObject project = Modrinth.project(projectId);
        if (project == null) {
            throw new InstallException("сборка не найдена на Modrinth");
        }
        log.log("Ищу последнюю версию «" + project.optString("title") + "»...");
        Http.Response r = Http.get(Builder.MODRINTH + "/project/" + project.optString("id") + "/version");
        if (!r.ok()) {
            throw new InstallException("не удалось получить версии сборки");
        }
        JSONArray versions = new JSONArray(r.body);
        JSONObject version = null;
        for (int i = 0; i < versions.length() && version == null; i++) {
            if ("release".equals(versions.getJSONObject(i).optString("version_type"))) {
                version = versions.getJSONObject(i);
            }
        }
        if (version == null && versions.length() > 0) {
            version = versions.getJSONObject(0);
        }
        JSONArray files = version == null ? null : version.optJSONArray("files");
        JSONObject file = null;
        for (int i = 0; files != null && i < files.length(); i++) {
            JSONObject f = files.getJSONObject(i);
            if (f.optString("filename").endsWith(".mrpack") && (file == null || f.optBoolean("primary"))) {
                file = f;
            }
        }
        if (file == null) {
            throw new InstallException("у сборки нет файла .mrpack");
        }
        log.log("Скачиваю " + file.optString("filename") + "...");
        Installation inst = new Installation();
        File target = store.sourcePack(inst.id);
        Http.download(file.optString("url"), target);
        try {
            readIndex(inst, target);
        } catch (Exception e) {
            target.delete();
            throw new InstallException("файл сборки повреждён: " + e.getMessage());
        }
        inst.hasSourcePack = true;
        inst.iconUrl = project.isNull("icon_url") ? "" : project.optString("icon_url");
        if (inst.name.isEmpty()) {
            inst.name = project.optString("title");
        }
        return inst;
    }

    /** a CurseForge file, downloaded once and checked against its SHA-1 */
    static File cachedCurseForgeFile(Installation.Store store, Mod mod) throws IOException {
        File dir = store.curseForgeCache();
        String name = (mod.sha1.isEmpty() ? String.valueOf(mod.url.hashCode()) : mod.sha1) + "-" + mod.fileName.replaceAll("[^A-Za-z0-9._+-]", "_");
        File file = new File(dir, name);
        if (file.isFile() && (mod.sha1.isEmpty() || mod.sha1.equalsIgnoreCase(sha1(file)))) {
            return file;
        }
        if (mod.url.isEmpty()) {
            throw new IOException("«" + mod.title + "» нельзя скачать с CurseForge");
        }
        File part = new File(dir, name + ".part");
        Http.download(mod.url, part);
        if (!mod.sha1.isEmpty() && !mod.sha1.equalsIgnoreCase(sha1(part))) {
            part.delete();
            throw new IOException("файл «" + mod.title + "» скачался с ошибкой, попробуй ещё раз");
        }
        if (!part.renameTo(file)) {
            throw new IOException("не удалось сохранить «" + mod.title + "»");
        }
        return file;
    }

    static String sha1(File file) throws IOException {
        try {
            java.security.MessageDigest digest = java.security.MessageDigest.getInstance("SHA-1");
            java.io.FileInputStream in = new java.io.FileInputStream(file);
            try {
                byte[] buffer = new byte[65536];
                int n;
                while ((n = in.read(buffer)) > 0) {
                    digest.update(buffer, 0, n);
                }
            } finally {
                in.close();
            }
            StringBuilder hex = new StringBuilder();
            for (byte b : digest.digest()) {
                hex.append(String.format("%02x", b & 0xFF));
            }
            return hex.toString();
        } catch (java.security.NoSuchAlgorithmException e) {
            throw new IOException(e.getMessage());
        }
    }

    /* ------------------------------------------------------------ CurseForge modpacks and pack files */

    /** downloads a CurseForge modpack (its newest release) and makes an installation of it */
    static Installation importCurseForgePack(String projectId, Installation.Store store, Builder.Listener log) throws Exception {
        JSONObject project = CurseForge.project(projectId);
        if (project == null) {
            throw new InstallException("сборка не найдена на CurseForge");
        }
        log.log("Ищу последнюю версию «" + project.optString("name") + "»...");
        JSONObject file = CurseForge.newestPackFile(projectId);
        if (file == null) {
            throw new InstallException("у сборки нет файлов");
        }
        if (file.isNull("downloadUrl") || file.optString("downloadUrl").isEmpty()) {
            throw new InstallException("автор сборки запретил скачивать её мимо приложения CurseForge");
        }
        log.log("Скачиваю " + file.optString("fileName") + "...");
        File zip = File.createTempFile("cf-pack", ".zip", store.curseForgeCache());
        try {
            Http.download(file.optString("downloadUrl"), zip);
            Installation inst = new Installation();
            readCurseForgePack(inst, zip, store, log);
            JSONObject logo = project.optJSONObject("logo");
            inst.iconUrl = logo != null ? logo.optString("thumbnailUrl", logo.optString("url")) : "";
            if (inst.name.isEmpty()) {
                inst.name = project.optString("name");
            }
            return inst;
        } finally {
            zip.delete();
        }
    }

    /** a pack file the user picked: a Modrinth .mrpack or a CurseForge .zip (manifest.json) */
    static Installation importFile(File file, Installation.Store store, Builder.Listener log) throws Exception {
        ZipFile zip;
        try {
            zip = new ZipFile(file);
        } catch (IOException e) {
            throw new InstallException("это не файл сборки (.mrpack или .zip)");
        }
        boolean modrinth;
        boolean curseForge;
        try {
            modrinth = zip.getEntry("modrinth.index.json") != null;
            curseForge = zip.getEntry("manifest.json") != null;
        } finally {
            zip.close();
        }
        Installation inst = new Installation();
        if (modrinth) {
            log.log("Читаю сборку Modrinth...");
            File target = store.sourcePack(inst.id);
            copyFile(file, target);
            try {
                readIndex(inst, target);
            } catch (Exception e) {
                target.delete();
                throw new InstallException("файл сборки повреждён: " + e.getMessage());
            }
            inst.hasSourcePack = true;
            return inst;
        }
        if (curseForge) {
            readCurseForgePack(inst, file, store, log);
            return inst;
        }
        throw new InstallException("в файле нет описания сборки (ни modrinth.index.json, ни manifest.json)");
    }

    private static void copyFile(File from, File to) throws IOException {
        java.io.FileInputStream in = new java.io.FileInputStream(from);
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

    /**
     * Reads a CurseForge pack (manifest.json): Minecraft, the loader and every file, asked from CurseForge in batches.
     * Files whose authors forbid downloads outside the CurseForge app are looked up on Modrinth by their SHA-1 (most
     * are there too); the rest are kept switched off with a note. The pack's own files (configs...) are kept like a
     * Modrinth pack's: in the installation's source pack, under overrides/.
     */
    static void readCurseForgePack(Installation inst, File pack, Installation.Store store, Builder.Listener log) throws Exception {
        ZipFile zip = new ZipFile(pack);
        try {
            ZipEntry manifestEntry = zip.getEntry("manifest.json");
            if (manifestEntry == null) {
                throw new InstallException("в сборке нет manifest.json");
            }
            JSONObject manifest = new JSONObject(Http.read(zip.getInputStream(manifestEntry)));
            inst.name = manifest.optString("name");
            JSONObject minecraft = manifest.optJSONObject("minecraft");
            if (minecraft == null) {
                throw new InstallException("в manifest.json нет версии Minecraft");
            }
            inst.minecraftVersion = minecraft.optString("version");
            JSONArray loaders = minecraft.optJSONArray("modLoaders");
            for (int i = 0; loaders != null && i < loaders.length(); i++) {
                JSONObject l = loaders.getJSONObject(i);
                String id = l.optString("id");  // "forge-47.2.0", "neoforge-21.1.80", "fabric-0.16.9", "quilt-0.27.1"
                int dash = id.indexOf('-');
                if (dash > 0 && (inst.loader.isEmpty() || l.optBoolean("primary"))) {
                    inst.loader = id.substring(0, dash).toLowerCase(Locale.ROOT);
                    inst.loaderVersion = id.substring(dash + 1);
                }
            }
            if (inst.loader.isEmpty()) {
                throw new InstallException("сборка без загрузчика модов");
            }
            if ("neoforge".equals(inst.loader) && inst.loaderVersion.startsWith("1.20.1-")) {
                inst.loaderVersion = inst.loaderVersion.substring("1.20.1-".length());  // NeoForge for 1.20.1 is a Forge fork
            }

            // the pack's own files
            String overrides = manifest.optString("overrides", "overrides");
            File source = store.sourcePack(inst.id);
            java.util.zip.ZipOutputStream out = new java.util.zip.ZipOutputStream(new FileOutputStream(source));
            int extra = 0;
            try {
                byte[] buffer = new byte[65536];
                Enumeration<? extends ZipEntry> entries = zip.entries();
                while (entries.hasMoreElements()) {
                    ZipEntry entry = entries.nextElement();
                    if (entry.isDirectory() || !entry.getName().startsWith(overrides + "/")) {
                        continue;
                    }
                    out.putNextEntry(new ZipEntry("overrides/" + entry.getName().substring(overrides.length() + 1)));
                    InputStream in = zip.getInputStream(entry);
                    int n;
                    while ((n = in.read(buffer)) > 0) {
                        out.write(buffer, 0, n);
                    }
                    in.close();
                    out.closeEntry();
                    extra++;
                }
            } finally {
                out.close();
            }
            inst.hasSourcePack = true;

            // the files, from CurseForge in batches of 100
            JSONArray list = manifest.optJSONArray("files");
            List<Long> fileIds = new ArrayList<Long>();
            List<Long> modIds = new ArrayList<Long>();
            for (int i = 0; list != null && i < list.length(); i++) {
                JSONObject f = list.getJSONObject(i);
                fileIds.add(f.optLong("fileID"));
                modIds.add(f.optLong("projectID"));
            }
            log.log("В сборке " + fileIds.size() + " файлов и " + extra + " своих файлов (настройки и т. п.). Спрашиваю CurseForge...");
            java.util.Map<Long, JSONObject> projects = new java.util.HashMap<Long, JSONObject>();
            java.util.Map<Long, JSONObject> files = new java.util.HashMap<Long, JSONObject>();
            for (int from = 0; from < fileIds.size(); from += 100) {
                List<Long> ids = modIds.subList(from, Math.min(modIds.size(), from + 100));
                JSONArray data = CurseForge.post("/mods", new JSONObject().put("modIds", new JSONArray(ids))).optJSONArray("data");
                for (int i = 0; data != null && i < data.length(); i++) {
                    projects.put(data.getJSONObject(i).optLong("id"), data.getJSONObject(i));
                }
                List<Long> fids = fileIds.subList(from, Math.min(fileIds.size(), from + 100));
                data = CurseForge.post("/mods/files", new JSONObject().put("fileIds", new JSONArray(fids))).optJSONArray("data");
                for (int i = 0; data != null && i < data.length(); i++) {
                    files.put(data.getJSONObject(i).optLong("id"), data.getJSONObject(i));
                }
                log.log("Получено " + Math.min(fileIds.size(), from + 100) + " из " + fileIds.size() + "...");
            }

            List<Mod> blocked = new ArrayList<Mod>();
            for (int i = 0; i < fileIds.size(); i++) {
                JSONObject file = files.get(fileIds.get(i));
                JSONObject project = projects.get(modIds.get(i));
                if (file == null) {
                    continue;
                }
                if (project == null) {
                    project = new JSONObject().put("id", modIds.get(i)).put("name", file.optString("displayName"));
                }
                int classId = project.optInt("classId", 6);
                String kind = classId == 12 ? Mod.RESOURCE_PACK : classId == 6552 ? Mod.SHADER : Mod.MOD;
                Mod mod = CurseForge.toMod(project, file, kind, false);
                mod.required.clear();  // the pack lists every file it needs itself
                if (classId != 6 && classId != 12 && classId != 6552) {
                    mod.kind = Mod.FILE;  // worlds, data packs...: kept as plain files of the pack
                    mod.path = (classId == 6945 ? "datapacks/" : "") + mod.fileName;
                }
                if (mod.url.isEmpty()) {
                    blocked.add(mod);
                }
                inst.mods.add(mod);
            }

            // files CurseForge does not give out: the same file (same SHA-1) is usually on Modrinth
            if (!blocked.isEmpty()) {
                log.log(blocked.size() + " файлов CurseForge не даёт скачать мимо своего приложения. Ищу их на Modrinth...");
                JSONArray hashes = new JSONArray();
                for (Mod mod : blocked) {
                    if (!mod.sha1.isEmpty()) {
                        hashes.put(mod.sha1);
                    }
                }
                JSONObject found = new JSONObject();
                if (hashes.length() > 0) {
                    Http.Response r = Http.post(Builder.MODRINTH + "/version_files", "application/json",
                                                new JSONObject().put("hashes", hashes).put("algorithm", "sha1").toString());
                    if (r.ok()) {
                        found = new JSONObject(r.body);
                    }
                }
                int fromModrinth = 0;
                for (Mod mod : blocked) {
                    JSONObject version = found.optJSONObject(mod.sha1.toLowerCase(Locale.ROOT));
                    JSONArray vfiles = version == null ? null : version.optJSONArray("files");
                    for (int i = 0; vfiles != null && i < vfiles.length(); i++) {
                        JSONObject vf = vfiles.getJSONObject(i);
                        JSONObject h = vf.optJSONObject("hashes");
                        if (h != null && mod.sha1.equalsIgnoreCase(h.optString("sha1"))) {
                            mod.projectId = version.optString("project_id");
                            mod.url = vf.optString("url");
                            mod.sha512 = h.optString("sha512");
                            mod.versionNumber = version.optString("version_number", mod.versionNumber);
                            mod.iconUrl = "";
                            fromModrinth++;
                        }
                    }
                    if (mod.url.isEmpty()) {
                        mod.enabled = false;
                        mod.description = "Автор разрешил скачивать этот файл только через приложение CurseForge — скачай его там "
                                          + "и положи в папку игры вручную.";
                    }
                }
                log.log("С Modrinth: " + fromModrinth + ", недоступно: " + (blocked.size() - fromModrinth) + ".");
            }
        } finally {
            zip.close();
        }
    }

    /** reads modrinth.index.json of a .mrpack into the installation */
    static void readIndex(Installation inst, File pack) throws IOException, JSONException {
        ZipFile zip = new ZipFile(pack);
        try {
            ZipEntry entry = zip.getEntry("modrinth.index.json");
            if (entry == null) {
                throw new IOException("нет modrinth.index.json");
            }
            JSONObject index = new JSONObject(Http.read(zip.getInputStream(entry)));
            inst.name = index.optString("name");
            JSONObject deps = index.optJSONObject("dependencies");
            if (deps != null) {
                inst.minecraftVersion = deps.optString("minecraft");
                for (String loader : new String[] { "neoforge", "forge", "fabric", "quilt" }) {
                    String version = deps.optString(Builder.indexKey(loader), "");
                    if (!version.isEmpty()) {
                        inst.loader = loader;
                        inst.loaderVersion = version;
                    }
                }
            }
            if (inst.loader.isEmpty()) {
                throw new IOException("сборка без загрузчика модов");
            }
            JSONArray list = index.optJSONArray("files");
            for (int i = 0; list != null && i < list.length(); i++) {
                JSONObject f = list.getJSONObject(i);
                JSONObject env = f.optJSONObject("env");
                if (env != null && "unsupported".equals(env.optString("client"))) {
                    continue;  // server only
                }
                Mod mod = new Mod();
                mod.path = f.optString("path");
                String lower = mod.path.toLowerCase(Locale.ROOT);
                mod.kind = lower.startsWith("mods/") ? Mod.MOD
                         : lower.startsWith("resourcepacks/") ? Mod.RESOURCE_PACK
                         : lower.startsWith("shaderpacks/") ? Mod.SHADER : Mod.FILE;
                mod.fileName = mod.path.substring(mod.path.lastIndexOf('/') + 1);
                mod.title = mod.fileName.replaceAll("\\.(jar|zip)$", "");
                JSONArray downloads = f.optJSONArray("downloads");
                mod.url = downloads != null && downloads.length() > 0 ? downloads.optString(0) : "";
                JSONObject hashes = f.optJSONObject("hashes");
                mod.sha1 = hashes != null ? hashes.optString("sha1") : "";
                mod.sha512 = hashes != null ? hashes.optString("sha512") : "";
                mod.size = f.optLong("fileSize");
                // Modrinth's CDN links carry the project id: cdn.modrinth.com/data/<project>/versions/...
                int at = mod.url.indexOf("/data/");
                if (at >= 0) {
                    String rest = mod.url.substring(at + 6);
                    mod.projectId = rest.contains("/") ? rest.substring(0, rest.indexOf('/')) : "";
                }
                inst.mods.add(mod);
            }
        } finally {
            zip.close();
        }
    }

    /** writes the installation as a .mrpack: the files of the original modpack (configs...) plus the new list of files */
    static void writeMrpack(Installation inst, Installation.Store store, File out) throws IOException, JSONException {
        Plan plan = new Plan();
        plan.name = inst.name;
        plan.summary = "Vellora Client: " + inst.describe();
        plan.minecraftVersion = inst.minecraftVersion;
        plan.loader = inst.loader;
        ZipOutputStream zip = new ZipOutputStream(new FileOutputStream(out));
        try {
            zip.putNextEntry(new ZipEntry("modrinth.index.json"));
            zip.write(Builder.makeIndex(plan, inst.loaderVersion, inst.mods).getBytes("UTF-8"));
            zip.closeEntry();
            // CurseForge files cannot be links in a .mrpack, so they go inside it, like configs
            for (Mod mod : Builder.included(inst.mods)) {
                if (!CurseForge.isCurseForge(mod.projectId)) {
                    continue;
                }
                File jar = cachedCurseForgeFile(store, mod);
                zip.putNextEntry(new ZipEntry("overrides/" + mod.targetPath()));
                java.io.FileInputStream in = new java.io.FileInputStream(jar);
                try {
                    byte[] buffer = new byte[65536];
                    int n;
                    while ((n = in.read(buffer)) > 0) {
                        zip.write(buffer, 0, n);
                    }
                } finally {
                    in.close();
                }
                zip.closeEntry();
            }
            File source = store.sourcePack(inst.id);
            if (inst.hasSourcePack && source.isFile()) {
                ZipFile original = new ZipFile(source);
                try {
                    byte[] buffer = new byte[65536];
                    Enumeration<? extends ZipEntry> entries = original.entries();
                    while (entries.hasMoreElements()) {
                        ZipEntry entry = entries.nextElement();
                        if ("modrinth.index.json".equals(entry.getName()) || entry.isDirectory()) {
                            continue;
                        }
                        zip.putNextEntry(new ZipEntry(entry.getName()));
                        InputStream in = original.getInputStream(entry);
                        int n;
                        while ((n = in.read(buffer)) > 0) {
                            zip.write(buffer, 0, n);
                        }
                        in.close();
                        zip.closeEntry();
                    }
                } finally {
                    original.close();
                }
            }
        } finally {
            zip.close();
        }
    }
}
