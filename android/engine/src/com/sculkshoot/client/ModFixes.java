package com.sculkshoot.client;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.zip.CRC32;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

/**
 * Fixes for mods that refuse to run on a phone.
 *
 * Sodium (and the mods that ship it inside) stops the game on purpose as soon as it sees the engine's
 * POJAV_RENDERER variable: "It appears that you are using PojavLauncher, which is not supported when using Sodium".
 * The game then closes right after the window opens. Lullaby renames the variable inside Sodium's classes (a name of
 * the same length, which nothing sets), so the check finds nothing and the game starts. Nothing else in the mod
 * changes. Every fixed jar is remembered with its size and time, so it is neither fixed again nor downloaded again.
 */
final class ModFixes {
    private static final byte[] FROM = ascii("POJAV_RENDERER");
    private static final byte[] TO = ascii("LULLABY_NONE_0");
    private static final String STATE = ".lullaby-fixed";

    private ModFixes() {
    }

    /** whether a jar may have the check: Sodium itself and its forks and bundles (Sodium Extra etc. are just scanned) */
    static boolean candidate(String fileName) {
        String name = fileName.toLowerCase(Locale.ROOT);
        return name.endsWith(".jar") && name.contains("sodium");
    }

    /** fixes the jars in the mods folder; returns the names of the jars that were changed now */
    static List<String> fixMods(File modsDir) throws IOException {
        List<String> changed = new ArrayList<String>();
        File[] files = modsDir.listFiles();
        if (files == null) {
            return changed;
        }
        Map<String, String> state = readState(modsDir);
        boolean dirty = false;
        for (File jar : files) {
            if (!jar.isFile() || !candidate(jar.getName())) {
                continue;
            }
            if (stamp(jar).equals(state.get(jar.getName()))) {
                continue;  // already looked at, unchanged since
            }
            byte[] fixed = fixJar(readAll(jar));
            if (fixed != null) {
                File part = new File(jar.getPath() + ".fix");
                FileOutputStream out = new FileOutputStream(part);
                try {
                    out.write(fixed);
                } finally {
                    out.close();
                }
                if (!jar.delete() || !part.renameTo(jar)) {
                    part.delete();
                    throw new IOException("не удалось исправить " + jar.getName());
                }
                changed.add(jar.getName());
            }
            state.put(jar.getName(), stamp(jar));
            dirty = true;
        }
        if (dirty) {
            writeState(modsDir, state);
        }
        return changed;
    }

    /** whether this file is exactly what fixMods left (so its checksum differs from the download on purpose) */
    static boolean fixedUnchanged(File jar) {
        try {
            return jar.isFile() && stamp(jar).equals(readState(jar.getParentFile()).get(jar.getName()));
        } catch (IOException e) {
            return false;
        }
    }

    /** the jar with the check renamed (also in jars inside it), or null when there was nothing to change */
    static byte[] fixJar(byte[] jar) throws IOException {
        List<ZipEntry> entries = new ArrayList<ZipEntry>();
        List<byte[]> contents = new ArrayList<byte[]>();
        boolean changed = false;
        ZipInputStream in = new ZipInputStream(new ByteArrayInputStream(jar));
        try {
            ZipEntry entry;
            while ((entry = in.getNextEntry()) != null) {
                byte[] data = readAll(in);
                String name = entry.getName();
                if (name.endsWith(".class") && indexOf(data, FROM) >= 0) {
                    data = replaceAll(data);
                    changed = true;
                } else if (name.endsWith(".jar")) {
                    byte[] inner = fixJar(data);
                    if (inner != null) {
                        data = inner;
                        changed = true;
                    }
                }
                entries.add(entry);
                contents.add(data);
            }
        } finally {
            in.close();
        }
        if (!changed) {
            return null;
        }
        ByteArrayOutputStream bytes = new ByteArrayOutputStream(jar.length + 1024);
        ZipOutputStream out = new ZipOutputStream(bytes);
        for (int i = 0; i < entries.size(); i++) {
            ZipEntry old = entries.get(i);
            byte[] data = contents.get(i);
            ZipEntry entry = new ZipEntry(old.getName());
            if (old.getTime() != -1) {
                entry.setTime(old.getTime());
            }
            if (old.getMethod() == ZipEntry.STORED) {
                // jars inside jars are often stored uncompressed, and the loaders read them in place
                CRC32 crc = new CRC32();
                crc.update(data);
                entry.setMethod(ZipEntry.STORED);
                entry.setSize(data.length);
                entry.setCompressedSize(data.length);
                entry.setCrc(crc.getValue());
            }
            out.putNextEntry(entry);
            out.write(data);
            out.closeEntry();
        }
        out.close();
        return bytes.toByteArray();
    }

    private static byte[] replaceAll(byte[] data) {
        byte[] copy = data.clone();
        int at = 0;
        while ((at = indexOf(copy, FROM, at)) >= 0) {
            System.arraycopy(TO, 0, copy, at, TO.length);
            at += TO.length;
        }
        return copy;
    }

    private static int indexOf(byte[] data, byte[] what) {
        return indexOf(data, what, 0);
    }

    private static int indexOf(byte[] data, byte[] what, int from) {
        outer:
        for (int i = from; i <= data.length - what.length; i++) {
            for (int j = 0; j < what.length; j++) {
                if (data[i + j] != what[j]) {
                    continue outer;
                }
            }
            return i;
        }
        return -1;
    }

    private static String stamp(File file) {
        return file.length() + ":" + file.lastModified();
    }

    private static Map<String, String> readState(File modsDir) throws IOException {
        Map<String, String> state = new LinkedHashMap<String, String>();
        File file = new File(modsDir, STATE);
        if (!file.isFile()) {
            return state;
        }
        for (String line : new String(readAll(file), "UTF-8").split("\n")) {
            int tab = line.indexOf('\t');
            if (tab > 0) {
                state.put(line.substring(0, tab), line.substring(tab + 1).trim());
            }
        }
        return state;
    }

    private static void writeState(File modsDir, Map<String, String> state) throws IOException {
        StringBuilder text = new StringBuilder();
        for (Map.Entry<String, String> entry : state.entrySet()) {
            if (new File(modsDir, entry.getKey()).isFile()) {
                text.append(entry.getKey()).append('\t').append(entry.getValue()).append('\n');
            }
        }
        FileOutputStream out = new FileOutputStream(new File(modsDir, STATE));
        try {
            out.write(text.toString().getBytes("UTF-8"));
        } finally {
            out.close();
        }
    }

    private static byte[] readAll(File file) throws IOException {
        InputStream in = new FileInputStream(file);
        try {
            return readAll(in);
        } finally {
            in.close();
        }
    }

    private static byte[] readAll(InputStream in) throws IOException {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] buffer = new byte[16384];
        int n;
        while ((n = in.read(buffer)) > 0) {
            out.write(buffer, 0, n);
        }
        return out.toByteArray();
    }

    private static byte[] ascii(String text) {
        try {
            return text.getBytes("US-ASCII");
        } catch (java.io.UnsupportedEncodingException e) {
            throw new IllegalStateException(e);
        }
    }
}
