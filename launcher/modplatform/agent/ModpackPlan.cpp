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
          { "zombie", "zombie apocalypse" },
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

    // classic versions (1.7.10, 1.12.2, 1.21.1) and the year based ones Mojang uses since 2026 (26.1, 26.1.2)
    static const QRegularExpression s_version(
        QStringLiteral("(?<![\\d.])(1\\.\\d{1,2}(?:\\.\\d{1,2})?|(?:2[5-9]|[3-9]\\d)\\.\\d{1,2}(?:\\.\\d{1,2})?)(?![\\d.])"));
    if (auto match = s_version.match(lower); match.hasMatch()) {
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

    for (const auto& rule : themeRules()) {
        bool hit = false;
        for (const auto& trigger : rule.triggers) {
            if (words.contains(trigger)) {
                hit = true;
                break;
            }
        }
        if (hit) {
            plan.themes << Theme{ tr(rule.title), rule.queries, rule.count };
        }
    }

    // words that matched no theme are searched for as they are, so "сборка с ведьмами" still finds something
    QStringList known{ "neoforge", "forge",   "fabric",   "quilt", "optimization", "performance", "fps",
                       "lag",      "shaders", "graphics", "map",   "minimap",      "textures" };
    for (const auto& rule : themeRules()) {
        known << rule.triggers;
    }
    for (const auto& word : words) {
        if (word.size() < 3 || known.contains(word) || s_version.match(word).hasMatch() || SearchTranslator::hasCyrillic(word)) {
            continue;
        }
        plan.themes << Theme{ word, { word }, 2 };
    }

    plan.curated = essentialMods(plan.loader, requestWantsShaders(request), requestWantsMap(request));
    describePlan(plan);
    return plan;
}

void describePlan(Plan& plan)
{
    QStringList titles;
    for (const auto& theme : plan.themes) {
        titles << theme.title.toLower();
    }
    if (titles.isEmpty()) {
        plan.name = tr("Optimized %1 %2").arg(loaderDisplayName(plan.loader), plan.minecraftVersion);
        plan.summary = tr("A light pack with performance and quality-of-life mods.");
    } else {
        auto first = titles.first();
        first[0] = first[0].toUpper();
        plan.name = tr("%1 pack %2").arg(first, plan.minecraftVersion);
        plan.summary = tr("Mods for: %1. Plus performance and quality-of-life mods.").arg(titles.join(", "));
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
    plan.curated = essentialMods(plan.loader, requestWantsShaders(topic), requestWantsMap(topic));
    describePlan(plan);
    return plan;
}

bool wantsOnlyPerformance(const QString& request)
{
    const auto words = SearchTranslator::keywords(request);
    return words.contains("optimization") || words.contains("performance") || words.contains("fps") || words.contains("lag");
}

}  // namespace ModpackAgent
