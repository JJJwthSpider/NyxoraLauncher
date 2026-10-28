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

#include <QJsonArray>
#include <QHash>
#include <QJsonDocument>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <functional>

#include "ModpackPlan.h"
#include "meta/VersionList.h"
#include "tasks/Task.h"

class QNetworkReply;

namespace ModpackAgent {

/// One mod file that ends up in the pack.
struct AgentMod {
    QString platform;     // "modrinth" or "curseforge"
    QString projectId;    // with platform prefix: "mr:AANobbMI", "cf:238222"
    QStringList aliases;  // the same mod on the other platform (filled when a dependency points there)
    QString slug;
    QString title;
    QString description;
    QString group;  // "Performance", "Zombies", "Libraries", ...
    QString versionNumber;
    QString fileName;
    QString url;
    QString sha1;
    QString sha512;  // Modrinth only
    qint64 size = 0;
    bool dependency = false;           // pulled in because another mod needs it
    bool requested = false;            // the user named this mod
    QStringList neededBy;              // titles of the mods that need this library
    bool enabled = true;               // the user can switch mods off before creating the instance
    QStringList requiredProjects;      // prefixed ids
    QStringList incompatibleProjects;  // prefixed ids
};

/// A project found on one of the platforms, before a file is chosen.
struct Candidate {
    QString platform;
    QString id;  // prefixed
    QString slug;
    QString title;
    QString description;
};

/// The file of a project that fits the Minecraft version and loader.
struct FileChoice {
    bool valid = false;
    QString versionNumber;
    QString fileName;
    QString url;
    QString sha1;
    QString sha512;
    qint64 size = 0;
    QStringList required;
    QStringList incompatible;
    QString problem;  // why there is no usable file (shown in the log)
};

/// Turns a Plan into a concrete list of compatible mod files using the Modrinth and CurseForge APIs:
/// picks the curated mods, searches mods for every theme on both platforms, resolves required
/// dependencies, drops duplicates and mods that declare themselves incompatible with each other,
/// and finds the loader version.
class ModpackBuilder : public QObject {
    Q_OBJECT
   public:
    explicit ModpackBuilder(QObject* parent = nullptr);
    ~ModpackBuilder() override;

    void build(const Plan& plan, bool useModrinth, bool useCurseForge);
    void abort();
    bool isRunning() const { return m_running; }

    const Plan& plan() const { return m_plan; }
    QList<AgentMod>& mods() { return m_mods; }
    QString loaderVersion() const { return m_loaderVersion; }
    /// the mods the user named: what was found (titles) and what was not (as the user wrote it)
    QStringList foundRequested() const { return m_foundRequested; }
    QStringList missingRequested() const { return m_missingRequested; }
    /// themes for which no fitting mod exists for this version and loader
    QStringList emptyThemes() const { return m_emptyThemes; }
    /// the loader and version that have the most mods for the first empty theme ("Forge 1.20.1"), and how many
    QString suggestedSetup() const { return m_suggestedSetup; }
    int suggestedSetupMods() const { return m_suggestedSetupMods; }

    /// modrinth.index.json for the enabled mods (see https://support.modrinth.com/en/articles/8802351)
    QByteArray makeIndex() const;

   signals:
    void log(QString message);
    void progress(int done, int total);
    void finished();
    void failed(QString reason);

   private:
    using JsonCallback = std::function<void(const QJsonDocument&, int status)>;
    using CandidateCallback = std::function<void(const Candidate&)>;
    using FileCallback = std::function<void(const FileChoice&)>;

    void get(const QString& url, const QList<std::pair<QString, QString>>& query, bool curseForge, JsonCallback callback);
    void requestDone();
    void phaseDone();

    void checkMinecraftVersion();
    void resolveLoaderVersion();
    void resolveRequested();
    void tryRequested(const QString& name, QList<Candidate> candidates);
    void resolveCurated();
    void tryCurated(const CuratedMod& curated, int alternative, bool curseForge);
    void searchThemes();
    /// wide: the second try for themes that got too few mods, with more queries, more results and fewer downloads
    void runThemeSearch(const QList<int>& themes, bool wide);
    void widenThemes();
    void themesDone();
    void suggestSetup(const Theme& theme);
    void pickFromTheme(int themeIndex, QList<Candidate> candidates);
    void resolveDependencies();
    void addLibrary(const QString& requiredId);
    void libraryFromOtherPlatform(const QString& requiredId, const Candidate& original, const QString& problem);
    /// if the candidate is already in the pack, remembers requiredId as another id of it and returns true
    bool aliasExisting(const Candidate& candidate, const QString& requiredId);
    void finish();

    // platform specific lookups; callbacks get an empty Candidate / invalid FileChoice when nothing fits
    void findBySlug(const QString& slug, bool curseForge, CandidateCallback callback);
    void findById(const QString& prefixedId, CandidateCallback callback);
    /// searches by name; the callback gets the project whose title or slug is exactly that name, else an empty Candidate
    void findByName(const QString& name, bool curseForge, CandidateCallback callback);
    QString modrinthFacets() const;
    void fetchFile(const Candidate& candidate, FileCallback callback);
    void fetchModrinthFile(const Candidate& candidate, FileCallback callback);
    void fetchCurseForgeFile(const Candidate& candidate, FileCallback callback);

    /// adds a mod; returns false if it is already there (on any platform) or conflicts with another mod
    bool addMod(const Candidate& candidate, const FileChoice& file, const QString& group, bool dependency);
    const AgentMod* findSameMod(const Candidate& candidate) const;
    int indexOfSameMod(const Candidate& candidate) const;

    QString modrinthLoaders() const;
    int curseForgeLoaderType() const;
    bool curseForgeFileFitsLoader(const QJsonArray& gameVersions) const;

    Plan m_plan;
    bool m_useModrinth = true;
    bool m_useCurseForge = true;
    QList<AgentMod> m_mods;
    QSet<QString> m_triedProjects;
    QSet<QString> m_triedDependencies;
    QHash<QString, QString> m_libraryNames;  // prefixed id -> title, for messages about missing libraries
    QStringList m_foundRequested;
    QStringList m_missingRequested;
    QList<int> m_themeTaken;
    QStringList m_emptyThemes;
    QString m_suggestedSetup;
    int m_suggestedSetupMods = 0;
    QString m_loaderVersion;
    Meta::VersionList::Ptr m_minecraftList;
    Meta::VersionList::Ptr m_loaderList;
    Task::Ptr m_loaderTask;

    QList<QPointer<QNetworkReply>> m_replies;
    int m_pending = 0;
    std::function<void()> m_onPhaseDone;
    int m_requestsDone = 0;
    int m_requestsStarted = 0;
    bool m_running = false;
    int m_dependencyRounds = 0;
};

}  // namespace ModpackAgent
