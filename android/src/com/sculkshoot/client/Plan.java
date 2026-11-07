package com.sculkshoot.client;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;

/** Everything Moony needs to know to assemble a pack. */
final class Plan {
    static final String DEFAULT_VERSION = "1.21.1";

    /** A group of mods searched for, e.g. "Зомби" with the searches "zombie", "zombie apocalypse". */
    static final class Theme {
        final String title;
        final List<String> queries;
        final int count;
        final List<String> matchWords;  // a found mod must mention one of these; empty: the queries

        Theme(String title, List<String> queries, int count, List<String> matchWords) {
            this.title = title;
            this.queries = queries;
            this.count = count;
            this.matchWords = matchWords;
        }
    }

    /** A hand-picked mod: the first slug that exists for the version and loader is used. */
    static final class CuratedMod {
        final List<String> alternatives;
        final String group;
        String kind = Mod.MOD;

        CuratedMod(String group, String... alternatives) {
            this.group = group;
            this.alternatives = Arrays.asList(alternatives);
        }

        /** a shader pack (goes to shaderpacks/ and needs Iris or Oculus), not a mod */
        CuratedMod shader() {
            kind = Mod.SHADER;
            return this;
        }
    }

    String name = "";
    String summary = "";
    String minecraftVersion = DEFAULT_VERSION;
    String loader;
    List<Theme> themes = new ArrayList<Theme>();
    List<CuratedMod> curated = new ArrayList<CuratedMod>();
    List<String> requestedMods = new ArrayList<String>();
    List<String> excluded = new ArrayList<String>();
    List<String> unknownWords = new ArrayList<String>();
    boolean versionGiven;
    boolean loaderGiven;

    static String defaultLoader(String minecraftVersion) {
        // NeoForge exists from 1.20.2, older versions use Forge
        return compareVersions(minecraftVersion, "1.20.2") >= 0 ? "neoforge" : "forge";
    }

    static int compareVersions(String a, String b) {
        String[] pa = a.split("\\.");
        String[] pb = b.split("\\.");
        for (int i = 0; i < Math.max(pa.length, pb.length); i++) {
            int x = i < pa.length ? parse(pa[i]) : 0;
            int y = i < pb.length ? parse(pb[i]) : 0;
            if (x != y) {
                return x < y ? -1 : 1;
            }
        }
        return 0;
    }

    private static int parse(String s) {
        try {
            return Integer.parseInt(s);
        } catch (NumberFormatException e) {
            return 0;
        }
    }

    static String loaderName(String loader) {
        if ("neoforge".equals(loader))
            return "NeoForge";
        if ("forge".equals(loader))
            return "Forge";
        if ("fabric".equals(loader))
            return "Fabric";
        if ("quilt".equals(loader))
            return "Quilt";
        return loader;
    }

    static List<CuratedMod> essentialMods(String loader, boolean wantsShaders, boolean wantsMap) {
        String performance = "Оптимизация";
        String comfort = "Удобство";
        boolean fabricLike = "fabric".equals(loader) || "quilt".equals(loader);
        List<CuratedMod> mods = new ArrayList<CuratedMod>();
        if ("forge".equals(loader)) {
            mods.add(new CuratedMod(performance, "embeddium", "rubidium"));
        } else {
            mods.add(new CuratedMod(performance, "sodium", "embeddium"));
        }
        if (fabricLike || "neoforge".equals(loader)) {
            mods.add(new CuratedMod(performance, "lithium"));
        }
        mods.add(new CuratedMod(performance, "ferrite-core"));
        mods.add(new CuratedMod(performance, "modernfix"));
        mods.add(new CuratedMod(performance, "entityculling"));
        mods.add(new CuratedMod(performance, "immediatelyfast"));
        mods.add(new CuratedMod(performance, "dynamic-fps"));
        mods.add(new CuratedMod(comfort, "jei", "emi"));
        mods.add(new CuratedMod(comfort, "jade", "wthit"));
        mods.add(new CuratedMod(comfort, "appleskin"));
        mods.add(new CuratedMod(comfort, "mouse-tweaks"));
        if (wantsShaders) {
            if ("forge".equals(loader)) {
                mods.add(new CuratedMod("Шейдеры", "oculus"));
            } else {
                mods.add(new CuratedMod("Шейдеры", "iris", "oculus"));
            }
            // the shaders themselves: the most popular pack, or the other well-known ones if it has no version yet
            mods.add(new CuratedMod("Шейдеры", "complementary-reimagined", "bsl-shaders", "makeup-ultra-fast-shaders").shader());
        }
        if (wantsMap) {
            mods.add(new CuratedMod("Карта", "xaeros-minimap", "journeymap"));
            mods.add(new CuratedMod("Карта", "xaeros-world-map"));
        }
        return mods;
    }

    /** Fills name and summary from the themes, version and loader. */
    void describe() {
        List<String> titles = new ArrayList<String>();
        for (Theme theme : themes) {
            titles.add(theme.title.toLowerCase(Locale.ROOT));
        }
        if (titles.isEmpty() && !requestedMods.isEmpty()) {
            name = requestedMods.get(0) + " — сборка " + minecraftVersion;
            summary = "Моды: " + Util.join(requestedMods, ", ") + ". А ещё оптимизация и удобство.";
        } else if (titles.isEmpty()) {
            name = "Оптимизированный " + loaderName(loader) + " " + minecraftVersion;
            summary = "Лёгкая сборка с модами на оптимизацию и удобство.";
        } else {
            String first = titles.get(0);
            name = Character.toUpperCase(first.charAt(0)) + first.substring(1) + " — сборка " + minecraftVersion;
            summary = "Моды про: " + Util.join(titles, ", ") + ". А ещё оптимизация и удобство.";
        }
    }

    /** True if the mod should be left out because the user asked for a pack without it. */
    boolean isExcluded(String title, String slug) {
        String name = Parser.normalized(title);
        String id = Parser.normalized(slug);
        for (String term : excluded) {
            String key = Parser.normalized(term);
            if (key.length() > 4 && key.endsWith("s")) {
                key = key.substring(0, key.length() - 1);
            }
            if (key.length() >= 3 && (name.contains(key) || id.contains(key))) {
                return true;
            }
        }
        return false;
    }
}
