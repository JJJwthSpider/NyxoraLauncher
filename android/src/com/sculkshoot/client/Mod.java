package com.sculkshoot.client;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

/** One mod file of the pack (Modrinth). */
final class Mod {
    String projectId = "";
    String slug = "";
    String title = "";
    String description = "";
    String group = "";
    String versionNumber = "";
    String fileName = "";
    String url = "";
    String sha1 = "";
    String sha512 = "";
    long size;
    boolean dependency;  // pulled in because another mod needs it
    boolean requested;   // the user named it
    boolean enabled = true;
    List<String> required = new ArrayList<String>();  // "mr:<project>" or "mrv:<version>"
    List<String> incompatible = new ArrayList<String>();
    List<String> neededBy = new ArrayList<String>();

    JSONObject toJson() throws JSONException {
        JSONObject o = new JSONObject();
        o.put("projectId", projectId);
        o.put("slug", slug);
        o.put("title", title);
        o.put("description", description);
        o.put("group", group);
        o.put("versionNumber", versionNumber);
        o.put("fileName", fileName);
        o.put("url", url);
        o.put("sha1", sha1);
        o.put("sha512", sha512);
        o.put("size", size);
        o.put("dependency", dependency);
        o.put("requested", requested);
        o.put("enabled", enabled);
        o.put("required", new JSONArray(required));
        o.put("incompatible", new JSONArray(incompatible));
        o.put("neededBy", new JSONArray(neededBy));
        return o;
    }

    static Mod fromJson(JSONObject o) {
        Mod m = new Mod();
        m.projectId = o.optString("projectId");
        m.slug = o.optString("slug");
        m.title = o.optString("title");
        m.description = o.optString("description");
        m.group = o.optString("group");
        m.versionNumber = o.optString("versionNumber");
        m.fileName = o.optString("fileName");
        m.url = o.optString("url");
        m.sha1 = o.optString("sha1");
        m.sha512 = o.optString("sha512");
        m.size = o.optLong("size");
        m.dependency = o.optBoolean("dependency");
        m.requested = o.optBoolean("requested");
        m.enabled = o.optBoolean("enabled", true);
        m.required = strings(o.optJSONArray("required"));
        m.incompatible = strings(o.optJSONArray("incompatible"));
        m.neededBy = strings(o.optJSONArray("neededBy"));
        return m;
    }

    static List<String> strings(JSONArray array) {
        List<String> out = new ArrayList<String>();
        if (array != null) {
            for (int i = 0; i < array.length(); i++) {
                out.add(array.optString(i));
            }
        }
        return out;
    }
}
