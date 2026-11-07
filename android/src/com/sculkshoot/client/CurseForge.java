package com.sculkshoot.client;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * CurseForge access for the mod browser: search, the newest fitting file and its libraries. CurseForge projects
 * are kept as "cf:<id>" so they never mix with Modrinth ids. A file whose author forbids downloads outside the
 * CurseForge app has no download link and cannot be installed.
 */
final class CurseForge {
    private CurseForge() {}

    static String API = "https://api.curseforge.com/v1";
    /** the CurseForge API key of the desktop launcher's build (CMakeLists.txt: Launcher_CURSEFORGE_API_KEY) */
    static String KEY = "$2a$10$wuAJuNZuted3NORVmpgUC.m8sI.pv1tOPKZyBgLFGjxFp/br0lZCC";
    static final String PREFIX = "cf:";
    private static final int MINECRAFT = 432;

    static boolean isCurseForge(String projectId) {
        return projectId != null && projectId.startsWith(PREFIX);
    }

    static final int CLASS_MODPACK = 4471;

    static int classId(String kind) {
        if (BrowserScreen.MODPACK.equals(kind)) {
            return CLASS_MODPACK;
        }
        if (Mod.RESOURCE_PACK.equals(kind)) {
            return 12;
        }
        if (Mod.SHADER.equals(kind)) {
            return 6552;
        }
        return 6;
    }

    /** CurseForge's loader numbers */
    static int loaderType(String loader) {
        if ("forge".equals(loader)) {
            return 1;
        }
        if ("fabric".equals(loader)) {
            return 4;
        }
        if ("quilt".equals(loader)) {
            return 5;
        }
        if ("neoforge".equals(loader)) {
            return 6;
        }
        return 0;
    }

    /** the sorts of the browser (Modrinth names) as CurseForge sort fields */
    static int sortField(String modrinthIndex) {
        if ("downloads".equals(modrinthIndex)) {
            return 6;   // total downloads
        }
        if ("updated".equals(modrinthIndex)) {
            return 3;   // last updated
        }
        if ("newest".equals(modrinthIndex)) {
            return 11;  // released date
        }
        return 2;       // popularity
    }

    private static JSONObject get(String path) throws IOException, JSONException {
        Http.Response r = Http.get(API + path, "x-api-key", KEY);
        if (r.status == 403 || r.status == 401) {
            throw new IOException("CurseForge не принял ключ API");
        }
        if (!r.ok()) {
            throw new IOException("CurseForge: HTTP " + r.status);
        }
        return new JSONObject(r.body);
    }

    /** search results in the browser's (Modrinth's) shape: project_id, title, description, author, downloads, icon_url */
    static JSONArray search(String query, Installation inst, String kind, String index, int offset, int limit) throws IOException, JSONException {
        StringBuilder path = new StringBuilder("/mods/search?gameId=" + MINECRAFT + "&classId=" + classId(kind));
        path.append("&searchFilter=").append(Http.encode(query));
        path.append("&sortField=").append(sortField(index)).append("&sortOrder=desc");
        if (inst != null) {
            path.append("&gameVersion=").append(Http.encode(inst.minecraftVersion));
        }
        if (inst != null && Mod.MOD.equals(kind) && loaderType(inst.loader) != 0 && !"quilt".equals(inst.loader)) {
            path.append("&modLoaderType=").append(loaderType(inst.loader));
        }
        path.append("&index=").append(offset).append("&pageSize=").append(limit);
        JSONArray data = get(path.toString()).optJSONArray("data");
        JSONArray hits = new JSONArray();
        for (int i = 0; data != null && i < data.length(); i++) {
            hits.put(toHit(data.getJSONObject(i)));
        }
        return hits;
    }

    static JSONObject toHit(JSONObject mod) throws JSONException {
        JSONObject hit = new JSONObject();
        hit.put("project_id", PREFIX + mod.optLong("id"));
        hit.put("slug", mod.optString("slug"));
        hit.put("title", mod.optString("name"));
        hit.put("description", mod.optString("summary"));
        JSONArray authors = mod.optJSONArray("authors");
        hit.put("author", authors != null && authors.length() > 0 ? authors.getJSONObject(0).optString("name") : "CurseForge");
        hit.put("downloads", mod.optLong("downloadCount"));
        JSONObject logo = mod.optJSONObject("logo");
        hit.put("icon_url", logo != null ? logo.optString("thumbnailUrl", logo.optString("url")) : "");
        return hit;
    }

    /** a POST with a JSON body (the batch requests: many files or projects at once) */
    static JSONObject post(String path, JSONObject body) throws IOException, JSONException {
        Http.Response r = Http.post(API + path, "application/json", body.toString(), "x-api-key", KEY);
        if (r.status == 403 || r.status == 401) {
            throw new IOException("CurseForge не принял ключ API");
        }
        if (!r.ok()) {
            throw new IOException("CurseForge: HTTP " + r.status);
        }
        return new JSONObject(r.body);
    }

    /** the newest release file of a modpack (else the newest file at all) */
    static JSONObject newestPackFile(String projectId) throws IOException, JSONException {
        JSONArray files = get("/mods/" + projectId.substring(PREFIX.length()) + "/files?pageSize=50").optJSONArray("data");
        JSONObject best = null;
        for (int i = 0; files != null && i < files.length(); i++) {
            JSONObject f = files.getJSONObject(i);
            if (best == null || f.optInt("releaseType", 3) < best.optInt("releaseType", 3)
                || (f.optInt("releaseType", 3) == best.optInt("releaseType", 3) && f.optString("fileDate").compareTo(best.optString("fileDate")) > 0)) {
                best = f;
            }
        }
        return best;
    }

    static JSONObject project(String projectId) throws IOException, JSONException {
        return get("/mods/" + projectId.substring(PREFIX.length())).optJSONObject("data");
    }

    /** the newest fitting file (release before beta before alpha); null if there is none; blocked if it cannot be downloaded */
    static final class FileChoice {
        JSONObject file;
        boolean blocked;
    }

    static FileChoice bestFile(String projectId, Installation inst, String kind) throws IOException, JSONException {
        String id = projectId.substring(PREFIX.length());
        List<Integer> loaders = new ArrayList<Integer>();
        if (Mod.MOD.equals(kind)) {
            loaders.add(loaderType(inst.loader));
            if ("quilt".equals(inst.loader)) {
                loaders.add(4);  // Quilt runs Fabric mods
            }
        } else {
            loaders.add(0);
        }
        FileChoice choice = new FileChoice();
        for (int loader : loaders) {
            String path = "/mods/" + id + "/files?gameVersion=" + Http.encode(inst.minecraftVersion) + (loader != 0 ? "&modLoaderType=" + loader : "")
                          + "&pageSize=50";
            JSONArray files = get(path).optJSONArray("data");
            JSONObject best = null;
            for (int i = 0; files != null && i < files.length(); i++) {
                JSONObject f = files.getJSONObject(i);
                if (!fits(f, inst, kind, loader)) {
                    continue;
                }
                if (best == null || f.optInt("releaseType", 3) < best.optInt("releaseType", 3)
                    || (f.optInt("releaseType", 3) == best.optInt("releaseType", 3) && f.optString("fileDate").compareTo(best.optString("fileDate")) > 0)) {
                    best = f;
                }
            }
            if (best != null) {
                choice.file = best;
                choice.blocked = best.isNull("downloadUrl") || best.optString("downloadUrl").isEmpty();
                return choice;
            }
        }
        return choice;
    }

    private static boolean fits(JSONObject file, Installation inst, String kind, int loader) {
        JSONArray versions = file.optJSONArray("gameVersions");
        boolean version = false;
        boolean loaderOk = loader == 0;
        String loaderName = loader == 1 ? "forge" : loader == 4 ? "fabric" : loader == 5 ? "quilt" : loader == 6 ? "neoforge" : "";
        for (int i = 0; versions != null && i < versions.length(); i++) {
            String v = versions.optString(i);
            version |= v.equals(inst.minecraftVersion);
            loaderOk |= v.equalsIgnoreCase(loaderName);
        }
        return version && loaderOk;
    }

    /** a file as a mod of the installation; libraries it needs are "mr:cf:<id>", like Modrinth's "mr:<id>" */
    static Mod toMod(JSONObject project, JSONObject file, String kind, boolean dependency) throws JSONException {
        Mod mod = new Mod();
        mod.kind = kind;
        mod.projectId = PREFIX + project.optLong("id");
        mod.slug = project.optString("slug");
        mod.title = project.optString("name");
        mod.description = project.optString("summary");
        JSONObject logo = project.optJSONObject("logo");
        mod.iconUrl = logo != null ? logo.optString("thumbnailUrl", logo.optString("url")) : "";
        mod.group = dependency ? Builder.GROUP_LIBRARIES : "";
        mod.dependency = dependency;
        mod.versionNumber = file.optString("displayName");
        mod.fileName = file.optString("fileName");
        mod.url = file.isNull("downloadUrl") ? "" : file.optString("downloadUrl");  // null: the author allows the CurseForge app only
        mod.size = file.optLong("fileLength");
        JSONArray hashes = file.optJSONArray("hashes");
        for (int i = 0; hashes != null && i < hashes.length(); i++) {
            if (hashes.getJSONObject(i).optInt("algo") == 1) {
                mod.sha1 = hashes.getJSONObject(i).optString("value");
            }
        }
        JSONArray deps = file.optJSONArray("dependencies");
        for (int i = 0; deps != null && i < deps.length(); i++) {
            JSONObject d = deps.getJSONObject(i);
            String ref = "mr:" + PREFIX + d.optLong("modId");
            if (d.optInt("relationType") == 3) {
                mod.required.add(ref);
            } else if (d.optInt("relationType") == 5) {
                mod.incompatible.add(ref);
            }
        }
        return mod;
    }
}
