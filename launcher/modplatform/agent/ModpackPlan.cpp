// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "ModpackPlan.h"

#include <QCoreApplication>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>

#include "SearchTranslator.h"
#include "Version.h"

namespace ModpackAgent {

namespace {

struct ThemeRule {
    const char* title;     // translated through QCoreApplication::translate("ModpackAgent", ...)
    QStringList triggers;  // English keywords (Russian requests are translated first)
    QStringList queries;   // what to search on Modrinth
    int count;
};

const QList<ThemeRule>& themeRules()
{
    static const QList<ThemeRule> s_rules = {
        { QT_TRANSLATE_NOOP("ModpackAgent", "Zombies"),
          { "zombie", "zombies", "undead", "apocalypse", "infection", "post-apocalyptic" },
          { "zombie", "zombie apocalypse", "zombies", "undead", "infection" },
          4 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Weapons"),
          { "weapons", "weapon", "guns", "gun", "rifle", "pistol", "swords", "sword", "bow", "crossbow", "firearms" },
          { "guns", "weapons", "swords" },
          4 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Armor"), { "armor", "armour", "shield" }, { "armor" }, 2 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Combat"), { "combat", "pvp", "battle", "fight" }, { "combat" }, 2 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Mobs and creatures"),
          { "mobs", "mob", "monsters", "monster", "creatures", "skeleton", "spider", "vampire", "werewolf" },
          { "mobs", "creatures" },
          3 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Animals and pets"), { "animals", "animal", "pets", "pet" }, { "animals", "pets" }, 3 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Dragons"), { "dragons", "dragon" }, { "dragons" }, 2 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Bosses"), { "bosses", "boss" }, { "bosses" }, 2 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Magic"),
          { "magic", "spells", "spell", "wizard", "enchantments", "potions", "alchemy", "fantasy" },
          { "magic", "spells" },
          3 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Technology"),
          { "technology", "tech", "mechanical", "automation", "electricity", "energy", "factory", "industrial", "pipes" },
          { "technology", "automation" },
          3 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Space"), { "space", "rockets", "rocket", "planets" }, { "space", "rocket" }, 2 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Adventure and dungeons"),
          { "adventure", "dungeons", "dungeon", "quests", "rpg", "pirates", "knights", "vikings", "medieval" },
          { "dungeons", "adventure" },
          4 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Structures"),
          { "structures", "buildings", "villages", "city", "cities", "town", "towns", "castles" },
          { "structures", "villages" },
          3 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "World generation"),
          { "biomes", "world", "generation", "terrain", "caves", "ocean", "forest", "mountains" },
          { "biomes", "terrain generation" },
          2 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Horror"), { "horror", "scary" }, { "horror" }, 3 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Survival and realism"),
          { "survival", "realism", "realistic", "hardcore", "radiation", "weather", "seasons" },
          { "survival", "realism" },
          3 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Food and farming"),
          { "food", "cooking", "farming", "crops", "plants" },
          { "food", "farming" },
          3 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Decoration and building"),
          { "decoration", "furniture", "building", "blocks" },
          { "furniture", "decoration" },
          3 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Vehicles"),
          { "vehicles", "vehicle", "cars", "car", "planes", "trains", "boats", "ships" },
          { "vehicles" },
          2 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Storage"), { "storage", "chests", "backpacks", "inventory" }, { "storage", "backpacks" }, 2 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Mining and tools"), { "mining", "ores", "tools", "pickaxe" }, { "mining", "tools" }, 2 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Skills and levels"), { "skills", "levels", "classes" }, { "skills", "leveling" }, 2 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Pokemon"), { "pokemon", "cobblemon", "pixelmon" }, { "cobblemon", "pokemon" }, 2 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Dinosaurs"), { "dinosaurs", "dinosaur", "jurassic", "prehistoric" }, { "dinosaurs", "prehistoric" }, 2 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Fishing"), { "fishing", "fish" }, { "fishing" }, 2 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Skyblock"), { "skyblock", "islands", "oneblock" }, { "skyblock", "floating islands" }, 2 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Anime"), { "anime", "naruto", "ninja" }, { "anime" }, 2 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Military"), { "military", "army", "tanks", "tank", "war", "soldiers" }, { "military", "tanks" }, 3 },
        { QT_TRANSLATE_NOOP("ModpackAgent", "Atmosphere"),
          { "sounds", "music", "animations", "physics", "lighting", "blood" },
          { "immersive", "animations" },
          2 },
    };
    return s_rules;
}

QString tr(const char* text)
{
    return QCoreApplication::translate("ModpackAgent", text);
}

// classic versions (1.7.10, 1.12.2, 1.21.1) and the year based ones Mojang uses since 2026 (26.1, 26.1.2)
const QRegularExpression& versionPattern()
{
    static const QRegularExpression s_version(
        QStringLiteral("(?<![\\d.])(1\\.\\d{1,2}(?:\\.\\d{1,2})?|(?:2[5-9]|[3-9]\\d)\\.\\d{1,2}(?:\\.\\d{1,2})?)(?![\\d.])"));
    return s_version;
}

/// English words that already mean something to the agent (themes, loaders, performance, shaders, maps)
const QStringList& knownWords()
{
    static const QStringList s_known = [] {
        QStringList known{ "neoforge", "forge",   "fabric",   "quilt", "optimization", "performance", "fps",
                           "lag",      "shaders", "graphics", "map",   "minimap",      "textures" };
        for (const auto& rule : themeRules()) {
            known << rule.triggers;
        }
        return known;
    }();
    return s_known;
}

/// Loader names, versions and words that are never a theme or a mod name
bool isSetupWord(const QString& word)
{
    static const QStringList s_setup{ "neoforge", "forge", "fabric", "quilt",  "neo", "minecraft", "mc",  "mod",  "mods",   "modpack",
                                      "pack",     "the",   "a",      "an",     "and", "with",      "add", "plus", "please", "also",
                                      "some",     "more",  "lots",   "me",     "my",  "i",         "to",  "of",   "in",     "for",
                                      "version",  "loader" };
    const auto lower = word.toLower();
    return s_setup.contains(lower) || versionPattern().match(lower).hasMatch() || lower.toInt() > 0;
}

/// "Just Enough Items" and "just-enough-items" both become "justenoughitems"
QString normalized(const QString& text)
{
    QString out;
    for (const auto& c : text.toLower()) {
        if (c.isLetterOrNumber()) {
            out += c;
        }
    }
    return out;
}

void addText(QStringList& list, const QString& text)
{
    if (!list.contains(text, Qt::CaseInsensitive)) {
        list << text;
    }
}

void removeText(QStringList& list, const QString& text)
{
    list.removeIf([&text](const QString& item) { return item.compare(text, Qt::CaseInsensitive) == 0; });
}

/// Part of a request: "сборка с зомби", "без магии", "добавь Create".
struct Clause {
    QString text;
    bool negative = false;  // "убери ...", "без ...", "не надо ..."
};

QList<Clause> splitClauses(const QString& text)
{
    // a dot ends a sentence, but not inside a version like 1.21.1
    static const QRegularExpression s_segments(QStringLiteral("[,;!?\\n]+|\\.(?!\\d)"));
    // group 1: words that leave something out, group 2: words that ask for something
    static const QRegularExpression s_markers(
        QStringLiteral("(?<![\\w'])(?:(убери|уберите|убрать|удали|удалите|удалить|выкинь|выкини|выбрось|исключи|без|"
                       "не\\s+надо|не\\s+нужн\\w*|не\\s+хочу|не\\s+добавляй|не\\s+ставь|не\\s+клади|remove|without|exclude|"
                       "drop|don'?t\\s+(?:add|want|need|include))|"
                       "(но|а|зато|добавь|добавьте|добавить|поставь|закинь|вставь|верни|хочу|ещё|еще|плюс|"
                       "but|add|include|also|plus))(?![\\w'])"),
        QRegularExpression::UseUnicodePropertiesOption | QRegularExpression::CaseInsensitiveOption);
    QList<Clause> clauses;
    for (const auto& segment : text.split(s_segments, Qt::SkipEmptyParts)) {
        qsizetype start = 0;
        bool negative = false;
        auto it = s_markers.globalMatch(segment);
        while (it.hasNext()) {
            const auto match = it.next();
            if (auto piece = segment.mid(start, match.capturedStart() - start).trimmed(); !piece.isEmpty()) {
                clauses << Clause{ piece, negative };
            }
            negative = !match.captured(1).isEmpty();
            start = match.capturedEnd();
        }
        if (auto piece = segment.mid(start).trimmed(); !piece.isEmpty()) {
            clauses << Clause{ piece, negative };
        }
    }
    return clauses;
}

/// The request without the parts that ask to leave something out.
QString positiveText(const QString& request)
{
    QStringList parts;
    for (const auto& clause : splitClauses(request)) {
        if (!clause.negative) {
            parts << clause.text;
        }
    }
    return parts.join(", ");
}

const QRegularExpression& morePattern()
{
    static const QRegularExpression s_more(QStringLiteral("(?<!\\w)(много|побольше|больше|кучу|куча|more|lots|plenty)(?!\\w)"),
                                           QRegularExpression::UseUnicodePropertiesOption | QRegularExpression::CaseInsensitiveOption);
    return s_more;
}

/// Mod names in a clause: anything in quotes, Latin words inside Russian text ("сборка с Create и Farmer's Delight"),
/// capitalized words in English text ("a zombie pack with Create").
QStringList modNames(QString clause)
{
    QStringList candidates;
    static const QRegularExpression s_quoted(QStringLiteral("[«\"“„]([^»\"”“]{2,60})[»\"”“]"));
    for (auto it = s_quoted.globalMatch(clause); it.hasNext();) {
        candidates << it.next().captured(1).trimmed();
    }
    clause.remove(s_quoted);

    static const QRegularExpression s_latinRun(QStringLiteral("[A-Za-z0-9][A-Za-z0-9'’&+:.\\-]*(?:[ \\t]+[A-Za-z0-9][A-Za-z0-9'’&+:.\\-]*)*"));
    static const QRegularExpression s_joiners(QStringLiteral("\\s+(?:and|&|\\+|with|или|и)\\s+"), QRegularExpression::CaseInsensitiveOption);
    if (SearchTranslator::hasCyrillic(clause)) {
        for (auto it = s_latinRun.globalMatch(clause); it.hasNext();) {
            candidates << it.next().captured(0).split(s_joiners, Qt::SkipEmptyParts);
        }
    } else {
        // English: only capitalized words are names, the rest is the description of the pack
        static const QRegularExpression s_capitalized(
            QStringLiteral("(?<![\\w'])[A-Z0-9][\\w'’&+:.\\-]*(?:[ \\t]+(?:of|the|n|'n')?[ \\t]*[A-Z0-9][\\w'’&+:.\\-]*)*"));
        for (auto it = s_capitalized.globalMatch(clause); it.hasNext();) {
            candidates << it.next().captured(0);
        }
    }

    QStringList names;
    const auto& known = knownWords();
    for (const auto& candidate : candidates) {
        auto tokens = candidate.simplified().split(' ', Qt::SkipEmptyParts);
        while (!tokens.isEmpty() && isSetupWord(tokens.first()))
            tokens.removeFirst();
        while (!tokens.isEmpty() && isSetupWord(tokens.last()))
            tokens.removeLast();
        bool meaningful = false;
        for (const auto& token : tokens) {
            const auto lower = token.toLower();
            if (!isSetupWord(lower) && !known.contains(lower) && !known.contains(lower + 's')) {
                meaningful = true;
            }
        }
        const auto name = tokens.join(' ');
        if (meaningful && name.size() >= 2 && !names.contains(name, Qt::CaseInsensitive)) {
            names << name;
        }
    }
    return names;
}

}  // namespace

QString defaultMinecraftVersion()
{
    // the version with the biggest choice of mods for every loader
    return QStringLiteral("1.21.1");
}

QString defaultLoader(const QString& minecraftVersion)
{
    // NeoForge took over the big content mods from 1.20.2 on, before that Forge was the place to be
    return Version(minecraftVersion) >= Version("1.20.2") ? QStringLiteral("neoforge") : QStringLiteral("forge");
}

QString loaderDisplayName(const QString& loader)
{
    if (loader == "neoforge")
        return QStringLiteral("NeoForge");
    if (loader == "forge")
        return QStringLiteral("Forge");
    if (loader == "fabric")
        return QStringLiteral("Fabric");
    if (loader == "quilt")
        return QStringLiteral("Quilt");
    return loader;
}

bool requestWantsShaders(const QString& request)
{
    const auto words = SearchTranslator::keywords(request);
    return words.contains("shaders") || words.contains("shader") || words.contains("graphics") || words.contains("iris");
}

bool requestWantsMap(const QString& request)
{
    const auto words = SearchTranslator::keywords(request);
    return words.contains("map") || words.contains("minimap") || words.contains("maps") || words.contains("waypoints");
}

QList<CuratedMod> essentialMods(const QString& loader, bool wantsShaders, bool wantsMap)
{
    const auto performance = tr("Performance");
    const auto comfort = tr("Quality of life");
    const bool fabricLike = loader == "fabric" || loader == "quilt";

    QList<CuratedMod> mods;
    if (loader == "forge") {
        mods << CuratedMod{ { "embeddium", "rubidium" }, performance };
    } else {
        mods << CuratedMod{ { "sodium", "embeddium" }, performance };
    }
    if (fabricLike || loader == "neoforge") {
        mods << CuratedMod{ { "lithium" }, performance };
    }
    mods << CuratedMod{ { "ferrite-core" }, performance };
    mods << CuratedMod{ { "modernfix" }, performance };
    mods << CuratedMod{ { "entityculling" }, performance };
    mods << CuratedMod{ { "immediatelyfast" }, performance };
    mods << CuratedMod{ { "dynamic-fps" }, performance };

    mods << CuratedMod{ { "jei", "emi" }, comfort };
    mods << CuratedMod{ { "jade", "wthit" }, comfort };
    mods << CuratedMod{ { "appleskin" }, comfort };
    mods << CuratedMod{ { "mouse-tweaks" }, comfort };

    if (wantsShaders) {
        mods << CuratedMod{ loader == "forge" ? QStringList{ "oculus" } : QStringList{ "iris", "oculus" }, tr("Shaders") };
        // the shaders themselves: the most popular pack, or the other well-known ones if it has no version yet
        mods << CuratedMod{ { "complementary-reimagined", "bsl-shaders", "makeup-ultra-fast-shaders" }, tr("Shaders"), "shader" };
    }
    if (wantsMap) {
        mods << CuratedMod{ { "xaeros-minimap", "journeymap" }, tr("Map") };
        mods << CuratedMod{ { "xaeros-world-map" }, tr("Map") };
    }
    return mods;
}

Plan parseLocally(const QString& request)
{
    Plan plan;
    const auto lower = request.toLower();

    if (auto match = versionPattern().match(lower); match.hasMatch()) {
        plan.minecraftVersion = match.captured(1);
        plan.versionGiven = true;
    } else {
        plan.minecraftVersion = defaultMinecraftVersion();
    }

    const auto words = SearchTranslator::keywords(request);
    if (words.contains("neoforge") || lower.contains("neo forge") || lower.contains("нео фордж")) {
        plan.loader = "neoforge";
    } else if (words.contains("forge")) {
        plan.loader = "forge";
    } else if (words.contains("fabric")) {
        plan.loader = "fabric";
    } else if (words.contains("quilt")) {
        plan.loader = "quilt";
    }
    plan.loaderGiven = !plan.loader.isEmpty();
    if (!plan.loaderGiven) {
        plan.loader = defaultLoader(plan.minecraftVersion);
    }

    // the conversation is read clause by clause and in order, so "add zombies" after "without zombies" brings them back
    const auto& known = knownWords();
    QStringList wanted;  // English keywords of everything the user asked for
    QSet<QString> boosted;
    for (const auto& clause : splitClauses(request)) {
        const auto names = modNames(clause.text);
        QStringList nameWords;
        for (const auto& name : names) {
            nameWords << SearchTranslator::keywords(name);
        }
        QStringList clauseWords;
        for (const auto& word : SearchTranslator::keywords(clause.text)) {
            if (word.size() >= 3 && !SearchTranslator::hasCyrillic(word) && !isSetupWord(word) && !nameWords.contains(word)) {
                clauseWords << word;
            }
        }

        if (clause.negative) {
            for (const auto& name : names) {
                removeText(plan.requestedMods, name);
                addText(plan.excluded, name);
            }
            for (const auto& word : clauseWords) {
                wanted.removeAll(word);
                addText(plan.excluded, word);
            }
            continue;
        }
        for (const auto& name : names) {
            removeText(plan.excluded, name);
            addText(plan.requestedMods, name);
        }
        const bool more = morePattern().match(clause.text).hasMatch();
        for (const auto& word : clauseWords) {
            removeText(plan.excluded, word);
            if (!wanted.contains(word)) {
                wanted << word;
            }
            if (more) {
                boosted.insert(word);
            }
        }
        for (const auto& word : SearchTranslator::unknownWords(clause.text)) {
            addText(plan.unknownWords, word);
        }
    }

    for (const auto& rule : themeRules()) {
        bool hit = false;
        bool more = false;
        bool unwanted = false;
        for (const auto& trigger : rule.triggers) {
            if (wanted.contains(trigger)) {
                hit = true;
                more = more || boosted.contains(trigger);
            }
            unwanted = unwanted || plan.excluded.contains(trigger, Qt::CaseInsensitive);
        }
        if (hit && !unwanted) {
            // "побольше оружия" doubles the number of weapon mods
            plan.themes << Theme{ tr(rule.title), rule.queries, more ? std::min(rule.count * 2, 8) : rule.count,
                                  rule.triggers + rule.queries };
        }
    }

    // words that matched no theme are searched for as they are, so "сборка с witches" still finds something
    for (const auto& word : wanted) {
        if (known.contains(word) || versionPattern().match(word).hasMatch()) {
            continue;
        }
        plan.themes << Theme{ word, { word }, boosted.contains(word) ? 4 : 2 };
    }

    finishPlan(plan, request);
    return plan;
}

void finishPlan(Plan& plan, const QString& request)
{
    // what goes beyond the themes: a weak PC or phone, playing with friends, the number of mods, OptiFine
    static const QRegularExpression s_lowEnd(
        QStringLiteral("слаб\\w*\\s+(?:телефон|смартфон|пк|комп|ноут|устройств|желез|видеокарт)|для\\s+слаб|картошк|потато|"
                       "low[ -]?end|weak\\s+(?:pc|phone|laptop)|potato"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);
    static const QRegularExpression s_multiplayer(
        QStringLiteral("(?<!\\w)(?:с\\s+друз|с\\s+друг(?:ом|ой)|друзьями|мультиплеер|кооп|по\\s+сети|вдво[её]м|втро[её]м|multiplayer|co-?op|"
                       "with\\s+friends)"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);
    static const QRegularExpression s_count(QStringLiteral("(?<![\\d.])(\\d{1,3})\\s*(?:мод|mods?\\b)"),
                                            QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption);
    const auto lower = request.toLower();
    const auto wishes = positiveText(request);
    bool shaders = requestWantsShaders(wishes);
    plan.notes.clear();
    plan.lowEnd = s_lowEnd.match(lower).hasMatch();
    plan.multiplayer = s_multiplayer.match(wishes.toLower()).hasMatch();
    for (qsizetype i = plan.requestedMods.size() - 1; i >= 0; i--) {
        if (normalized(plan.requestedMods.at(i)).startsWith("optifine")) {
            plan.requestedMods.removeAt(i);
            shaders = true;
            plan.notes << tr("instead of OptiFine: Sodium and Iris, they are faster, work with other mods and show shaders too");
        }
    }
    if (plan.lowEnd) {
        plan.notes << (shaders ? tr("no shaders: they are slow on a weak computer (you can add them later)")
                               : tr("for a weak computer: only light mods and all the performance mods"));
        shaders = false;
        for (auto& theme : plan.themes) {
            theme.count = std::min(theme.count, 2);
        }
    }
    plan.curated = essentialMods(plan.loader, shaders, requestWantsMap(wishes));
    if (plan.multiplayer) {
        plan.curated << CuratedMod{ { "simple-voice-chat", "plasmo-voice" }, tr("Playing with friends") };
        plan.notes << tr("a voice chat for playing with friends: Simple Voice Chat");
    }
    plan.targetMods = 0;
    if (auto match = s_count.match(lower); match.hasMatch()) {
        const int wanted = match.captured(1).toInt();
        if (wanted >= 5 && wanted <= 300) {
            plan.targetMods = wanted;
            int themed = 0;
            for (const auto& theme : plan.themes) {
                themed += theme.count;
            }
            const int room = wanted - int(plan.curated.size()) - int(plan.requestedMods.size());
            if (themed > 0 && room > 0) {
                for (auto& theme : plan.themes) {
                    theme.count = std::clamp(int(std::lround(theme.count * double(room) / themed)), 1, 15);
                }
            }
            plan.notes << tr("about %1 mods, as asked").arg(wanted);
        }
    }
    describePlan(plan);
}

bool isExcluded(const Plan& plan, const QString& title, const QString& slug)
{
    const auto name = normalized(title);
    const auto id = normalized(slug);
    for (const auto& term : plan.excluded) {
        auto key = normalized(term);
        // "weapons" should also catch "Weapon Master"
        if (key.size() > 4 && key.endsWith('s')) {
            key.chop(1);
        }
        if (key.size() >= 3 && (name.contains(key) || id.contains(key))) {
            return true;
        }
    }
    return false;
}

void describePlan(Plan& plan)
{
    QStringList titles;
    for (const auto& theme : plan.themes) {
        titles << theme.title.toLower();
    }
    if (titles.isEmpty() && !plan.requestedMods.isEmpty()) {
        plan.name = tr("%1 pack %2").arg(plan.requestedMods.first(), plan.minecraftVersion);
        plan.summary = tr("Mods for: %1. Plus performance and quality-of-life mods.").arg(plan.requestedMods.join(", "));
    } else if (titles.isEmpty()) {
        plan.name = tr("Optimized %1 %2").arg(loaderDisplayName(plan.loader), plan.minecraftVersion);
        plan.summary = tr("A light pack with performance and quality-of-life mods.");
    } else {
        auto first = titles.first();
        first[0] = first[0].toUpper();
        plan.name = tr("%1 pack %2").arg(first, plan.minecraftVersion);
        plan.summary = tr("Mods for: %1. Plus performance and quality-of-life mods.").arg(titles.join(", "));
    }
    for (const auto& note : plan.notes) {
        plan.summary += " " + note.left(1).toUpper() + note.mid(1) + ".";
    }
}

Plan planFor(const QString& topic, const QString& loader, const QString& minecraftVersion)
{
    auto plan = parseLocally(topic);
    if (!minecraftVersion.isEmpty()) {
        plan.minecraftVersion = minecraftVersion;
        plan.versionGiven = true;
    }
    if (!loader.isEmpty()) {
        plan.loader = loader;
        plan.loaderGiven = true;
    }
    finishPlan(plan, topic);
    return plan;
}

bool wantsOnlyPerformance(const QString& request)
{
    const auto words = SearchTranslator::keywords(request);
    return words.contains("optimization") || words.contains("performance") || words.contains("fps") || words.contains("lag");
}

}  // namespace ModpackAgent
