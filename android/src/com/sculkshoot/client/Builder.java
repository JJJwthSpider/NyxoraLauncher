package com.sculkshoot.client;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.regex.Pattern;

/**
 * Turns a Plan into a list of compatible Modrinth files, like the desktop ModpackBuilder: the mods the user named,
 * performance and comfort mods, mods for every theme (only ones really about the theme), every required library,
 * no duplicates or incompatible mods, and the newest loader version. Runs on a background thread.
 */
final class Builder {
    /** Can be pointed at a test server. */
    static String MODRINTH = "https://api.modrinth.com/v2";  // also used by Modrinth
    static String FABRIC_META = "https://meta.fabricmc.net";
    static String QUILT_META = "https://meta.quiltmc.org";
    static String FORGE_PROMOTIONS = "https://files.minecraftforge.net/net/minecraftforge/forge/promotions_slim.json";
    static String NEOFORGE_MAVEN = "https://maven.neoforged.net";

    interface Listener {
        void log(String line);
    }

    static final class BuildException extends Exception {
        BuildException(String message) {
            super(message);
        }
    }

    static final String GROUP_REQUESTED = "По твоей просьбе";
    static final String GROUP_LIBRARIES = "Библиотеки";

    private static final int MINIMUM_DOWNLOADS = 2000;
    private static final int MAX_DEPENDENCY_ROUNDS = 5;
    private static final int SEARCH_LIMIT = 15;

    final Plan plan;
    private final Listener listener;
    private final AtomicBoolean cancelled = new AtomicBoolean();
    final List<Mod> mods = new ArrayList<Mod>();
    String loaderVersion = "";
    final List<String> foundRequested = new ArrayList<String>();
    final List<String> missingRequested = new ArrayList<String>();
    final List<String> emptyThemes = new ArrayList<String>();
    String suggestedSetup = "";
    int suggestedSetupMods;
    private final Set<String> tried = Collections.synchronizedSet(new HashSet<String>());
    private final Map<String, String> libraryNames = Collections.synchronizedMap(new HashMap<String, String>());
    private int[] themeTaken = new int[0];
    private ExecutorService pool;

    Builder(Plan plan, Listener listener) {
        this.plan = plan;
        this.listener = listener;
    }

    void cancel() {
        cancelled.set(true);
    }

    boolean isCancelled() {
        return cancelled.get();
    }

    private void log(String line) {
        if (!cancelled.get()) {
            listener.log(line);
        }
    }

    /* ------------------------------------------------------------ candidates and files */

    static final class Candidate {
        String id = "";
        String slug = "";
        String title = "";
        String description = "";
        String iconUrl = "";

        static Candidate from(JSONObject project) {
            Candidate c = new Candidate();
            c.id = project.optString("id", project.optString("project_id"));
            if (c.id.isEmpty()) {
                c.id = project.optString("project_id");
            }
            c.slug = project.optString("slug");
            c.title = project.optString("title");
            c.description = project.optString("description");
            c.iconUrl = project.isNull("icon_url") ? "" : project.optString("icon_url");
            return c;
        }
    }

    private JSONObject getObject(String url) throws IOException, JSONException {
        Http.Response r = Http.get(url);
        return r.ok() ? new JSONObject(r.body) : null;
    }

    private String loadersJson() {
        return Modrinth.loadersJson(plan.loader);
    }

    private String facets() {
        if ("quilt".equals(plan.loader)) {
            return "[[\"categories:quilt\",\"categories:fabric\"],[\"versions:" + plan.minecraftVersion + "\"],[\"project_type:mod\"]]";
        }
        return "[[\"categories:" + plan.loader + "\"],[\"versions:" + plan.minecraftVersion + "\"],[\"project_type:mod\"]]";
    }

    private JSONArray search(String query, String facets, int limit) throws IOException, JSONException {
        Http.Response r = Http.get(MODRINTH + "/search?query=" + Http.encode(query) + "&facets=" + Http.encode(facets)
                                   + "&index=relevance&limit=" + limit);
        return r.ok() ? new JSONObject(r.body).optJSONArray("hits") : new JSONArray();
    }

    private Candidate project(String idOrSlug) throws IOException, JSONException {
        return project(idOrSlug, Mod.MOD);
    }

    /** a project of this kind ("mod" or "shader"); null if there is none */
    private Candidate project(String idOrSlug, String kind) throws IOException, JSONException {
        JSONObject project = getObject(MODRINTH + "/project/" + Http.encode(idOrSlug));
        if (project == null || !kind.equals(project.optString("project_type"))) {
            return null;
        }
        return Candidate.from(project);
    }

    private Modrinth.FileChoice file(Candidate candidate) throws IOException, JSONException {
        tried.add(candidate.id);
        return Modrinth.bestFile(candidate.id, loadersJson(), plan.minecraftVersion);
    }

    private synchronized Mod findSame(Candidate c) {
        String name = Parser.normalized(c.title);
        String slug = Parser.normalized(c.slug);
        for (Mod mod : mods) {
            if (mod.projectId.equals(c.id) || (!name.isEmpty() && Parser.normalized(mod.title).equals(name))
                || (!slug.isEmpty() && Parser.normalized(mod.slug).equals(slug))) {
                return mod;
            }
        }
        return null;
    }

    /** Adds a mod; false if it is already there, conflicts with another mod or the user asked to leave it out. */
    private synchronized boolean addMod(Candidate c, Modrinth.FileChoice file, String group, boolean dependency) {
        return addMod(c, file, group, dependency, Mod.MOD);
    }

    private synchronized boolean addMod(Candidate c, Modrinth.FileChoice file, String group, boolean dependency, String kind) {
        if (findSame(c) != null || !file.valid) {
            return false;
        }
        if (!dependency && plan.isExcluded(c.title, c.slug)) {
            log("Пропустил " + c.title + ": по твоей просьбе сборка без него.");
            return false;
        }
        for (Mod other : mods) {
            if (file.incompatible.contains("mr:" + other.projectId) || other.incompatible.contains("mr:" + c.id)) {
                log("Пропустил " + c.title + ": не работает вместе с " + other.title + ".");
                return false;
            }
        }
        Mod mod = new Mod();
        mod.kind = kind;
        mod.projectId = c.id;
        mod.slug = c.slug;
        mod.title = c.title;
        mod.description = c.description;
        mod.iconUrl = c.iconUrl;
        mod.group = group;
        mod.dependency = dependency;
        mod.versionNumber = file.versionNumber;
        mod.fileName = file.fileName;
        mod.url = file.url;
        mod.sha1 = file.sha1;
        mod.sha512 = file.sha512;
        mod.size = file.size;
        mod.required = file.required;
        mod.incompatible = file.incompatible;
        mods.add(mod);
        log(dependency ? "+ " + c.title + " (нужен другим модам)" : "+ " + c.title);
        return true;
    }

    /* ------------------------------------------------------------ build */

    void run() throws Exception {
        pool = Executors.newFixedThreadPool(6);
        try {
            log("Minecraft " + plan.minecraftVersion + ", " + Plan.loaderName(plan.loader) + ", моды с Modrinth.");
            checkVersion();
            resolveLoaderVersion();
            resolveRequested();
            resolveCurated();
            searchThemes();
            resolveDependencies();
            finish();
        } finally {
            pool.shutdownNow();
        }
    }

    private void checkCancelled() throws BuildException {
        if (cancelled.get()) {
            throw new BuildException("отменено");
        }
    }

    private void checkVersion() throws Exception {
        log("Проверяю, что Minecraft " + plan.minecraftVersion + " существует...");
        Http.Response r = Http.get(MODRINTH + "/tag/game_version");
        if (!r.ok()) {
            throw new BuildException("не удалось загрузить список версий Minecraft. Проверь интернет.");
        }
        JSONArray versions = new JSONArray(r.body);
        for (int i = 0; i < versions.length(); i++) {
            if (plan.minecraftVersion.equals(versions.getJSONObject(i).optString("version"))) {
                return;
            }
        }
        throw new BuildException("Minecraft " + plan.minecraftVersion + " не существует. Проверь номер версии.");
    }

    private void resolveLoaderVersion() throws Exception {
        checkCancelled();
        log("Ищу свежий " + Plan.loaderName(plan.loader) + " для Minecraft " + plan.minecraftVersion + "...");
        String mc = plan.minecraftVersion;
        loaderVersion = Modrinth.loaderVersion(plan.loader, mc);
        if (loaderVersion.isEmpty()) {
            throw new BuildException(Plan.loaderName(plan.loader) + " нет для Minecraft " + mc + ". Попробуй другую версию или загрузчик.");
        }
        log("Будет установлен " + Plan.loaderName(plan.loader) + " " + loaderVersion + ".");
    }

    /** How well a project matches a name the user wrote: 3 exact, 2 starts with it, 1 contains it, 0 no match. */
    static int nameScore(String name, Candidate c) {
        String wanted = Parser.normalized(name);
        String title = Parser.normalized(c.title);
        String slug = Parser.normalized(c.slug);
        if (wanted.isEmpty())
            return 0;
        if (title.equals(wanted) || slug.equals(wanted))
            return 3;
        if (title.startsWith(wanted) || slug.startsWith(wanted))
            return 2;
        if (title.contains(wanted) || slug.contains(wanted))
            return 1;
        return 0;
    }

    private void resolveRequested() throws Exception {
        if (plan.requestedMods.isEmpty()) {
            return;
        }
        checkCancelled();
        log("Ищу названные тобой моды: " + Util.join(plan.requestedMods, ", ") + "...");
        List<Future<?>> jobs = new ArrayList<Future<?>>();
        for (final String name : plan.requestedMods) {
            jobs.add(pool.submit(new java.util.concurrent.Callable<Void>() {
                public Void call() throws Exception {
                    findRequested(name);
                    return null;
                }
            }));
        }
        waitAll(jobs);
    }

    private void findRequested(final String name) throws Exception {
        JSONArray hits = search(name, facets(), 10);
        List<Candidate> candidates = new ArrayList<Candidate>();
        for (int i = 0; i < hits.length(); i++) {
            Candidate c = Candidate.from(hits.getJSONObject(i));
            if (nameScore(name, c) > 0) {
                candidates.add(c);
            }
        }
        Collections.sort(candidates, new Comparator<Candidate>() {
            public int compare(Candidate a, Candidate b) {
                return nameScore(name, b) - nameScore(name, a);
            }
        });
        for (Candidate c : candidates) {
            Mod same = findSame(c);
            if (same != null) {
                synchronized (this) {
                    foundRequested.add(same.title);
                }
                return;
            }
            Modrinth.FileChoice file = file(c);
            if (addMod(c, file, GROUP_REQUESTED, false)) {
                synchronized (this) {
                    mods.get(mods.size() - 1).requested = true;
                    foundRequested.add(c.title);
                }
                return;
            }
        }
        synchronized (this) {
            missingRequested.add(name);
            // maybe it was not a mod name but a theme ("vampires"), so it is searched like one
            plan.themes.add(new Plan.Theme(name, java.util.Arrays.asList(name), 2, new ArrayList<String>()));
        }
        log("Не нашёл мод «" + name + "» для Minecraft " + plan.minecraftVersion + " с " + Plan.loaderName(plan.loader) + ". Поищу похожие.");
    }

    private void resolveCurated() throws Exception {
        checkCancelled();
        log("Добавляю моды на оптимизацию и удобство...");
        List<Future<?>> jobs = new ArrayList<Future<?>>();
        for (final Plan.CuratedMod curated : plan.curated) {
            jobs.add(pool.submit(new java.util.concurrent.Callable<Void>() {
                public Void call() throws Exception {
                    for (String slug : curated.alternatives) {
                        Candidate c = project(slug, curated.kind);
                        if (c == null) {
                            continue;
                        }
                        if (findSame(c) != null) {
                            return null;
                        }
                        Modrinth.FileChoice file = Mod.SHADER.equals(curated.kind)
                                                       ? Modrinth.bestFile(c.id, "[\"iris\",\"optifine\"]", plan.minecraftVersion)
                                                       : file(c);
                        if (file.valid) {
                            addMod(c, file, curated.group, false, curated.kind);
                            return null;
                        }
                    }
                    return null;
                }
            }));
        }
        waitAll(jobs);
    }

    /** Word beginnings a mod of the theme must mention: "zombie" -> "zomb", so "zombies" matches too. */
    static Pattern themePattern(Plan.Theme theme) {
        List<String> stems = new ArrayList<String>();
        List<String> words = theme.matchWords.isEmpty() ? theme.queries : theme.matchWords;
        for (String phrase : words) {
            for (String word : phrase.toLowerCase(Locale.ROOT).split(" ")) {
                String stem = word.length() > 5 ? word.substring(0, word.length() - 2) : word;
                if (stem.length() >= 3 && !stems.contains(Pattern.quote(stem))) {
                    stems.add(Pattern.quote(stem));
                }
            }
        }
        if (stems.isEmpty()) {
            return Pattern.compile(".*");
        }
        // only at the start of a word: "pet" must not match "carpet"
        return Pattern.compile("(?<![a-z])(?:" + Util.join(stems, "|") + ")", Pattern.CASE_INSENSITIVE);
    }

    /** 2: the name is about the theme, 1: only the description, 0: unrelated. */
    static int themeScore(Pattern pattern, Candidate c) {
        if (pattern.matcher(c.title).find() || pattern.matcher(c.slug.replace('-', ' ')).find())
            return 2;
        if (pattern.matcher(c.description).find())
            return 1;
        return 0;
    }

    private void searchThemes() throws Exception {
        themeTaken = new int[plan.themes.size()];
        if (plan.themes.isEmpty()) {
            return;
        }
        List<Integer> all = new ArrayList<Integer>();
        for (int i = 0; i < plan.themes.size(); i++) {
            all.add(i);
            log("Ищу моды для «" + plan.themes.get(i).title + "»...");
        }
        runThemeSearch(all, false);

        List<Integer> lacking = new ArrayList<Integer>();
        List<String> titles = new ArrayList<String>();
        for (int i = 0; i < plan.themes.size(); i++) {
            if (themeTaken[i] < plan.themes.get(i).count) {
                lacking.add(i);
                titles.add(plan.themes.get(i).title);
            }
        }
        if (!lacking.isEmpty()) {
            log("Мало модов для: " + Util.join(titles, ", ") + ". Ищу шире...");
            runThemeSearch(lacking, true);
        }

        Plan.Theme firstEmpty = null;
        for (int i = 0; i < plan.themes.size(); i++) {
            // a mod name that was not found is already reported as missing
            if (themeTaken[i] == 0 && !missingRequested.contains(plan.themes.get(i).title)) {
                log("Ничего подходящего для «" + plan.themes.get(i).title + "» не нашлось.");
                emptyThemes.add(plan.themes.get(i).title);
                if (firstEmpty == null) {
                    firstEmpty = plan.themes.get(i);
                }
            }
        }
        if (firstEmpty != null) {
            suggestSetup(firstEmpty);
        }
    }

    private void runThemeSearch(List<Integer> themes, final boolean wide) throws Exception {
        checkCancelled();
        List<Future<?>> jobs = new ArrayList<Future<?>>();
        for (final int index : themes) {
            jobs.add(pool.submit(new java.util.concurrent.Callable<Void>() {
                public Void call() throws Exception {
                    searchTheme(index, wide);
                    return null;
                }
            }));
        }
        waitAll(jobs);
    }

    private void searchTheme(int index, boolean wide) throws Exception {
        Plan.Theme theme = plan.themes.get(index);
        List<String> queries = new ArrayList<String>(theme.queries);
        if (wide) {
            // the other words of the theme ("undead", "infection") find mods the first searches missed
            for (String word : theme.matchWords) {
                if (queries.size() < 6 && !queries.contains(word)) {
                    queries.add(word);
                }
            }
        }
        int limit = wide ? 50 : SEARCH_LIMIT;
        int minimumDownloads = wide ? 100 : MINIMUM_DOWNLOADS;
        Pattern pattern = themePattern(theme);

        // interleave the result lists so every search contributes its best matches; mods named after the theme first
        List<List<Candidate>> lists = new ArrayList<List<Candidate>>();
        final Map<String, Integer> scores = new HashMap<String, Integer>();
        for (String query : queries) {
            checkCancelled();
            List<Candidate> list = new ArrayList<Candidate>();
            JSONArray hits = search(query, facets(), limit);
            for (int i = 0; i < hits.length(); i++) {
                JSONObject hit = hits.getJSONObject(i);
                JSONArray categories = hit.optJSONArray("categories");
                boolean library = categories != null && categories.toString().contains("\"library\"");
                boolean serverOnly = "unsupported".equals(hit.optString("client_side"));
                if (library || serverOnly || hit.optLong("downloads") < minimumDownloads) {
                    continue;
                }
                Candidate c = Candidate.from(hit);
                int score = themeScore(pattern, c);
                if (score > 0) {
                    list.add(c);
                    scores.put(c.id, score);
                }
            }
            lists.add(list);
        }
        List<Candidate> merged = new ArrayList<Candidate>();
        Set<String> seen = new HashSet<String>();
        for (int rank = 0; rank < limit; rank++) {
            for (List<Candidate> list : lists) {
                if (rank < list.size() && seen.add(list.get(rank).id)) {
                    merged.add(list.get(rank));
                }
            }
        }
        Collections.sort(merged, new Comparator<Candidate>() {
            public int compare(Candidate a, Candidate b) {
                return scores.get(b.id) - scores.get(a.id);
            }
        });
        for (Candidate c : merged) {
            if (themeTaken[index] >= theme.count) {
                break;
            }
            checkCancelled();
            if (tried.contains(c.id) || findSame(c) != null) {
                continue;
            }
            if (addMod(c, file(c), theme.title, false)) {
                themeTaken[index]++;
            }
        }
    }

    /** Looks at every version and loader: where are the most mods for this theme? */
    private void suggestSetup(Plan.Theme theme) throws Exception {
        Pattern pattern = themePattern(theme);
        JSONArray hits = search(theme.queries.get(0), "[[\"project_type:mod\"]]", 100);
        Pattern release = Pattern.compile("^1\\.\\d+(\\.\\d+)?$");
        Map<String, Integer> counts = new HashMap<String, Integer>();
        for (int i = 0; i < hits.length(); i++) {
            JSONObject hit = hits.getJSONObject(i);
            JSONArray categories = hit.optJSONArray("categories");
            String cats = categories != null ? categories.toString() : "";
            if (cats.contains("\"library\"") || hit.optLong("downloads") < MINIMUM_DOWNLOADS || themeScore(pattern, Candidate.from(hit)) == 0) {
                continue;
            }
            JSONArray versions = hit.optJSONArray("versions");
            for (String loader : new String[] { "neoforge", "forge", "fabric", "quilt" }) {
                if (!cats.contains("\"" + loader + "\"")) {
                    continue;
                }
                for (int v = 0; versions != null && v < versions.length(); v++) {
                    if (release.matcher(versions.optString(v)).matches()) {
                        String key = loader + "|" + versions.optString(v);
                        counts.put(key, counts.containsKey(key) ? counts.get(key) + 1 : 1);
                    }
                }
            }
        }
        String current = plan.loader + "|" + plan.minecraftVersion;
        String best = null;
        for (Map.Entry<String, Integer> entry : counts.entrySet()) {
            if (entry.getKey().equals(current)) {
                continue;
            }
            int bestCount = best == null ? 0 : counts.get(best);
            boolean sameLoader = entry.getKey().startsWith(plan.loader + "|");
            if (best == null || entry.getValue() > bestCount
                || (entry.getValue() == bestCount && sameLoader && !best.startsWith(plan.loader + "|"))) {
                best = entry.getKey();
            }
        }
        if (best != null && counts.get(best) >= 2) {
            String[] parts = best.split("\\|");
            suggestedSetup = Plan.loaderName(parts[0]) + " " + parts[1];
            suggestedSetupMods = counts.get(best);
        }
    }

    private Set<String> presentIds() {
        Set<String> present = new HashSet<String>();
        synchronized (this) {
            for (Mod mod : mods) {
                present.add("mr:" + mod.projectId);
            }
        }
        return present;
    }

    private void resolveDependencies() throws Exception {
        Set<String> triedDependencies = new HashSet<String>();
        for (int round = 0; round < MAX_DEPENDENCY_ROUNDS; round++) {
            checkCancelled();
            Set<String> present = presentIds();
            final List<String> missing = new ArrayList<String>();
            synchronized (this) {
                for (Mod mod : mods) {
                    for (String required : mod.required) {
                        if (!present.contains(required) && !triedDependencies.contains(required) && !missing.contains(required)) {
                            missing.add(required);
                        }
                    }
                }
            }
            if (missing.isEmpty()) {
                return;
            }
            log("Добавляю библиотеки, нужные модам...");
            triedDependencies.addAll(missing);
            List<Future<?>> jobs = new ArrayList<Future<?>>();
            for (final String requiredId : missing) {
                jobs.add(pool.submit(new java.util.concurrent.Callable<Void>() {
                    public Void call() throws Exception {
                        addLibrary(requiredId);
                        return null;
                    }
                }));
            }
            waitAll(jobs);
        }
    }

    private void addLibrary(String requiredId) throws Exception {
        String projectId;
        if (requiredId.startsWith("mrv:")) {
            // only the version is known: find out which project it belongs to
            JSONObject version = getObject(MODRINTH + "/version/" + requiredId.substring(4));
            if (version == null || version.optString("project_id").isEmpty()) {
                return;
            }
            projectId = version.optString("project_id");
            synchronized (this) {
                for (Mod mod : mods) {
                    for (int i = 0; i < mod.required.size(); i++) {
                        if (mod.required.get(i).equals(requiredId)) {
                            mod.required.set(i, "mr:" + projectId);
                        }
                    }
                }
            }
            if (presentIds().contains("mr:" + projectId)) {
                return;
            }
        } else {
            projectId = requiredId.substring(3);
        }
        Candidate c = project(projectId);
        if (c == null) {
            log("Библиотека, нужная модам (" + projectId + "), не найдена.");
            return;
        }
        libraryNames.put("mr:" + c.id, c.title);
        if (findSame(c) != null) {
            return;
        }
        Modrinth.FileChoice file = file(c);
        if (!addMod(c, file, GROUP_LIBRARIES, true) && findSame(c) == null) {
            log("Библиотека " + c.title + " недоступна: нет файла для этой версии и загрузчика.");
        }
    }

    private void finish() {
        // a mod whose required library could not be found would crash the game, so leave it out
        boolean changed = true;
        while (changed) {
            changed = false;
            Set<String> present = presentIds();
            for (int i = 0; i < mods.size() && !changed; i++) {
                Mod mod = mods.get(i);
                for (String required : mod.required) {
                    if (!present.contains(required)) {
                        String library = libraryNames.get(required);
                        log(library == null ? "Убрал " + mod.title + ": нужной ему библиотеки нет для этой версии."
                                            : "Убрал " + mod.title + ": ему нужна " + library + ", а её нет для этой версии.");
                        if (mod.requested) {
                            foundRequested.remove(mod.title);
                            missingRequested.add(mod.title);
                        }
                        mods.remove(i);
                        changed = true;
                        break;
                    }
                }
            }
        }
        for (Mod library : mods) {
            if (!library.dependency) {
                continue;
            }
            library.neededBy.clear();
            for (Mod mod : mods) {
                if (mod.required.contains("mr:" + library.projectId) && !library.neededBy.contains(mod.title)) {
                    library.neededBy.add(mod.title);
                }
            }
        }
        // what the pack is about comes first, then optimization and comfort, libraries last
        final List<String> order = new ArrayList<String>();
        order.add(GROUP_REQUESTED);
        for (Plan.Theme theme : plan.themes) {
            if (!order.contains(theme.title))
                order.add(theme.title);
        }
        for (Plan.CuratedMod curated : plan.curated) {
            if (!order.contains(curated.group))
                order.add(curated.group);
        }
        Collections.sort(mods, new Comparator<Mod>() {
            public int compare(Mod a, Mod b) {
                int ia = a.dependency ? order.size() : order.indexOf(a.group);
                int ib = b.dependency ? order.size() : order.indexOf(b.group);
                return ia - ib;
            }
        });
        log("Готово: выбрано модов: " + mods.size() + ".");
    }

    private static void waitAll(List<Future<?>> jobs) throws Exception {
        for (Future<?> job : jobs) {
            job.get();
        }
    }

    /* ------------------------------------------------------------ export */

    static String indexKey(String loader) {
        if ("fabric".equals(loader))
            return "fabric-loader";
        if ("quilt".equals(loader))
            return "quilt-loader";
        return loader;
    }

    /** modrinth.index.json for the enabled mods; libraries only while an enabled mod needs them. */
    /** the files that go into the game: everything switched on, and the libraries those need */
    static List<Mod> included(List<Mod> mods) {
        Set<Integer> included = includedIndexes(mods);
        List<Mod> out = new ArrayList<Mod>();
        for (int i = 0; i < mods.size(); i++) {
            if (included.contains(i)) {
                out.add(mods.get(i));
            }
        }
        return out;
    }

    private static Set<Integer> includedIndexes(List<Mod> mods) {
        Set<Integer> included = new HashSet<Integer>();
        List<Integer> queue = new ArrayList<Integer>();
        for (int i = 0; i < mods.size(); i++) {
            if (mods.get(i).enabled && !mods.get(i).dependency) {
                included.add(i);
                queue.add(i);
            }
        }
        while (!queue.isEmpty()) {
            Mod mod = mods.get(queue.remove(0));
            for (String required : mod.required) {
                for (int i = 0; i < mods.size(); i++) {
                    if (!included.contains(i) && required.equals("mr:" + mods.get(i).projectId)) {
                        included.add(i);
                        queue.add(i);
                    }
                }
            }
        }
        return included;
    }

    static String makeIndex(Plan plan, String loaderVersion, List<Mod> mods) throws JSONException {
        JSONArray files = new JSONArray();
        for (Mod mod : included(mods)) {
            if (CurseForge.isCurseForge(mod.projectId)) {
                continue;  // .mrpack files may only link to Modrinth and GitHub: CurseForge files travel inside the pack (overrides)
            }
            JSONObject hashes = new JSONObject();
            if (!mod.sha1.isEmpty()) {
                hashes.put("sha1", mod.sha1);
            }
            if (!mod.sha512.isEmpty()) {
                hashes.put("sha512", mod.sha512);
            }
            JSONObject env = new JSONObject();
            env.put("client", "required");
            env.put("server", "optional");
            JSONObject file = new JSONObject();
            file.put("path", mod.targetPath());
            file.put("hashes", hashes);
            file.put("env", env);
            file.put("downloads", new JSONArray().put(mod.url));
            file.put("fileSize", mod.size);
            files.put(file);
        }
        JSONObject dependencies = new JSONObject();
        dependencies.put("minecraft", plan.minecraftVersion);
        dependencies.put(indexKey(plan.loader), loaderVersion);
        JSONObject index = new JSONObject();
        index.put("formatVersion", 1);
        index.put("game", "minecraft");
        index.put("versionId", "1.0.0");
        index.put("name", plan.name);
        index.put("summary", plan.summary);
        index.put("files", files);
        index.put("dependencies", dependencies);
        return index.toString(2);
    }
}
