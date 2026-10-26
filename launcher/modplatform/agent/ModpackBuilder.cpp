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
constexpr int s_searchLimit = 15;

// CurseForge constants: https://docs.curseforge.com/rest-api/
constexpr int s_cfMinecraftGameId = 432;
constexpr int s_cfModsClassId = 6;
constexpr int s_cfSortByPopularity = 2;
constexpr int s_cfReleaseFile = 1;
constexpr int s_cfRequiredDependency = 3;
constexpr int s_cfIncompatible = 5;
constexpr int s_cfSha1 = 1;

const QString s_modrinth = QStringLiteral("modrinth");
const QString s_curseForge = QStringLiteral("curseforge");

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

Candidate modrinthCandidate(const QJsonObject& project)
{
    auto id = project["id"].toString();
    if (id.isEmpty()) {
        id = project["project_id"].toString();  // search hits
    }
    return { s_modrinth, "mr:" + id, project["slug"].toString(), project["title"].toString(), project["description"].toString() };
}

Candidate curseForgeCandidate(const QJsonObject& mod)
{
    return { s_curseForge, "cf:" + QString::number(mod["id"].toInteger()), mod["slug"].toString(), mod["name"].toString(),
             mod["summary"].toString() };
}

bool curseForgeIsLibrary(const QJsonObject& mod)
{
    for (const auto& value : mod["categories"].toArray()) {
        if (value.toObject()["slug"].toString().contains("library")) {
            return true;
        }
    }
    return false;
}

QString rawId(const QString& prefixedId)
{
    return prefixedId.mid(3);
}

}  // namespace

ModpackBuilder::ModpackBuilder(QObject* parent) : QObject(parent) {}

ModpackBuilder::~ModpackBuilder()
{
    abort();
}

void ModpackBuilder::build(const Plan& plan, bool useModrinth, bool useCurseForge)
{
    abort();
    m_plan = plan;
    m_useModrinth = useModrinth;
    m_useCurseForge = useCurseForge;
    m_mods.clear();
    m_triedProjects.clear();
    m_loaderVersion.clear();
    m_requestsDone = 0;
    m_requestsStarted = 0;
    m_dependencyRounds = 0;
    m_running = true;

    if (m_useCurseForge && APPLICATION->getFlameAPIKey().isEmpty()) {
        emit log(tr("CurseForge is not available in this build (no API key), only Modrinth is used."));
        m_useCurseForge = false;
    }
    if (!m_useModrinth && !m_useCurseForge) {
        m_running = false;
        emit failed(tr("Choose at least one source of mods: Modrinth or CurseForge."));
        return;
    }

    QStringList sources;
    if (m_useModrinth)
        sources << "Modrinth";
    if (m_useCurseForge)
        sources << "CurseForge";
    emit log(
        tr("Minecraft %1, loader %2, mods from %3.").arg(m_plan.minecraftVersion, loaderDisplayName(m_plan.loader), sources.join(" + ")));
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

QString ModpackBuilder::modrinthLoaders() const
{
    // Quilt runs Fabric mods too
    if (m_plan.loader == "quilt")
        return R"(["quilt","fabric"])";
    return QString(R"(["%1"])").arg(m_plan.loader);
}

int ModpackBuilder::curseForgeLoaderType() const
{
    if (m_plan.loader == "forge")
        return 1;
    if (m_plan.loader == "fabric")
        return 4;
    if (m_plan.loader == "quilt")
        return 5;
    return 6;  // NeoForge
}

bool ModpackBuilder::curseForgeFileFitsLoader(const QJsonArray& gameVersions) const
{
    QStringList accepted{ loaderDisplayName(m_plan.loader).toLower() };
    if (m_plan.loader == "quilt") {
        accepted << "fabric";
    }
    bool loaderOk = false;
    bool versionOk = false;
    for (const auto& value : gameVersions) {
        const auto tag = value.toString().toLower();
        if (accepted.contains(tag)) {
            loaderOk = true;
        }
        if (tag == m_plan.minecraftVersion) {
            versionOk = true;
        }
    }
    return loaderOk && versionOk;
}

void ModpackBuilder::get(const QString& baseUrl, const QList<std::pair<QString, QString>>& query, bool curseForge, JsonCallback callback)
{
    QUrl url(baseUrl);
    QStringList parts;
    for (const auto& [key, value] : query) {
        parts << key + '=' + QString::fromLatin1(QUrl::toPercentEncoding(value));
    }
    if (!parts.isEmpty()) {
        url.setQuery(parts.join('&'), QUrl::StrictMode);
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, APPLICATION->getUserAgent());
    request.setRawHeader("Accept", "application/json");
    if (curseForge) {
        request.setRawHeader("x-api-key", APPLICATION->getFlameAPIKey().toUtf8());
    }
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

/* ---------------------------------------------------------------- lookups */

void ModpackBuilder::findBySlug(const QString& slug, bool curseForge, CandidateCallback callback)
{
    if (!curseForge) {
        get(BuildConfig.MODRINTH_PROD_URL + "/project/" + slug, {}, false, [callback](const QJsonDocument& doc, int status) {
            const auto project = doc.object();
            if (status != 200 || project["project_type"].toString() != "mod") {
                callback({});
                return;
            }
            callback(modrinthCandidate(project));
        });
        return;
    }
    get(BuildConfig.FLAME_BASE_URL + "/mods/search",
        { { "gameId", QString::number(s_cfMinecraftGameId) }, { "classId", QString::number(s_cfModsClassId) }, { "slug", slug } }, true,
        [callback](const QJsonDocument& doc, int status) {
            const auto data = doc.object()["data"].toArray();
            if (status != 200 || data.isEmpty()) {
                callback({});
                return;
            }
            callback(curseForgeCandidate(data.first().toObject()));
        });
}

void ModpackBuilder::findById(const QString& prefixedId, CandidateCallback callback)
{
    if (prefixedId.startsWith("mr:")) {
        get(BuildConfig.MODRINTH_PROD_URL + "/project/" + rawId(prefixedId), {}, false,
            [callback](const QJsonDocument& doc, int status) { callback(status == 200 ? modrinthCandidate(doc.object()) : Candidate{}); });
    } else {
        get(BuildConfig.FLAME_BASE_URL + "/mods/" + rawId(prefixedId), {}, true, [callback](const QJsonDocument& doc, int status) {
            callback(status == 200 ? curseForgeCandidate(doc.object()["data"].toObject()) : Candidate{});
        });
    }
}

void ModpackBuilder::fetchFile(const Candidate& candidate, FileCallback callback)
{
    m_triedProjects.insert(candidate.id);
    if (candidate.platform == s_modrinth) {
        fetchModrinthFile(candidate, callback);
    } else {
        fetchCurseForgeFile(candidate, callback);
    }
}

void ModpackBuilder::fetchModrinthFile(const Candidate& candidate, FileCallback callback)
{
    get(BuildConfig.MODRINTH_PROD_URL + "/project/" + rawId(candidate.id) + "/version",
        { { "loaders", modrinthLoaders() }, { "game_versions", QString(R"(["%1"])").arg(m_plan.minecraftVersion) } }, false,
        [callback](const QJsonDocument& doc, int status) {
            FileChoice choice;
            if (status != 200 || !doc.isArray() || doc.array().isEmpty()) {
                callback(choice);
                return;
            }
            // newest first; prefer a stable release over betas
            const auto versions = doc.array();
            auto version = versions.first().toObject();
            for (const auto& value : versions) {
                if (value.toObject()["version_type"].toString() == "release") {
                    version = value.toObject();
                    break;
                }
            }

            const auto files = version["files"].toArray();
            QJsonObject file = files.isEmpty() ? QJsonObject{} : files.first().toObject();
            for (const auto& value : files) {
                if (value.toObject()["primary"].toBool()) {
                    file = value.toObject();
                    break;
                }
            }
            const auto hashes = file["hashes"].toObject();
            choice.versionNumber = version["version_number"].toString();
            choice.fileName = file["filename"].toString();
            choice.url = file["url"].toString();
            choice.sha1 = hashes["sha1"].toString();
            choice.sha512 = hashes["sha512"].toString();
            choice.size = file["size"].toInteger();
            for (const auto& value : version["dependencies"].toArray()) {
                const auto dep = value.toObject();
                const auto project = dep["project_id"].toString();
                if (project.isEmpty()) {
                    continue;
                }
                const auto type = dep["dependency_type"].toString();
                if (type == "required") {
                    choice.required << "mr:" + project;
                } else if (type == "incompatible") {
                    choice.incompatible << "mr:" + project;
                }
            }
            choice.valid = !choice.fileName.isEmpty() && !choice.url.isEmpty() && !choice.sha512.isEmpty();
            if (!choice.valid) {
                choice.problem = tr("the download is incomplete");
            }
            callback(choice);
        });
}

void ModpackBuilder::fetchCurseForgeFile(const Candidate& candidate, FileCallback callback)
{
    get(BuildConfig.FLAME_BASE_URL + "/mods/" + rawId(candidate.id) + "/files",
        { { "gameVersion", m_plan.minecraftVersion }, { "modLoaderType", QString::number(curseForgeLoaderType()) }, { "pageSize", "30" } },
        true, [this, callback](const QJsonDocument& doc, int status) {
            FileChoice choice;
            if (status != 200) {
                callback(choice);
                return;
            }
            // the API returns the newest files first; take the newest release that really fits, else the newest that fits
            QJsonObject file;
            for (const auto& value : doc.object()["data"].toArray()) {
                const auto candidateFile = value.toObject();
                if (!curseForgeFileFitsLoader(candidateFile["gameVersions"].toArray())) {
                    continue;
                }
                if (file.isEmpty()) {
                    file = candidateFile;
                }
                if (candidateFile["releaseType"].toInt() == s_cfReleaseFile) {
                    file = candidateFile;
                    break;
                }
            }
            if (file.isEmpty()) {
                callback(choice);
                return;
            }

            choice.versionNumber = file["displayName"].toString();
            choice.fileName = file["fileName"].toString();
            choice.url = file["downloadUrl"].toString();
            choice.size = file["fileLength"].toInteger();
            for (const auto& value : file["hashes"].toArray()) {
                const auto hash = value.toObject();
                if (hash["algo"].toInt() == s_cfSha1) {
                    choice.sha1 = hash["value"].toString();
                }
            }
            for (const auto& value : file["dependencies"].toArray()) {
                const auto dep = value.toObject();
                const auto id = "cf:" + QString::number(dep["modId"].toInteger());
                const auto type = dep["relationType"].toInt();
                if (type == s_cfRequiredDependency) {
                    choice.required << id;
                } else if (type == s_cfIncompatible) {
                    choice.incompatible << id;
                }
            }
            if (choice.url.isEmpty()) {
                // the author turned off downloads through third party launchers
                choice.problem = tr("its author allows downloads only through the CurseForge app");
            } else if (choice.sha1.isEmpty()) {
                choice.problem = tr("the download is incomplete");
            } else {
                choice.valid = true;
            }
            callback(choice);
        });
}

const AgentMod* ModpackBuilder::findSameMod(const Candidate& candidate) const
{
    const auto name = normalized(candidate.title);
    const auto slug = normalized(candidate.slug);
    for (const auto& mod : m_mods) {
        if (mod.projectId == candidate.id || mod.aliases.contains(candidate.id)) {
            return &mod;
        }
        // the same mod published on both platforms
        if ((!name.isEmpty() && normalized(mod.title) == name) || (!slug.isEmpty() && normalized(mod.slug) == slug)) {
            return &mod;
        }
    }
    return nullptr;
}

bool ModpackBuilder::addMod(const Candidate& candidate, const FileChoice& file, const QString& group, bool dependency)
{
    if (const auto* same = findSameMod(candidate)) {
        // the same mod from the other platform: remember its id, so mods that need it see it as present
        for (auto& mod : m_mods) {
            if (&mod == same && mod.projectId != candidate.id && !mod.aliases.contains(candidate.id)) {
                mod.aliases << candidate.id;
            }
        }
        return false;
    }
    if (!file.valid) {
        if (!file.problem.isEmpty()) {
            emit log(tr("Skipped %1: %2.").arg(candidate.title, file.problem));
        }
        return false;
    }

    for (const auto& other : m_mods) {
        bool conflict = file.incompatible.contains(other.projectId) || other.incompatibleProjects.contains(candidate.id);
        for (const auto& alias : other.aliases) {
            conflict = conflict || file.incompatible.contains(alias);
        }
        if (conflict) {
            emit log(tr("Skipped %1: it does not work together with %2.").arg(candidate.title, other.title));
            return false;
        }
    }

    AgentMod mod;
    mod.platform = candidate.platform;
    mod.projectId = candidate.id;
    mod.slug = candidate.slug;
    mod.title = candidate.title;
    mod.description = candidate.description;
    mod.group = group;
    mod.dependency = dependency;
    mod.versionNumber = file.versionNumber;
    mod.fileName = file.fileName;
    mod.url = file.url;
    mod.sha1 = file.sha1;
    mod.sha512 = file.sha512;
    mod.size = file.size;
    mod.requiredProjects = file.required;
    mod.incompatibleProjects = file.incompatible;
    m_mods << mod;

    const auto source = candidate.platform == s_modrinth ? QStringLiteral("Modrinth") : QStringLiteral("CurseForge");
    emit log(dependency ? tr("+ %1 (needed by other mods, %2)").arg(mod.title, source) : tr("+ %1 (%2)").arg(mod.title, source));
    return true;
}

/* ---------------------------------------------------------------- phases */

void ModpackBuilder::resolveCurated()
{
    emit log(tr("Adding performance and quality-of-life mods..."));
    m_onPhaseDone = [this] { searchThemes(); };
    for (const auto& curated : m_plan.curated) {
        tryCurated(curated, 0, !m_useModrinth);
    }
    if (m_pending == 0) {
        phaseDone();
    }
}

void ModpackBuilder::tryCurated(const CuratedMod& curated, int alternative, bool curseForge)
{
    if (alternative >= curated.alternatives.size()) {
        emit log(tr("Skipped %1: not available for this version.").arg(curated.alternatives.join(" / ")));
        return;
    }
    // every alternative is tried on Modrinth first and then on CurseForge
    auto next = [this, curated, alternative, curseForge] {
        if (!curseForge && m_useCurseForge) {
            tryCurated(curated, alternative, true);
        } else {
            tryCurated(curated, alternative + 1, !m_useModrinth);
        }
    };
    findBySlug(curated.alternatives.at(alternative), curseForge, [this, curated, next](const Candidate& candidate) {
        if (candidate.id.isEmpty()) {
            next();
            return;
        }
        if (findSameMod(candidate)) {
            return;  // already in the pack
        }
        fetchFile(candidate, [this, candidate, curated, next](const FileChoice& file) {
            if (!file.valid) {
                next();
                return;
            }
            addMod(candidate, file, curated.group, false);
        });
    });
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

        // one result list per query and platform; they are merged when all searches of the theme are back
        struct Collected {
            int remaining = 0;
            QList<QList<Candidate>> lists;
        };
        auto collected = std::make_shared<Collected>();
        const int platforms = (m_useModrinth ? 1 : 0) + (m_useCurseForge ? 1 : 0);
        collected->remaining = theme.queries.size() * platforms;
        collected->lists.resize(collected->remaining);

        auto merge = [this, themeIndex, collected] {
            if (--collected->remaining > 0) {
                return;
            }
            // interleave all lists so every search and both platforms contribute their best matches
            QList<Candidate> candidates;
            for (int rank = 0; rank < s_searchLimit; rank++) {
                for (const auto& list : collected->lists) {
                    if (rank < list.size()) {
                        candidates << list.at(rank);
                    }
                }
            }
            pickFromTheme(themeIndex, candidates, 0);
        };

        int listIndex = 0;
        for (const auto& query : theme.queries) {
            if (m_useModrinth) {
                const int slot = listIndex++;
                get(BuildConfig.MODRINTH_PROD_URL + "/search",
                    { { "query", query }, { "facets", facets }, { "index", "relevance" }, { "limit", QString::number(s_searchLimit) } },
                    false, [collected, slot, merge](const QJsonDocument& doc, int status) {
                        if (status == 200) {
                            for (const auto& value : doc.object()["hits"].toArray()) {
                                const auto hit = value.toObject();
                                const bool library = hit["categories"].toArray().contains("library");
                                const bool serverOnly = hit["client_side"].toString() == "unsupported";
                                if (library || serverOnly || hit["downloads"].toInteger() < s_minimumDownloads) {
                                    continue;
                                }
                                collected->lists[slot] << modrinthCandidate(hit);
                            }
                        }
                        merge();
                    });
            }
            if (m_useCurseForge) {
                const int slot = listIndex++;
                get(BuildConfig.FLAME_BASE_URL + "/mods/search",
                    { { "gameId", QString::number(s_cfMinecraftGameId) },
                      { "classId", QString::number(s_cfModsClassId) },
                      { "searchFilter", query },
                      { "gameVersion", m_plan.minecraftVersion },
                      { "modLoaderType", QString::number(curseForgeLoaderType()) },
                      { "sortField", QString::number(s_cfSortByPopularity) },
                      { "sortOrder", "desc" },
                      { "pageSize", QString::number(s_searchLimit) } },
                    true, [collected, slot, merge](const QJsonDocument& doc, int status) {
                        if (status == 200) {
                            for (const auto& value : doc.object()["data"].toArray()) {
                                const auto mod = value.toObject();
                                if (curseForgeIsLibrary(mod) || mod["downloadCount"].toDouble() < s_minimumDownloads) {
                                    continue;
                                }
                                collected->lists[slot] << curseForgeCandidate(mod);
                            }
                        }
                        merge();
                    });
            }
        }
    }
}

void ModpackBuilder::pickFromTheme(int themeIndex, QList<Candidate> candidates, int taken)
{
    const auto& theme = m_plan.themes.at(themeIndex);
    while (taken < theme.count && !candidates.isEmpty()) {
        const auto candidate = candidates.takeFirst();
        if (m_triedProjects.contains(candidate.id) || findSameMod(candidate)) {
            continue;
        }
        fetchFile(candidate, [this, themeIndex, candidates, taken, candidate](const FileChoice& file) {
            const bool added = addMod(candidate, file, m_plan.themes.at(themeIndex).title, false);
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
        for (const auto& alias : mod.aliases) {
            present.insert(alias);
        }
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
        if (projectId.startsWith("cf:") && !m_useCurseForge) {
            continue;
        }
        findById(projectId, [this, projectId, group](const Candidate& candidate) {
            if (candidate.id.isEmpty()) {
                return;
            }
            // the library may already be in the pack from the other platform
            if (const auto* same = findSameMod(candidate)) {
                for (auto& mod : m_mods) {
                    if (&mod == same) {
                        mod.aliases << projectId;
                    }
                }
                return;
            }
            fetchFile(candidate, [this, candidate, group](const FileChoice& file) { addMod(candidate, file, group, true); });
        });
    }
    if (m_pending == 0) {
        phaseDone();
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
            for (const auto& alias : mod.aliases) {
                present.insert(alias);
            }
        }
        for (int i = 0; i < m_mods.size() && !changed; i++) {
            for (const auto& required : m_mods.at(i).requiredProjects) {
                if (!present.contains(required)) {
                    emit log(tr("Removed %1: a library it needs is not available for this version.").arg(m_mods.at(i).title));
                    m_mods.removeAt(i);
                    changed = true;
                    break;
                }
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
        QJsonObject hashes{ { "sha1", mod.sha1 } };
        if (!mod.sha512.isEmpty()) {
            hashes["sha512"] = mod.sha512;  // CurseForge only provides sha1
        }
        files.append(QJsonObject{
            { "path", "mods/" + mod.fileName },
            { "hashes", hashes },
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
