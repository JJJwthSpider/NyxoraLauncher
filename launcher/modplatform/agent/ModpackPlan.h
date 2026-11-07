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
#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace ModpackAgent {

/// A group of mods the agent looks for, e.g. "Zombies" with the searches "zombie", "zombie apocalypse".
struct Theme {
    QString title;        // shown to the user (already translated)
    QStringList queries;  // English search terms for Modrinth
    int count = 3;        // how many mods to take for this theme
    QStringList matchWords;  // a found mod must mention one of these (English) in its name or description; empty: the queries
};

/// A hand-picked mod. The first slug of the alternatives that exists for the version/loader is used.
struct CuratedMod {
    QStringList alternatives;  // Modrinth slugs
    QString group;             // shown to the user (already translated)
    QString kind = "mod";      // "mod", or "shader" for a shader pack (goes to shaderpacks/, needs Iris or Oculus)
};

/// Everything the agent needs to know to assemble a pack.
struct Plan {
    QString name;
    QString summary;
    QString minecraftVersion;
    QString loader;  // "neoforge", "forge", "fabric" or "quilt"
    QList<Theme> themes;
    QList<CuratedMod> curated;
    QStringList requestedMods;  // mods the user named ("Create", "Farmer's Delight"), searched by name
    QStringList excluded;       // what the user asked to leave out: mod names and English keywords
    QStringList unknownWords;   // Russian words the agent did not understand, reported back to the user
    bool fromClaude = false;
    bool versionGiven = false;
    bool loaderGiven = false;
};

/// The Minecraft version used when the request does not name one.
QString defaultMinecraftVersion();

/// The loader used when the request does not name one.
QString defaultLoader(const QString& minecraftVersion);

/// Human readable name of a loader id ("neoforge" -> "NeoForge").
QString loaderDisplayName(const QString& loader);

/// Understands requests like "сборка с зомби neoforge 1.21.1" without any AI service.
Plan parseLocally(const QString& request);

/// The plan for a topic described in a chat, with the loader and version the user chose separately.
/// Empty loader / version fall back to what the topic says or to the defaults.
Plan planFor(const QString& topic, const QString& loader, const QString& minecraftVersion);

/// Fills name and summary from the themes, version and loader.
void describePlan(Plan& plan);

/// The request is about performance only ("оптимизация для слабого ПК").
bool wantsOnlyPerformance(const QString& request);

/// Performance and quality-of-life mods that are added to every pack.
QList<CuratedMod> essentialMods(const QString& loader, bool wantsShaders, bool wantsMap);

/// True if the mod should be left out because the user asked for a pack without it.
bool isExcluded(const Plan& plan, const QString& title, const QString& slug);

/// Detects shader / map wishes in a request (used to pick the right curated mods).
bool requestWantsShaders(const QString& request);
bool requestWantsMap(const QString& request);

}  // namespace ModpackAgent
