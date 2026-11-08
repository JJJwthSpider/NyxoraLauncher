package com.sculkshoot.client;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.UUID;

/**
 * An installation, like an instance of the desktop launcher: Minecraft version, loader and the mods, resource packs
 * and shaders. Made by hand, from a Modrinth modpack or by Moony in a chat. "Play" hands it to an Android Java
 * Edition launcher as a .mrpack.
 */
final class Installation {
    String id = UUID.randomUUID().toString();
    String name = "";
    String minecraftVersion = "";
    String loader = "";
    String loaderVersion = "";
    String iconUrl = "";
    String chatId = "";      // made by Moony in this chat
    boolean hasSourcePack;   // made from a modpack: its original .mrpack (configs, worlds...) is kept next to the JSON
    long created = System.currentTimeMillis();
    long updated = created;
    boolean fpsBoost;        // "Буст FPS": lighter graphics in options.txt (FpsBoost)
    long playedMillis;       // how long it was played, like Millida's play time
    long lastPlayed;         // when it was started last (0: never)
    List<Mod> mods = new ArrayList<Mod>();

    String describe() {
        return "Minecraft " + minecraftVersion + " · " + Plan.loaderName(loader);
    }

    /** "3 ч 20 мин", "12 мин" or "" */
    static String playTime(long millis) {
        long minutes = millis / 60000;
        if (minutes <= 0) {
            return "";
        }
        return minutes >= 60 ? (minutes / 60) + " ч " + (minutes % 60) + " мин" : minutes + " мин";
    }

    int count(String kind) {
        int n = 0;
        for (Mod mod : mods) {
            if (kind.equals(mod.kind) && !mod.dependency) {
                n++;
            }
        }
        return n;
    }

    Mod find(String projectId) {
        for (Mod mod : mods) {
            if (!projectId.isEmpty() && projectId.equals(mod.projectId)) {
                return mod;
            }
        }
        return null;
    }

    JSONObject toJson() throws JSONException {
        JSONObject o = new JSONObject();
        o.put("formatVersion", 1);
        o.put("id", id);
        o.put("name", name);
        o.put("minecraftVersion", minecraftVersion);
        o.put("loader", loader);
        o.put("loaderVersion", loaderVersion);
        o.put("iconUrl", iconUrl);
        o.put("chatId", chatId);
        o.put("hasSourcePack", hasSourcePack);
        o.put("created", created);
        o.put("updated", updated);
        o.put("fpsBoost", fpsBoost);
        o.put("playedMillis", playedMillis);
        o.put("lastPlayed", lastPlayed);
        JSONArray list = new JSONArray();
        for (Mod mod : mods) {
            list.put(mod.toJson());
        }
        o.put("mods", list);
        return o;
    }

    static Installation fromJson(JSONObject o) {
        Installation i = new Installation();
        i.id = o.optString("id", i.id);
        i.name = o.optString("name");
        i.minecraftVersion = o.optString("minecraftVersion");
        i.loader = o.optString("loader");
        i.loaderVersion = o.optString("loaderVersion");
        i.iconUrl = o.optString("iconUrl");
        i.chatId = o.optString("chatId");
        i.hasSourcePack = o.optBoolean("hasSourcePack");
        i.created = o.optLong("created", i.created);
        i.updated = o.optLong("updated", i.created);
        i.fpsBoost = o.optBoolean("fpsBoost");
        i.playedMillis = o.optLong("playedMillis");
        i.lastPlayed = o.optLong("lastPlayed");
        JSONArray list = o.optJSONArray("mods");
        for (int k = 0; list != null && k < list.length(); k++) {
            i.mods.add(Mod.fromJson(list.optJSONObject(k)));
        }
        return i;
    }

    /** a copy with its own id (the original modpack file is copied by the store) */
    Installation copy(String newName) {
        try {
            Installation c = fromJson(toJson());
            c.id = UUID.randomUUID().toString();
            c.name = newName;
            c.chatId = "";
            c.created = System.currentTimeMillis();
            c.updated = c.created;
            c.playedMillis = 0;
            c.lastPlayed = 0;
            return c;
        } catch (JSONException e) {
            throw new IllegalStateException(e);
        }
    }

    /* ------------------------------------------------------------ storage */

    static final class Store {
        private final File dir;

        Store(File filesDir) {
            dir = new File(filesDir, "installations");
            dir.mkdirs();
        }

        /** downloaded CurseForge files (they travel inside the exported pack) */
        File curseForgeCache() {
            File cache = new File(dir.getParentFile(), "curseforge");
            cache.mkdirs();
            return cache;
        }

        File sourcePack(String id) {
            return new File(dir, id + ".mrpack");
        }

        List<Installation> list() {
            List<Installation> out = new ArrayList<Installation>();
            File[] files = dir.listFiles();
            for (int i = 0; files != null && i < files.length; i++) {
                if (files[i].getName().endsWith(".json")) {
                    Installation inst = load(files[i]);
                    if (inst != null) {
                        out.add(inst);
                    }
                }
            }
            // the most recently changed first, like "recently played" on the desktop
            Collections.sort(out, new Comparator<Installation>() {
                public int compare(Installation a, Installation b) {
                    return a.updated < b.updated ? 1 : a.updated > b.updated ? -1 : 0;
                }
            });
            return out;
        }

        Installation get(String id) {
            return load(new File(dir, id + ".json"));
        }

        Installation forChat(String chatId) {
            for (Installation inst : list()) {
                if (chatId.equals(inst.chatId)) {
                    return inst;
                }
            }
            return null;
        }

        private Installation load(File file) {
            try {
                return fromJson(new JSONObject(Http.read(new FileInputStream(file))));
            } catch (Exception e) {
                return null;
            }
        }

        void save(Installation inst) throws IOException {
            inst.updated = System.currentTimeMillis();
            File tmp = new File(dir, inst.id + ".tmp");
            FileOutputStream out = new FileOutputStream(tmp);
            try {
                out.write(inst.toJson().toString().getBytes("UTF-8"));
            } catch (JSONException e) {
                throw new IOException(e.getMessage());
            } finally {
                out.close();
            }
            if (!tmp.renameTo(new File(dir, inst.id + ".json"))) {
                throw new IOException("rename failed");
            }
        }

        void delete(String id) {
            new File(dir, id + ".json").delete();
            sourcePack(id).delete();
        }
    }
}
