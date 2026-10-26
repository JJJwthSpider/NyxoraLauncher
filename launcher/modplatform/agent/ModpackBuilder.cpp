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
#include "ModpackBuilder.h"

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <memory>

#include "Application.h"
#include "BuildConfig.h"
#include "meta/Index.h"

namespace ModpackAgent {

namespace {

// search results with fewer downloads are usually unfinished or abandoned
constexpr int s_minimumDownloads = 2000;
// required dependencies of dependencies are followed this many levels deep
constexpr int s_maxDependencyRounds = 5;

QString loaderUid(const QString& loader)
{
    if (loader == "neoforge")
        return "net.neoforged";
    if (loader == "forge")
        return "net.minecraftforge";
    if (loader == "quilt")
        return "org.quiltmc.quilt-loader";
    return "net.fabricmc.fabric-loader";
}

QString loaderIndexKey(const QString& loader)
{
    if (loader == "fabric")
        return "fabric-loader";
    if (loader == "quilt")
        return "quilt-loader";
    return loader;
}

QString projectIdOf(const QJsonObject& project)
{
    auto id = project["id"].toString();
    return id.isEmpty() ? project["project_id"].toString() : id;
}

}  // namespace

ModpackBuilder::ModpackBuilder(QObject* parent) : QObject(parent) {}

ModpackBuilder::~ModpackBuilder()
{
    abort();
}

void ModpackBuilder::build(const Plan& plan)
{
    abort();
    m_plan = plan;
    m_mods.clear();
    m_triedProjects.clear();
    m_loaderVersion.clear();
    m_requestsDone = 0;
    m_requestsStarted = 0;
    m_dependencyRounds = 0;
    m_running = true;

    emit log(tr("Minecraft %1, loader %2.").arg(m_plan.minecraftVersion, loaderDisplayName(m_plan.loader)));
    resolveLoaderVersion();
}

void ModpackBuilder::abort()
{
    m_running = false;
    m_onPhaseDone = nullptr;
    if (m_loaderTask) {
        disconnect(m_loaderTask.get(), nullptr, this, nullptr);
        m_loaderTask.reset();
    }
    for (auto& reply : m_replies) {
        if (reply) {
            reply->disconnect(this);
            reply->abort();
            reply->deleteLater();
        }
    }
    m_replies.clear();
    m_pending = 0;
}

QString ModpackBuilder::loadersJson() const
{
    // Quilt runs Fabric mods too
    if (m_plan.loader == "quilt")
        return R"(["quilt","fabric"])";
    return QString(R"(["%1"])").arg(m_plan.loader);
}

QString ModpackBuilder::versionsJson() const
{
    return QString(R"(["%1"])").arg(m_plan.minecraftVersion);
}

void ModpackBuilder::get(const QString& path, const QList<std::pair<QString, QString>>& query, JsonCallback callback)
{
    QUrl url(BuildConfig.MODRINTH_PROD_URL + path);
    QStringList parts;
    for (const auto& [key, value] : query) {
        parts << key + '=' + QString::fromLatin1(QUrl::toPercentEncoding(value));
    }
    if (!parts.isEmpty()) {
        url.setQuery(parts.join('&'), QUrl::StrictMode);
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, APPLICATION->getUserAgent());
    request.setTransferTimeout(60000);

    auto* reply = APPLICATION->network()->get(request);
    m_replies << reply;
    m_pending++;
    m_requestsStarted++;
    emit progress(m_requestsDone, m_requestsStarted);

    connect(reply, &QNetworkReply::finished, this, [this, reply, callback] {
        m_replies.removeAll(reply);
        reply->deleteLater();
        if (!m_running) {
            return;
        }
        const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto document = QJsonDocument::fromJson(reply->readAll());
        if (reply->error() != QNetworkReply::NoError && status == 0) {
            emit log(tr("Network error: %1").arg(reply->errorString()));
        }
        callback(document, status);
        requestDone();
    });
}

void ModpackBuilder::requestDone()
{
    m_pending--;
    m_requestsDone++;
    emit progress(m_requestsDone, m_requestsStarted);
    if (m_pending == 0) {
        phaseDone();
    }
}

void ModpackBuilder::phaseDone()
{
    if (!m_running || !m_onPhaseDone) {
        return;
    }
    auto next = std::move(m_onPhaseDone);
    m_onPhaseDone = nullptr;
    next();
}

void ModpackBuilder::resolveLoaderVersion()
{
    emit log(tr("Looking up the newest %1 for Minecraft %2...").arg(loaderDisplayName(m_plan.loader), m_plan.minecraftVersion));
    m_loaderList = APPLICATION->metadataIndex()->get(loaderUid(m_plan.loader));
    m_loaderTask = m_loaderList->getLoadTask();
    connect(m_loaderTask.get(), &Task::succeeded, this, [this] {
        m_loaderTask.reset();
        const bool fabricLike = m_plan.loader == "fabric" || m_plan.loader == "quilt";
        if (fabricLike) {
            // Fabric and Quilt loaders work with every Minecraft version
            if (auto recommended = m_loaderList->getRecommended()) {
                m_loaderVersion = recommended->descriptor();
            } else if (!m_loaderList->versions().isEmpty()) {
                m_loaderVersion = m_loaderList->versions().first()->descriptor();
            }
        } else {
            auto version = m_loaderList->getRecommendedForParent("net.minecraft", m_plan.minecraftVersion);
            if (!version) {
                version = m_loaderList->getLatestForParent("net.minecraft", m_plan.minecraftVersion);
            }
            if (version) {
                m_loaderVersion = version->descriptor();
            }
        }
        if (m_loaderVersion.isEmpty()) {
            m_running = false;
            emit failed(tr("%1 is not available for Minecraft %2. Try another version or loader.")
                            .arg(loaderDisplayName(m_plan.loader), m_plan.minecraftVersion));
            return;
        }
        emit log(tr("%1 %2 will be installed.").arg(loaderDisplayName(m_plan.loader), m_loaderVersion));
        resolveCurated();
    });
    connect(m_loaderTask.get(), &Task::failed, this, [this](const QString& reason) {
        m_loaderTask.reset();
        m_running = false;
        emit failed(tr("Could not load the list of %1 versions: %2").arg(loaderDisplayName(m_plan.loader), reason));
    });
    m_loaderTask->start();
}

void ModpackBuilder::resolveCurated()
{
    emit log(tr("Adding performance and quality-of-life mods..."));
    m_onPhaseDone = [this] { searchThemes(); };
    for (const auto& curated : m_plan.curated) {
        tryCurated(curated, 0);
    }
    if (m_pending == 0) {
        phaseDone();
    }
}

void ModpackBuilder::tryCurated(const CuratedMod& curated, int alternative)
{
    if (alternative >= curated.alternatives.size()) {
        emit log(tr("Skipped %1: not available for this version.").arg(curated.alternatives.join(" / ")));
        return;
    }
    const auto slug = curated.alternatives.at(alternative);
    get("/project/" + slug, {}, [this, curated, alternative](const QJsonDocument& doc, int status) {
        if (status != 200 || !doc.isObject()) {
            tryCurated(curated, alternative + 1);
            return;
        }
        const auto project = doc.object();
        if (project["project_type"].toString() != "mod") {
            tryCurated(curated, alternative + 1);
            return;
        }
        fetchVersion(projectIdOf(project), [this, curated, alternative, project](const QJsonObject& version) {
            if (version.isEmpty()) {
                tryCurated(curated, alternative + 1);
                return;
            }
            addMod(project, version, curated.group, false);
        });
    });
}

void ModpackBuilder::fetchVersion(const QString& projectId, std::function<void(const QJsonObject&)> callback)
{
    m_triedProjects.insert(projectId);
    get("/project/" + projectId + "/version", { { "loaders", loadersJson() }, { "game_versions", versionsJson() } },
        [callback](const QJsonDocument& doc, int status) {
            if (status != 200 || !doc.isArray() || doc.array().isEmpty()) {
                callback({});
                return;
            }
            // newest first; prefer a stable release over betas
            const auto versions = doc.array();
            for (const auto& value : versions) {
                if (value.toObject()["version_type"].toString() == "release") {
                    callback(value.toObject());
                    return;
                }
            }
            callback(versions.first().toObject());
        });
}

bool ModpackBuilder::addMod(const QJsonObject& project, const QJsonObject& version, const QString& group, bool dependency)
{
    const auto projectId = projectIdOf(project);
    for (const auto& mod : m_mods) {
        if (mod.projectId == projectId) {
            return false;
        }
    }

    AgentMod mod;
    mod.projectId = projectId;
    mod.slug = project["slug"].toString();
    mod.title = project["title"].toString();
    mod.description = project["description"].toString();
    mod.group = group;
    mod.dependency = dependency;
    mod.versionNumber = version["version_number"].toString();

    for (const auto& value : version["dependencies"].toArray()) {
        const auto dep = value.toObject();
        const auto depProject = dep["project_id"].toString();
        if (depProject.isEmpty()) {
            continue;
        }
        const auto type = dep["dependency_type"].toString();
        if (type == "required") {
            mod.requiredProjects << depProject;
        } else if (type == "incompatible") {
            mod.incompatibleProjects << depProject;
        }
    }

    for (const auto& other : m_mods) {
        if (mod.incompatibleProjects.contains(other.projectId) || other.incompatibleProjects.contains(mod.projectId)) {
            emit log(tr("Skipped %1: it does not work together with %2.").arg(mod.title, other.title));
            return false;
        }
    }

    const auto files = version["files"].toArray();
    QJsonObject file;
    for (const auto& value : files) {
        if (value.toObject()["primary"].toBool()) {
            file = value.toObject();
            break;
        }
    }
    if (file.isEmpty() && !files.isEmpty()) {
        file = files.first().toObject();
    }
    const auto hashes = file["hashes"].toObject();
    mod.fileName = file["filename"].toString();
    mod.url = file["url"].toString();
    mod.sha1 = hashes["sha1"].toString();
    mod.sha512 = hashes["sha512"].toString();
    mod.size = file["size"].toInteger();
    if (mod.fileName.isEmpty() || mod.url.isEmpty() || mod.sha512.isEmpty()) {
        emit log(tr("Skipped %1: the download is incomplete.").arg(mod.title));
        return false;
    }

    m_mods << mod;
    emit log(dependency ? tr("+ %1 (needed by other mods)").arg(mod.title) : tr("+ %1").arg(mod.title));
    return true;
}

void ModpackBuilder::searchThemes()
{
    if (m_plan.themes.isEmpty()) {
        resolveDependencies();
        return;
    }
    m_onPhaseDone = [this] { resolveDependencies(); };

    QString facets;
    if (m_plan.loader == "quilt") {
        facets = QString(R"([["categories:quilt","categories:fabric"],["versions:%1"],["project_type:mod"]])").arg(m_plan.minecraftVersion);
    } else {
        facets = QString(R"([["categories:%1"],["versions:%2"],["project_type:mod"]])").arg(m_plan.loader, m_plan.minecraftVersion);
    }

    for (int themeIndex = 0; themeIndex < m_plan.themes.size(); themeIndex++) {
        const auto& theme = m_plan.themes.at(themeIndex);
        emit log(tr("Searching mods for \"%1\"...").arg(theme.title));

        struct Collected {
            int remaining = 0;
            QList<QList<QJsonObject>> perQuery;
        };
        auto collected = std::make_shared<Collected>();
        collected->remaining = theme.queries.size();
        collected->perQuery.resize(theme.queries.size());

        for (int queryIndex = 0; queryIndex < theme.queries.size(); queryIndex++) {
            get("/search", { { "query", theme.queries.at(queryIndex) }, { "facets", facets }, { "index", "relevance" }, { "limit", "15" } },
                [this, themeIndex, queryIndex, collected](const QJsonDocument& doc, int status) {
                    if (status == 200) {
                        for (const auto& value : doc.object()["hits"].toArray()) {
                            const auto hit = value.toObject();
                            const auto categories = hit["categories"].toArray();
                            const bool library = categories.contains("library");
                            const bool serverOnly = hit["client_side"].toString() == "unsupported";
                            if (library || serverOnly || hit["downloads"].toInteger() < s_minimumDownloads) {
                                continue;
                            }
                            collected->perQuery[queryIndex] << hit;
                        }
                    }
                    if (--collected->remaining > 0) {
                        return;
                    }
                    // interleave the results of all queries so each search contributes its best matches
                    QList<QJsonObject> candidates;
                    QSet<QString> seen;
                    for (int rank = 0; rank < 15; rank++) {
                        for (const auto& list : collected->perQuery) {
                            if (rank < list.size()) {
                                const auto id = projectIdOf(list.at(rank));
                                if (!seen.contains(id)) {
                                    seen.insert(id);
                                    candidates << list.at(rank);
                                }
                            }
                        }
                    }
                    pickFromTheme(themeIndex, candidates, 0);
                });
        }
    }
}

void ModpackBuilder::pickFromTheme(int themeIndex, QList<QJsonObject> candidates, int taken)
{
    const auto& theme = m_plan.themes.at(themeIndex);
    while (taken < theme.count && !candidates.isEmpty()) {
        const auto hit = candidates.takeFirst();
        const auto id = projectIdOf(hit);
        if (m_triedProjects.contains(id)) {
            continue;
        }
        fetchVersion(id, [this, themeIndex, candidates, taken, hit](const QJsonObject& version) {
            const bool added = !version.isEmpty() && addMod(hit, version, m_plan.themes.at(themeIndex).title, false);
            pickFromTheme(themeIndex, candidates, taken + (added ? 1 : 0));
        });
        return;
    }
    if (taken == 0) {
        emit log(tr("Nothing suitable was found for \"%1\".").arg(theme.title));
    }
}

void ModpackBuilder::resolveDependencies()
{
    QSet<QString> present;
    for (const auto& mod : m_mods) {
        present.insert(mod.projectId);
    }
    QStringList missing;
    for (const auto& mod : m_mods) {
        for (const auto& required : mod.requiredProjects) {
            if (!present.contains(required) && !m_triedProjects.contains(required) && !missing.contains(required)) {
                missing << required;
            }
        }
    }
    if (missing.isEmpty() || m_dependencyRounds >= s_maxDependencyRounds) {
        finish();
        return;
    }
    m_dependencyRounds++;
    emit log(tr("Adding libraries the mods need..."));
    m_onPhaseDone = [this] { resolveDependencies(); };

    const auto group = tr("Libraries");
    for (const auto& projectId : missing) {
        m_triedProjects.insert(projectId);
        get("/project/" + projectId, {}, [this, projectId, group](const QJsonDocument& doc, int status) {
            if (status != 200) {
                return;
            }
            const auto project = doc.object();
            fetchVersion(projectId, [this, project, group](const QJsonObject& version) {
                if (!version.isEmpty()) {
                    addMod(project, version, group, true);
                }
            });
        });
    }
}

void ModpackBuilder::finish()
{
    // a mod whose required library could not be found would crash the game, so leave it out
    bool changed = true;
    while (changed) {
        changed = false;
        QSet<QString> present;
        for (const auto& mod : m_mods) {
            present.insert(mod.projectId);
        }
        for (int i = 0; i < m_mods.size(); i++) {
            for (const auto& required : m_mods.at(i).requiredProjects) {
                if (!present.contains(required)) {
                    emit log(tr("Removed %1: a library it needs is not available for this version.").arg(m_mods.at(i).title));
                    m_mods.removeAt(i);
                    changed = true;
                    break;
                }
            }
            if (changed) {
                break;
            }
        }
    }

    // keep the groups in the order of the plan: curated first, then themes, libraries last
    QStringList order;
    for (const auto& curated : m_plan.curated) {
        if (!order.contains(curated.group))
            order << curated.group;
    }
    for (const auto& theme : m_plan.themes) {
        if (!order.contains(theme.title))
            order << theme.title;
    }
    std::stable_sort(m_mods.begin(), m_mods.end(), [&order](const AgentMod& a, const AgentMod& b) {
        auto ia = a.dependency ? order.size() : order.indexOf(a.group);
        auto ib = b.dependency ? order.size() : order.indexOf(b.group);
        return ia < ib;
    });

    m_running = false;
    emit log(tr("Done: %n mod(s) selected.", "", m_mods.size()));
    emit finished();
}

QByteArray ModpackBuilder::makeIndex() const
{
    QJsonArray files;
    for (const auto& mod : m_mods) {
        if (!mod.enabled) {
            continue;
        }
        files.append(QJsonObject{
            { "path", "mods/" + mod.fileName },
            { "hashes", QJsonObject{ { "sha1", mod.sha1 }, { "sha512", mod.sha512 } } },
            { "downloads", QJsonArray{ mod.url } },
            { "fileSize", mod.size },
        });
    }
    QJsonObject index{
        { "formatVersion", 1 },
        { "game", "minecraft" },
        { "versionId", "1.0.0" },
        { "name", m_plan.name },
        { "summary", m_plan.summary },
        { "files", files },
        { "dependencies", QJsonObject{ { "minecraft", m_plan.minecraftVersion }, { loaderIndexKey(m_plan.loader), m_loaderVersion } } },
    };
    return QJsonDocument(index).toJson();
}

}  // namespace ModpackAgent
