package com.sculkshoot.client;

import org.json.JSONObject;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/** Saved chats: one JSON file per conversation (and its pack) in the app's private storage. */
final class ChatStore {
    private final File dir;

    ChatStore(File filesDir) {
        dir = new File(filesDir, "chats");
        dir.mkdirs();
    }

    File fileFor(String id) {
        return new File(dir, id + ".json");
    }

    JSONObject load(String id) {
        try {
            FileInputStream in = new FileInputStream(fileFor(id));
            return new JSONObject(Http.read(in));
        } catch (Exception e) {
            return null;
        }
    }

    void save(String id, JSONObject chat) throws IOException {
        File tmp = new File(dir, id + ".tmp");
        FileOutputStream out = new FileOutputStream(tmp);
        try {
            out.write(chat.toString().getBytes("UTF-8"));
            out.getFD().sync();
        } finally {
            out.close();
        }
        if (!tmp.renameTo(fileFor(id))) {
            throw new IOException("rename failed");
        }
    }

    void delete(String id) {
        fileFor(id).delete();
    }

    /** All chats, the most recently changed first. */
    List<JSONObject> list() {
        List<JSONObject> chats = new ArrayList<JSONObject>();
        File[] files = dir.listFiles();
        if (files != null) {
            for (File file : files) {
                if (file.getName().endsWith(".json")) {
                    JSONObject chat = load(file.getName().substring(0, file.getName().length() - 5));
                    if (chat != null && !chat.optString("id").isEmpty()) {
                        chats.add(chat);
                    }
                }
            }
        }
        Collections.sort(chats, new Comparator<JSONObject>() {
            public int compare(JSONObject a, JSONObject b) {
                long x = a.optLong("updated");
                long y = b.optLong("updated");
                return x < y ? 1 : x > y ? -1 : 0;
            }
        });
        return chats;
    }
}
