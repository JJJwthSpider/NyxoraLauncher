package com.sculkshoot.client;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * "Буст FPS", like Millida Launcher's: lighter graphics in the installation's options.txt (render and simulation
 * distance, fast graphics, minimal particles, no clouds, no entity shadows, no smooth lighting, no biome blending, no
 * mipmaps). The values it replaced are remembered in .vellora-fpsboost, so switching it off puts back exactly what the
 * player had. The performance mods (Sodium and friends) are in every modded installation anyway.
 */
final class FpsBoost {
    private FpsBoost() {}

    static final String[][] SETTINGS = {
        { "renderDistance", "6" }, { "simulationDistance", "5" }, { "graphicsMode", "0" }, { "fancyGraphics", "false" },
        { "particles", "2" }, { "renderClouds", "\"false\"" }, { "entityShadows", "false" }, { "ao", "false" },
        { "biomeBlendRadius", "0" }, { "mipmapLevels", "0" }, { "entityDistanceScaling", "0.5" },
    };
    private static final String SAVED = ".vellora-fpsboost";
    private static final String NONE = "\u0000";  // the key was not in options.txt

    /** switches the boost on or off for the game folder of an installation; returns whether anything changed */
    static boolean apply(File gameDir, boolean on) throws IOException {
        File options = new File(gameDir, "options.txt");
        File saved = new File(gameDir, SAVED);
        Map<String, String> current = read(options);
        if (on) {
            Map<String, String> before = saved.isFile() ? read(saved) : new LinkedHashMap<String, String>();
            boolean changed = false;
            for (String[] setting : SETTINGS) {
                String old = current.get(setting[0]);
                if (setting[1].equals(old)) {
                    continue;
                }
                if (!before.containsKey(setting[0])) {
                    before.put(setting[0], old == null ? NONE : old);
                }
                current.put(setting[0], setting[1]);
                changed = true;
            }
            if (changed) {
                gameDir.mkdirs();
                write(options, current);
                write(saved, before);
            }
            return changed;
        }
        if (!saved.isFile()) {
            return false;
        }
        for (Map.Entry<String, String> entry : read(saved).entrySet()) {
            if (NONE.equals(entry.getValue())) {
                current.remove(entry.getKey());
            } else {
                current.put(entry.getKey(), entry.getValue());
            }
        }
        write(options, current);
        saved.delete();
        return true;
    }

    static boolean isOn(File gameDir) {
        return new File(gameDir, SAVED).isFile();
    }

    /** options.txt as key -> value, in file order */
    static Map<String, String> read(File file) throws IOException {
        Map<String, String> map = new LinkedHashMap<String, String>();
        if (!file.isFile()) {
            return map;
        }
        InputStream in = new FileInputStream(file);
        try {
            java.io.ByteArrayOutputStream bytes = new java.io.ByteArrayOutputStream();
            byte[] buffer = new byte[8192];
            int n;
            while ((n = in.read(buffer)) > 0) {
                bytes.write(buffer, 0, n);
            }
            for (String line : new String(bytes.toByteArray(), "UTF-8").split("\n")) {
                int colon = line.indexOf(':');
                if (colon > 0) {
                    map.put(line.substring(0, colon), line.substring(colon + 1).replace("\r", ""));
                }
            }
        } finally {
            in.close();
        }
        return map;
    }

    static void write(File file, Map<String, String> map) throws IOException {
        StringBuilder text = new StringBuilder();
        List<String> keys = new ArrayList<String>(map.keySet());
        for (String key : keys) {
            text.append(key).append(':').append(map.get(key)).append('\n');
        }
        FileOutputStream out = new FileOutputStream(file);
        try {
            out.write(text.toString().getBytes("UTF-8"));
        } finally {
            out.close();
        }
    }
}
