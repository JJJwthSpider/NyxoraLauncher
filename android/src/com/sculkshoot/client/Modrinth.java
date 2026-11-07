package com.sculkshoot.client;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/** The Modrinth API and the loader version services, shared by Moony's builder and the manual installation editor. */
final class Modrinth {
    private Modrinth() {}

    /** The file of a project that fits the Minecraft version and loader. */
    static final class FileChoice {
        boolean valid;
        String versionNumber = "";
        String fileName = "";
        String url = "";
        String sha1 = "";
        String sha512 = "";
        long size;
        List<String> required = new ArrayList<String>();
        List<String> incompatible = new ArrayList<String>();
    }


    static JSONObject getObject(String url) throws IOException, JSONException {
        Http.Response r = Http.get(url);
        return r.ok() ? new JSONObject(r.body) : null;
    }

    /** the loaders whose files fit: Quilt runs Fabric mods too */
    static String loadersJson(String loader) {
        return "quilt".equals(loader) ? "[\"quilt\",\"fabric\"]" : "[\"" + loader + "\"]";
    }

    /** The newest release (else beta) file of a project for these loaders and this Minecraft version. */
    static FileChoice bestFile(String projectId, String loadersJson, String minecraftVersion) throws IOException, JSONException {
        FileChoice choice = new FileChoice();
        Http.Response r = Http.get(Builder.MODRINTH + "/project/" + projectId + "/version?loaders=" + Http.encode(loadersJson)
                                   + "&game_versions=" + Http.encode("[\"" + minecraftVersion + "\"]"));
        if (!r.ok()) {
            return choice;
        }
        JSONArray versions = new JSONArray(r.body);
        if (versions.length() == 0) {
            return choice;
        }
        // newest first; prefer a stable release over betas
        JSONObject version = versions.getJSONObject(0);
        for (int i = 0; i < versions.length(); i++) {
            if ("release".equals(versions.getJSONObject(i).optString("version_type"))) {
                version = versions.getJSONObject(i);
                break;
            }
        }
        JSONArray files = version.optJSONArray("files");
        JSONObject file = files != null && files.length() > 0 ? files.getJSONObject(0) : new JSONObject();
        for (int i = 0; files != null && i < files.length(); i++) {
            if (files.getJSONObject(i).optBoolean("primary")) {
                file = files.getJSONObject(i);
                break;
            }
        }
        JSONObject hashes = file.optJSONObject("hashes");
        choice.versionNumber = version.optString("version_number");
        choice.fileName = file.optString("filename");
        choice.url = file.optString("url");
        choice.sha1 = hashes != null ? hashes.optString("sha1") : "";
        choice.sha512 = hashes != null ? hashes.optString("sha512") : "";
        choice.size = file.optLong("size");
        JSONArray deps = version.optJSONArray("dependencies");
        for (int i = 0; deps != null && i < deps.length(); i++) {
            JSONObject dep = deps.getJSONObject(i);
            String depProject = dep.isNull("project_id") ? "" : dep.optString("project_id");
            String versionId = dep.isNull("version_id") ? "" : dep.optString("version_id");
            // some mods name only the version of a dependency; its project is looked up later
            String id = !depProject.isEmpty() ? "mr:" + depProject : !versionId.isEmpty() ? "mrv:" + versionId : "";
            if (id.isEmpty()) {
                continue;
            }
            String type = dep.optString("dependency_type");
            if ("required".equals(type)) {
                choice.required.add(id);
            } else if ("incompatible".equals(type) && !depProject.isEmpty()) {
                choice.incompatible.add(id);
            }
        }
        choice.valid = !choice.fileName.isEmpty() && !choice.url.isEmpty() && !choice.sha512.isEmpty();
        return choice;
    }

    /** The newest loader version for a Minecraft version, or "" if the loader does not support it. */
    static String loaderVersion(String loader, String mc) throws IOException, JSONException {
        String loaderVersion = "";
        if ("fabric".equals(loader) || "quilt".equals(loader)) {
            String base = "fabric".equals(loader) ? Builder.FABRIC_META + "/v2" : Builder.QUILT_META + "/v3";
            Http.Response r = Http.get(base + "/versions/loader/" + Http.encode(mc));
            if (r.ok()) {
                JSONArray list = new JSONArray(r.body);
                for (int i = 0; i < list.length() && loaderVersion.isEmpty(); i++) {
                    JSONObject entry = list.getJSONObject(i).optJSONObject("loader");
                    // Quilt marks no versions stable: take the first that is not a beta
                    if (entry != null && (entry.optBoolean("stable") || ("quilt".equals(loader) && !entry.optString("version").contains("beta")))) {
                        loaderVersion = entry.optString("version");
                    }
                }
                if (loaderVersion.isEmpty() && list.length() > 0 && list.getJSONObject(0).optJSONObject("loader") != null) {
                    loaderVersion = list.getJSONObject(0).getJSONObject("loader").optString("version");
                }
            }
        } else if ("forge".equals(loader)) {
            JSONObject promos = getObject(Builder.FORGE_PROMOTIONS);
            if (promos != null) {
                JSONObject p = promos.optJSONObject("promos");
                if (p != null) {
                    loaderVersion = p.optString(mc + "-recommended", p.optString(mc + "-latest", ""));
                }
            }
        } else {
            // NeoForge numbers its versions after Minecraft: 1.21.1 -> 21.1.x; 1.20.1 still used the "forge" artifact
            boolean legacy = "1.20.1".equals(mc);
            JSONObject list = getObject(Builder.NEOFORGE_MAVEN + "/api/maven/versions/releases/net/neoforged/" + (legacy ? "forge" : "neoforge"));
            String prefix;
            if (legacy) {
                prefix = "1.20.1-";
            } else if (mc.startsWith("1.")) {
                String[] parts = mc.substring(2).split("\\.");
                prefix = parts[0] + "." + (parts.length > 1 ? parts[1] : "0") + ".";
            } else {
                prefix = mc + ".";
            }
            if (list != null) {
                JSONArray versions = list.optJSONArray("versions");
                String beta = "";
                for (int i = 0; versions != null && i < versions.length(); i++) {
                    String v = versions.optString(i);
                    if (v.startsWith(prefix)) {
                        if (v.contains("beta") || v.contains("alpha")) {
                            beta = v;
                        } else {
                            loaderVersion = v;
                        }
                    }
                }
                if (loaderVersion.isEmpty()) {
                    loaderVersion = beta;
                }
                if (legacy && loaderVersion.startsWith(prefix)) {
                    loaderVersion = loaderVersion.substring(prefix.length());
                }
            }
        }
        return loaderVersion;
    }

    /** Release versions of Minecraft, newest first. */
    static List<String> gameVersions() throws IOException, JSONException {
        List<String> out = new ArrayList<String>();
        Http.Response r = Http.get(Builder.MODRINTH + "/tag/game_version");
        if (!r.ok()) {
            throw new IOException("HTTP " + r.status);
        }
        JSONArray versions = new JSONArray(r.body);
        for (int i = 0; i < versions.length(); i++) {
            JSONObject v = versions.getJSONObject(i);
            if ("release".equals(v.optString("version_type"))) {
                out.add(v.optString("version"));
            }
        }
        return out;
    }

    /** One page of search results. */
    static JSONArray search(String query, String facets, String index, int offset, int limit) throws IOException, JSONException {
        Http.Response r = Http.get(Builder.MODRINTH + "/search?query=" + Http.encode(query) + "&facets=" + Http.encode(facets) + "&index=" + index
                                   + "&offset=" + offset + "&limit=" + limit);
        if (!r.ok()) {
            throw new IOException("HTTP " + r.status);
        }
        return new JSONObject(r.body).optJSONArray("hits");
    }

    /** many projects in one request (at most 100 ids): their titles, descriptions and icons */
    static JSONArray projects(List<String> ids) throws IOException, JSONException {
        JSONArray list = new JSONArray();
        for (String id : ids) {
            list.put(id);
        }
        Http.Response r = Http.get(Builder.MODRINTH + "/projects?ids=" + Http.encode(list.toString()));
        if (!r.ok()) {
            throw new IOException("HTTP " + r.status);
        }
        return new JSONArray(r.body);
    }

    static JSONObject project(String idOrSlug) throws IOException, JSONException {
        return getObject(Builder.MODRINTH + "/project/" + Http.encode(idOrSlug));
    }
}
