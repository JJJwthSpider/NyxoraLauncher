package com.sculkshoot.client;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Understands requests like "сборка с зомби, добавь Create, без JEI, neoforge 1.21.1" without any AI service.
 * The same rules as the desktop Moony (launcher/modplatform/agent/ModpackPlan.cpp).
 */
final class Parser {
    private Parser() {}

    private static final String WORD = "[\\p{L}\\p{N}_]";

    /** classic versions (1.7.10, 1.21.1) and the year based ones Mojang uses since 2026 (26.1, 26.1.2) */
    static final Pattern VERSION = Pattern.compile("(?<![\\d.])(1\\.\\d{1,2}(?:\\.\\d{1,2})?|(?:2[5-9]|[3-9]\\d)\\.\\d{1,2}(?:\\.\\d{1,2})?)(?![\\d.])");
    private static final Pattern SEPARATORS = Pattern.compile("[^\\p{L}\\p{N}_.\\-]+");
    private static final Pattern WORD_SEPARATORS = Pattern.compile("[^\\p{L}\\p{N}_\\-]+");
    private static final Pattern CYRILLIC = Pattern.compile("[\\u0400-\\u04FF]");
    private static final Pattern SEGMENTS = Pattern.compile("[,;!?\\n]+|\\.(?!\\d)");
    // group 1: words that leave something out, group 2: words that ask for something
    private static final Pattern MARKERS = Pattern.compile(
        "(?<![\\p{L}\\p{N}_'])(?:(убери|уберите|убрать|удали|удалите|удалить|выкинь|выкини|выбрось|исключи|без|"
            + "не\\s+надо|не\\s+нужн" + WORD + "*|не\\s+хочу|не\\s+добавляй|не\\s+ставь|не\\s+клади|remove|without|exclude|"
            + "drop|don'?t\\s+(?:add|want|need|include))|"
            + "(но|а|зато|добавь|добавьте|добавить|поставь|закинь|вставь|верни|хочу|ещё|еще|плюс|but|add|include|also|plus))"
            + "(?![\\p{L}\\p{N}_'])",
        Pattern.CASE_INSENSITIVE | Pattern.UNICODE_CASE);
    private static final Pattern MORE = Pattern.compile("(?<![\\p{L}\\p{N}_])(много|побольше|больше|кучу|куча|more|lots|plenty)(?![\\p{L}\\p{N}_])",
                                                        Pattern.CASE_INSENSITIVE | Pattern.UNICODE_CASE);
    private static final Pattern QUOTED = Pattern.compile("[«\"“„]([^»\"”“]{2,60})[»\"”“]");
    private static final Pattern LATIN_RUN = Pattern.compile("[A-Za-z0-9][A-Za-z0-9'’&+:.\\-]*(?:[ \\t]+[A-Za-z0-9][A-Za-z0-9'’&+:.\\-]*)*");
    private static final Pattern JOINERS = Pattern.compile("\\s+(?:and|&|\\+|with|или|и)\\s+", Pattern.CASE_INSENSITIVE | Pattern.UNICODE_CASE);
    private static final Pattern CAPITALIZED = Pattern.compile("(?<![\\p{L}\\p{N}_'])[A-Z0-9][\\p{L}\\p{N}_'’&+:.\\-]*(?:[ \\t]+(?:of|the|n|'n')?[ \\t]*[A-Z0-9][\\p{L}\\p{N}_'’&+:.\\-]*)*");

    private static final Set<String> FILLERS = new HashSet<String>(Arrays.asList(Rules.FILLERS));
    private static final List<String> SETUP_WORDS = Arrays.asList("neoforge", "forge", "fabric", "quilt", "neo", "minecraft", "mc", "mod", "mods",
                                                                  "modpack", "pack", "the", "a", "an", "and", "with", "add", "plus", "please",
                                                                  "also", "some", "more", "lots", "me", "my", "i", "to", "of", "in", "for",
                                                                  "version", "loader");
    private static List<String> knownWords;

    static final class Clause {
        final String text;
        final boolean negative;

        Clause(String text, boolean negative) {
            this.text = text;
            this.negative = negative;
        }
    }

    /* ------------------------------------------------------------ words */

    static boolean hasCyrillic(String text) {
        return CYRILLIC.matcher(text).find();
    }

    static String translateWord(String word) {
        String best = null;
        int bestLength = 0;
        for (String[] entry : Rules.DICTIONARY) {
            String stem = entry[0];
            // very short stems ("бой", "лук") must match nearly the whole word
            if (stem.length() <= 3 && word.length() > stem.length() + 2) {
                continue;
            }
            if (word.startsWith(stem) && stem.length() > bestLength) {
                best = entry[1];
                bestLength = stem.length();
            }
        }
        return best != null ? best : word;
    }

    /** English keywords of a request: Russian words translated, filler words removed. */
    static List<String> keywords(String query) {
        List<String> result = new ArrayList<String>();
        List<String> unknown = new ArrayList<String>();
        for (String word : SEPARATORS.split(query.toLowerCase(Locale.ROOT))) {
            if (word.isEmpty() || FILLERS.contains(word)) {
                continue;
            }
            String translated = hasCyrillic(word) ? translateWord(word) : word;
            if (hasCyrillic(translated)) {
                unknown.add(translated);
            } else if (!result.contains(translated)) {
                result.add(translated);
            }
        }
        // Russian words without a translation would only spoil an English search
        return result.isEmpty() ? unknown : result;
    }

    /** Russian words the dictionary does not know (fillers left out). */
    static List<String> unknownWords(String text) {
        List<String> result = new ArrayList<String>();
        for (String word : WORD_SEPARATORS.split(text.toLowerCase(Locale.ROOT))) {
            if (word.length() < 4 || !hasCyrillic(word) || FILLERS.contains(word) || result.contains(word)) {
                continue;
            }
            if (hasCyrillic(translateWord(word))) {
                result.add(word);
            }
        }
        return result;
    }

    static List<String> knownWords() {
        if (knownWords == null) {
            List<String> known = new ArrayList<String>(Arrays.asList("neoforge", "forge", "fabric", "quilt", "optimization", "performance", "fps",
                                                                     "lag", "shaders", "graphics", "map", "minimap", "textures"));
            for (Rules.ThemeRule rule : Rules.THEMES) {
                known.addAll(Arrays.asList(rule.triggers));
            }
            knownWords = known;
        }
        return knownWords;
    }

    static boolean isSetupWord(String word) {
        String lower = word.toLowerCase(Locale.ROOT);
        if (SETUP_WORDS.contains(lower) || VERSION.matcher(lower).find()) {
            return true;
        }
        try {
            return Integer.parseInt(lower) > 0;
        } catch (NumberFormatException e) {
            return false;
        }
    }

    static String normalized(String text) {
        StringBuilder out = new StringBuilder();
        for (char c : text.toLowerCase(Locale.ROOT).toCharArray()) {
            if (Character.isLetterOrDigit(c)) {
                out.append(c);
            }
        }
        return out.toString();
    }

    private static boolean containsIgnoreCase(List<String> list, String text) {
        for (String item : list) {
            if (item.equalsIgnoreCase(text)) {
                return true;
            }
        }
        return false;
    }

    private static void addText(List<String> list, String text) {
        if (!containsIgnoreCase(list, text)) {
            list.add(text);
        }
    }

    private static void removeText(List<String> list, String text) {
        for (int i = list.size() - 1; i >= 0; i--) {
            if (list.get(i).equalsIgnoreCase(text)) {
                list.remove(i);
            }
        }
    }

    /* ------------------------------------------------------------ clauses */

    static List<Clause> splitClauses(String text) {
        List<Clause> clauses = new ArrayList<Clause>();
        for (String segment : SEGMENTS.split(text)) {
            int start = 0;
            boolean negative = false;
            Matcher m = MARKERS.matcher(segment);
            while (m.find()) {
                String piece = segment.substring(start, m.start()).trim();
                if (!piece.isEmpty()) {
                    clauses.add(new Clause(piece, negative));
                }
                negative = m.group(1) != null;
                start = m.end();
            }
            String piece = segment.substring(start).trim();
            if (!piece.isEmpty()) {
                clauses.add(new Clause(piece, negative));
            }
        }
        return clauses;
    }

    static String positiveText(String request) {
        StringBuilder out = new StringBuilder();
        for (Clause clause : splitClauses(request)) {
            if (!clause.negative) {
                if (out.length() > 0) {
                    out.append(", ");
                }
                out.append(clause.text);
            }
        }
        return out.toString();
    }

    /** Mod names: anything in quotes, Latin words inside Russian text, capitalized words in English text. */
    static List<String> modNames(String clause) {
        List<String> candidates = new ArrayList<String>();
        Matcher quoted = QUOTED.matcher(clause);
        while (quoted.find()) {
            candidates.add(quoted.group(1).trim());
        }
        clause = QUOTED.matcher(clause).replaceAll(" ");
        if (hasCyrillic(clause)) {
            Matcher run = LATIN_RUN.matcher(clause);
            while (run.find()) {
                candidates.addAll(Arrays.asList(JOINERS.split(run.group())));
            }
        } else {
            Matcher capitalized = CAPITALIZED.matcher(clause);
            while (capitalized.find()) {
                candidates.add(capitalized.group());
            }
        }

        List<String> names = new ArrayList<String>();
        List<String> known = knownWords();
        for (String candidate : candidates) {
            List<String> tokens = new ArrayList<String>(Arrays.asList(candidate.trim().split("\\s+")));
            while (!tokens.isEmpty() && (tokens.get(0).isEmpty() || isSetupWord(tokens.get(0)))) {
                tokens.remove(0);
            }
            while (!tokens.isEmpty() && isSetupWord(tokens.get(tokens.size() - 1))) {
                tokens.remove(tokens.size() - 1);
            }
            boolean meaningful = false;
            for (String token : tokens) {
                String lower = token.toLowerCase(Locale.ROOT);
                if (!isSetupWord(lower) && !known.contains(lower) && !known.contains(lower + "s")) {
                    meaningful = true;
                }
            }
            String name = Util.join(tokens, " ");
            if (meaningful && name.length() >= 2 && !containsIgnoreCase(names, name)) {
                names.add(name);
            }
        }
        return names;
    }

    /* ------------------------------------------------------------ plan */

    static boolean wantsShaders(String request) {
        List<String> words = keywords(request);
        return words.contains("shaders") || words.contains("shader") || words.contains("graphics") || words.contains("iris");
    }

    static boolean wantsMap(String request) {
        List<String> words = keywords(request);
        return words.contains("map") || words.contains("minimap") || words.contains("maps") || words.contains("waypoints");
    }

    static boolean wantsOnlyPerformance(String request) {
        List<String> words = keywords(request);
        return words.contains("optimization") || words.contains("performance") || words.contains("fps") || words.contains("lag");
    }

    static Plan parse(String request) {
        Plan plan = new Plan();
        String lower = request.toLowerCase(Locale.ROOT);

        Matcher version = VERSION.matcher(lower);
        if (version.find()) {
            plan.minecraftVersion = version.group(1);
            plan.versionGiven = true;
        } else {
            plan.minecraftVersion = Plan.DEFAULT_VERSION;
        }

        List<String> words = keywords(request);
        if (words.contains("neoforge") || lower.contains("neo forge") || lower.contains("нео фордж")) {
            plan.loader = "neoforge";
        } else if (words.contains("forge")) {
            plan.loader = "forge";
        } else if (words.contains("fabric")) {
            plan.loader = "fabric";
        } else if (words.contains("quilt")) {
            plan.loader = "quilt";
        }
        plan.loaderGiven = plan.loader != null;
        if (!plan.loaderGiven) {
            plan.loader = Plan.defaultLoader(plan.minecraftVersion);
        }

        // the conversation is read clause by clause and in order: later wishes change earlier ones
        List<String> known = knownWords();
        List<String> wanted = new ArrayList<String>();
        Set<String> boosted = new HashSet<String>();
        for (Clause clause : splitClauses(request)) {
            List<String> names = modNames(clause.text);
            List<String> nameWords = new ArrayList<String>();
            for (String name : names) {
                nameWords.addAll(keywords(name));
            }
            List<String> clauseWords = new ArrayList<String>();
            for (String word : keywords(clause.text)) {
                if (word.length() >= 3 && !hasCyrillic(word) && !isSetupWord(word) && !nameWords.contains(word)) {
                    clauseWords.add(word);
                }
            }
            if (clause.negative) {
                for (String name : names) {
                    removeText(plan.requestedMods, name);
                    addText(plan.excluded, name);
                }
                for (String word : clauseWords) {
                    wanted.remove(word);
                    addText(plan.excluded, word);
                }
                continue;
            }
            for (String name : names) {
                removeText(plan.excluded, name);
                addText(plan.requestedMods, name);
            }
            boolean more = MORE.matcher(clause.text).find();
            for (String word : clauseWords) {
                removeText(plan.excluded, word);
                if (!wanted.contains(word)) {
                    wanted.add(word);
                }
                if (more) {
                    boosted.add(word);
                }
            }
            for (String word : unknownWords(clause.text)) {
                addText(plan.unknownWords, word);
            }
        }

        for (Rules.ThemeRule rule : Rules.THEMES) {
            boolean hit = false;
            boolean more = false;
            boolean unwanted = false;
            for (String trigger : rule.triggers) {
                if (wanted.contains(trigger)) {
                    hit = true;
                    more = more || boosted.contains(trigger);
                }
                unwanted = unwanted || containsIgnoreCase(plan.excluded, trigger);
            }
            if (hit && !unwanted) {
                List<String> match = new ArrayList<String>(Arrays.asList(rule.triggers));
                match.addAll(Arrays.asList(rule.queries));
                plan.themes.add(new Plan.Theme(rule.title, Arrays.asList(rule.queries), more ? Math.min(rule.count * 2, 8) : rule.count, match));
            }
        }
        // words that matched no theme are searched for as they are
        for (String word : wanted) {
            if (known.contains(word) || VERSION.matcher(word).find()) {
                continue;
            }
            plan.themes.add(new Plan.Theme(word, Arrays.asList(word), boosted.contains(word) ? 4 : 2, new ArrayList<String>()));
        }

        String wishes = positiveText(request);
        plan.curated = Plan.essentialMods(plan.loader, wantsShaders(wishes), wantsMap(wishes));
        plan.describe();
        return plan;
    }

    /** The plan for a topic gathered in a chat, with the loader and version chosen separately (null: from the topic). */
    static Plan planFor(String topic, String loader, String minecraftVersion) {
        Plan plan = parse(topic);
        if (minecraftVersion != null && !minecraftVersion.isEmpty()) {
            plan.minecraftVersion = minecraftVersion;
            plan.versionGiven = true;
        }
        if (loader != null && !loader.isEmpty()) {
            plan.loader = loader;
            plan.loaderGiven = true;
        }
        String wishes = positiveText(topic);
        plan.curated = Plan.essentialMods(plan.loader, wantsShaders(wishes), wantsMap(wishes));
        plan.describe();
        return plan;
    }
}
