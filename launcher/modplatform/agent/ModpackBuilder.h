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
    QString projectId;
    QString slug;
    QString title;
    QString description;
    QString group;  // "Performance", "Zombies", "Libraries", ...
    QString versionNumber;
    QString fileName;
    QString url;
    QString sha1;
    QString sha512;
    qint64 size = 0;
    bool dependency = false;  // pulled in because another mod needs it
    bool enabled = true;      // the user can switch mods off before creating the instance
    QStringList requiredProjects;
    QStringList incompatibleProjects;
};

/// Turns a Plan into a concrete list of compatible mod files using the Modrinth API:
/// picks the curated mods, searches mods for every theme, resolves required dependencies,
/// drops mods that declare themselves incompatible with each other and finds the loader version.
class ModpackBuilder : public QObject {
    Q_OBJECT
   public:
    explicit ModpackBuilder(QObject* parent = nullptr);
    ~ModpackBuilder() override;

    void build(const Plan& plan);
    void abort();
    bool isRunning() const { return m_running; }

    const Plan& plan() const { return m_plan; }
    QList<AgentMod>& mods() { return m_mods; }
    QString loaderVersion() const { return m_loaderVersion; }

    /// modrinth.index.json for the enabled mods (see https://support.modrinth.com/en/articles/8802351)
    QByteArray makeIndex() const;

   signals:
    void log(QString message);
    void progress(int done, int total);
    void finished();
    void failed(QString reason);

   private:
    using JsonCallback = std::function<void(const QJsonDocument&, int status)>;

    void get(const QString& path, const QList<std::pair<QString, QString>>& query, JsonCallback callback);
    void requestDone();
    void phaseDone();

    void resolveLoaderVersion();
    void resolveCurated();
    void tryCurated(const CuratedMod& curated, int alternative);
    void searchThemes();
    void pickFromTheme(int themeIndex, QList<QJsonObject> candidates, int taken);
    void resolveDependencies();
    void finish();

    /// fetches the newest compatible file of a project; calls back with an empty object if there is none
    void fetchVersion(const QString& projectId, std::function<void(const QJsonObject& version)> callback);
    /// adds a mod; returns false if it is already there or conflicts with another mod
    bool addMod(const QJsonObject& project, const QJsonObject& version, const QString& group, bool dependency);

    QString loadersJson() const;
    QString versionsJson() const;

    Plan m_plan;
    QList<AgentMod> m_mods;
    QSet<QString> m_triedProjects;
    QString m_loaderVersion;
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
