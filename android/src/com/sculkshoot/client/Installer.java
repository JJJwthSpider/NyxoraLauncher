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
        plan.summary = "Lullaby Launcher: " + inst.describe();
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
