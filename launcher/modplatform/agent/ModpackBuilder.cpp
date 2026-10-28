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
#include <QMap>
#include <QRegularExpression>
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
    return prefixedId.mid(prefixedId.indexOf(':') + 1);
}

/// Word beginnings a mod of the theme must mention: "zombie" -> "zomb", so "zombies" and "zombified" match too.
QRegularExpression themePattern(const Theme& theme)
{
    QStringList stems;
    const auto words = theme.matchWords.isEmpty() ? theme.queries : theme.matchWords;
    for (const auto& phrase : words) {
        for (const auto& word : phrase.toLower().split(' ', Qt::SkipEmptyParts)) {
            const auto stem = word.size() > 5 ? word.left(word.size() - 2) : word;
            if (stem.size() >= 3 && !stems.contains(stem)) {
                stems << QRegularExpression::escape(stem);
            }
        }
    }
    if (stems.isEmpty()) {
        return QRegularExpression(QStringLiteral(".*"));
    }
    // only at the start of a word: "pet" must not match "carpet"
    return QRegularExpression(QString("(?<![a-z])(?:%1)").arg(stems.join('|')), QRegularExpression::CaseInsensitiveOption);
}

/// 2: the name is about the theme, 1: only the description, 0: unrelated (search engines also return popular mods that
/// merely mention the word somewhere)
int themeScore(const QRegularExpression& pattern, const Candidate& candidate)
{
    if (pattern.match(candidate.title).hasMatch() || pattern.match(QString(candidate.slug).replace('-', ' ')).hasMatch())
        return 2;
    if (pattern.match(candidate.description).hasMatch())
        return 1;
    return 0;
}

/// How well a project matches a name the user wrote: 3 exact, 2 starts with it, 1 contains it, 0 no match.
int nameScore(const QString& name, const Candidate& candidate)
{
    const auto wanted = normalized(name);
    const auto title = normalized(candidate.title);
    const auto slug = normalized(candidate.slug);
    if (wanted.isEmpty())
        return 0;
    if (title == wanted || slug == wanted)
        return 3;
    if (title.startsWith(wanted) || slug.startsWith(wanted))
        return 2;
    if (title.contains(wanted) || slug.contains(wanted))
        return 1;
    return 0;
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
    m_triedDependencies.clear();
    m_libraryNames.clear();
    m_foundRequested.clear();
    m_missingRequested.clear();
    m_themeTaken.clear();
    m_emptyThemes.clear();
    m_suggestedSetup.clear();
    m_suggestedSetupMods = 0;
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
    checkMinecraftVersion();
}

void ModpackBuilder::restore(const Plan& plan, const QList<AgentMod>& mods, const QString& loaderVersion)
{
    abort();
    m_plan = plan;
    m_mods = mods;
    m_loaderVersion = loaderVersion;
    m_foundRequested.clear();
    m_missingRequested.clear();
    m_emptyThemes.clear();
    m_suggestedSetup.clear();
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

void ModpackBuilder::checkMinecraftVersion()
{
    // any version works as long as Minecraft has it and the loader supports it
    emit log(tr("Checking that Minecraft %1 exists...").arg(m_plan.minecraftVersion));
    m_minecraftList = APPLICATION->metadataIndex()->get("net.minecraft");
    m_loaderTask = m_minecraftList->getLoadTask();
    connect(m_loaderTask.get(), &Task::succeeded, this, [this] {
        m_loaderTask.reset();
        if (!m_minecraftList->hasVersion(m_plan.minecraftVersion)) {
            m_running = false;
            emit failed(tr("Minecraft %1 does not exist. Check the version number.").arg(m_plan.minecraftVersion));
            return;
        }
        resolveLoaderVersion();
    });
    connect(m_loaderTask.get(), &Task::failed, this, [this](const QString& reason) {
        m_loaderTask.reset();
        m_running = false;
        emit failed(tr("Could not load the list of Minecraft versions: %1").arg(reason));
    });
    m_loaderTask->start();
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
        resolveRequested();
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

QString ModpackBuilder::modrinthFacets() const
{
    if (m_plan.loader == "quilt") {
        return QString(R"([["categories:quilt","categories:fabric"],["versions:%1"],["project_type:mod"]])").arg(m_plan.minecraftVersion);
    }
    return QString(R"([["categories:%1"],["versions:%2"],["project_type:mod"]])").arg(m_plan.loader, m_plan.minecraftVersion);
}

void ModpackBuilder::findByName(const QString& name, bool curseForge, CandidateCallback callback)
{
    if (!curseForge) {
        get(BuildConfig.MODRINTH_PROD_URL + "/search",
            { { "query", name }, { "facets", modrinthFacets() }, { "index", "relevance" }, { "limit", "10" } }, false,
            [name, callback](const QJsonDocument& doc, int status) {
                if (status == 200) {
                    for (const auto& value : doc.object()["hits"].toArray()) {
                        const auto candidate = modrinthCandidate(value.toObject());
                        if (nameScore(name, candidate) == 3) {
                            callback(candidate);
                            return;
                        }
                    }
                }
                callback({});
            });
        return;
    }
    get(BuildConfig.FLAME_BASE_URL + "/mods/search",
        { { "gameId", QString::number(s_cfMinecraftGameId) },
          { "classId", QString::number(s_cfModsClassId) },
          { "searchFilter", name },
          { "gameVersion", m_plan.minecraftVersion },
          { "modLoaderType", QString::number(curseForgeLoaderType()) },
          { "sortField", QString::number(s_cfSortByPopularity) },
          { "sortOrder", "desc" },
          { "pageSize", "10" } },
        true, [name, callback](const QJsonDocument& doc, int status) {
            if (status == 200) {
                for (const auto& value : doc.object()["data"].toArray()) {
                    const auto candidate = curseForgeCandidate(value.toObject());
                    if (nameScore(name, candidate) == 3) {
                        callback(candidate);
                        return;
                    }
                }
            }
            callback({});
        });
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
                const auto versionId = dep["version_id"].toString();
                // some mods name only the version of a dependency; its project is looked up later
                const auto id = !project.isEmpty() ? "mr:" + project : !versionId.isEmpty() ? "mrv:" + versionId : QString();
                if (id.isEmpty()) {
                    continue;
                }
                const auto type = dep["dependency_type"].toString();
                if (type == "required") {
                    choice.required << id;
                } else if (type == "incompatible" && !project.isEmpty()) {
                    choice.incompatible << id;
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
    const int index = indexOfSameMod(candidate);
    return index < 0 ? nullptr : &m_mods.at(index);
}

int ModpackBuilder::indexOfSameMod(const Candidate& candidate) const
{
    const auto name = normalized(candidate.title);
    const auto slug = normalized(candidate.slug);
    for (int i = 0; i < m_mods.size(); i++) {
        const auto& mod = m_mods.at(i);
        if (mod.projectId == candidate.id || mod.aliases.contains(candidate.id)) {
            return i;
        }
        // the same mod published on both platforms
        if ((!name.isEmpty() && normalized(mod.title) == name) || (!slug.isEmpty() && normalized(mod.slug) == slug)) {
            return i;
        }
    }
    return -1;
}

bool ModpackBuilder::aliasExisting(const Candidate& candidate, const QString& requiredId)
{
    const int index = indexOfSameMod(candidate);
    if (index < 0) {
        return false;
    }
    auto& mod = m_mods[index];
    for (const auto& id : { requiredId, candidate.id }) {
        if (mod.projectId != id && !mod.aliases.contains(id)) {
            mod.aliases << id;
        }
    }
    return true;
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
    if (!dependency && isExcluded(m_plan, candidate.title, candidate.slug)) {
        emit log(tr("Skipped %1: you asked for a pack without it.").arg(candidate.title));
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

void ModpackBuilder::resolveRequested()
{
    if (m_plan.requestedMods.isEmpty()) {
        resolveCurated();
        return;
    }
    emit log(tr("Looking for the mods you named: %1...").arg(m_plan.requestedMods.join(", ")));
    m_onPhaseDone = [this] { resolveCurated(); };

    for (const auto& name : m_plan.requestedMods) {
        // both platforms are searched; the best name matches are tried first, Modrinth before CurseForge on a tie
        struct Found {
            int remaining = 0;
            QList<std::pair<int, Candidate>> hits;
        };
        auto found = std::make_shared<Found>();
        found->remaining = (m_useModrinth ? 1 : 0) + (m_useCurseForge ? 1 : 0);
        auto collect = [found](const Candidate& candidate, int score) {
            if (score > 0) {
                found->hits << std::make_pair(score, candidate);
            }
        };
        auto done = [this, name, found] {
            if (--found->remaining > 0) {
                return;
            }
            std::stable_sort(found->hits.begin(), found->hits.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
            QList<Candidate> candidates;
            for (const auto& [score, candidate] : found->hits) {
                candidates << candidate;
            }
            tryRequested(name, candidates);
        };
        if (m_useModrinth) {
            get(BuildConfig.MODRINTH_PROD_URL + "/search",
                { { "query", name }, { "facets", modrinthFacets() }, { "index", "relevance" }, { "limit", "10" } }, false,
                [name, collect, done](const QJsonDocument& doc, int status) {
                    if (status == 200) {
                        for (const auto& value : doc.object()["hits"].toArray()) {
                            const auto candidate = modrinthCandidate(value.toObject());
                            collect(candidate, nameScore(name, candidate));
                        }
                    }
                    done();
                });
        }
        if (m_useCurseForge) {
            get(BuildConfig.FLAME_BASE_URL + "/mods/search",
                { { "gameId", QString::number(s_cfMinecraftGameId) },
                  { "classId", QString::number(s_cfModsClassId) },
                  { "searchFilter", name },
                  { "gameVersion", m_plan.minecraftVersion },
                  { "modLoaderType", QString::number(curseForgeLoaderType()) },
                  { "sortField", QString::number(s_cfSortByPopularity) },
                  { "sortOrder", "desc" },
                  { "pageSize", "10" } },
                true, [name, collect, done](const QJsonDocument& doc, int status) {
                    if (status == 200) {
                        for (const auto& value : doc.object()["data"].toArray()) {
                            const auto candidate = curseForgeCandidate(value.toObject());
                            collect(candidate, nameScore(name, candidate));
                        }
                    }
                    done();
                });
        }
    }
    if (m_pending == 0) {
        phaseDone();
    }
}

void ModpackBuilder::tryRequested(const QString& name, QList<Candidate> candidates)
{
    if (candidates.isEmpty()) {
        m_missingRequested << name;
        emit log(tr("Could not find the mod \"%1\" for Minecraft %2 with %3. I'll look for mods like it.")
                     .arg(name, m_plan.minecraftVersion, loaderDisplayName(m_plan.loader)));
        // maybe it was not a mod name but a theme ("vampires"), so it is searched like one
        m_plan.themes << Theme{ name, { name }, 2 };
        return;
    }
    const auto candidate = candidates.takeFirst();
    if (const auto* same = findSameMod(candidate)) {
        m_foundRequested << same->title;
        return;
    }
    fetchFile(candidate, [this, name, candidates, candidate](const FileChoice& file) {
        if (file.valid && addMod(candidate, file, tr("You asked for"), false)) {
            m_mods.last().requested = true;
            m_foundRequested << candidate.title;
            return;
        }
        if (const auto* same = findSameMod(candidate)) {
            m_foundRequested << same->title;
            return;
        }
        if (!file.problem.isEmpty()) {
            emit log(tr("%1 from %2 does not fit: %3.")
                         .arg(candidate.title, candidate.platform == s_modrinth ? QStringLiteral("Modrinth") : QStringLiteral("CurseForge"),
                              file.problem));
        }
        tryRequested(name, candidates);
    });
}

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
    m_themeTaken = QList<int>(m_plan.themes.size(), 0);
    if (m_plan.themes.isEmpty()) {
        resolveDependencies();
        return;
    }
    QList<int> all;
    for (int i = 0; i < m_plan.themes.size(); i++) {
        all << i;
    }
    runThemeSearch(all, false);
}

void ModpackBuilder::runThemeSearch(const QList<int>& themes, bool wide)
{
    m_onPhaseDone = [this, wide] {
        if (wide) {
            themesDone();
        } else {
            widenThemes();
        }
    };
    const auto facets = modrinthFacets();
    const int limit = wide ? 50 : s_searchLimit;
    const int minimumDownloads = wide ? 100 : s_minimumDownloads;

    for (const int themeIndex : themes) {
        const auto& theme = m_plan.themes.at(themeIndex);
        auto queries = theme.queries;
        if (wide) {
            // the other words of the theme ("undead", "infection") find mods the first searches missed
            for (const auto& word : theme.matchWords) {
                if (queries.size() < 6 && !queries.contains(word, Qt::CaseInsensitive)) {
                    queries << word;
                }
            }
        } else {
            emit log(tr("Searching mods for \"%1\"...").arg(theme.title));
        }
        const auto pattern = themePattern(theme);

        // one result list per query and platform; they are merged when all searches of the theme are back
        struct Collected {
            int remaining = 0;
            QList<QList<std::pair<int, Candidate>>> lists;
        };
        auto collected = std::make_shared<Collected>();
        const int platforms = (m_useModrinth ? 1 : 0) + (m_useCurseForge ? 1 : 0);
        collected->remaining = queries.size() * platforms;
        collected->lists.resize(collected->remaining);

        auto merge = [this, themeIndex, collected, limit] {
            if (--collected->remaining > 0) {
                return;
            }
            // interleave all lists so every search and both platforms contribute their best matches,
            // then put the mods whose name is about the theme first
            QList<std::pair<int, Candidate>> merged;
            for (int rank = 0; rank < limit; rank++) {
                for (const auto& list : collected->lists) {
                    if (rank < list.size()) {
                        merged << list.at(rank);
                    }
                }
            }
            std::stable_sort(merged.begin(), merged.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
            QList<Candidate> candidates;
            for (const auto& [score, candidate] : merged) {
                candidates << candidate;
            }
            pickFromTheme(themeIndex, candidates);
        };

        int listIndex = 0;
        for (const auto& query : queries) {
            if (m_useModrinth) {
                const int slot = listIndex++;
                get(BuildConfig.MODRINTH_PROD_URL + "/search",
                    { { "query", query }, { "facets", facets }, { "index", "relevance" }, { "limit", QString::number(limit) } }, false,
                    [collected, slot, merge, pattern, minimumDownloads](const QJsonDocument& doc, int status) {
                        if (status == 200) {
                            for (const auto& value : doc.object()["hits"].toArray()) {
                                const auto hit = value.toObject();
                                const bool library = hit["categories"].toArray().contains("library");
                                const bool serverOnly = hit["client_side"].toString() == "unsupported";
                                if (library || serverOnly || hit["downloads"].toInteger() < minimumDownloads) {
                                    continue;
                                }
                                const auto candidate = modrinthCandidate(hit);
                                if (const int score = themeScore(pattern, candidate); score > 0) {
                                    collected->lists[slot] << std::make_pair(score, candidate);
                                }
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
                      { "pageSize", QString::number(limit) } },
                    true, [collected, slot, merge, pattern, minimumDownloads](const QJsonDocument& doc, int status) {
                        if (status == 200) {
                            for (const auto& value : doc.object()["data"].toArray()) {
                                const auto mod = value.toObject();
                                if (curseForgeIsLibrary(mod) || mod["downloadCount"].toDouble() < minimumDownloads) {
                                    continue;
                                }
                                const auto candidate = curseForgeCandidate(mod);
                                if (const int score = themeScore(pattern, candidate); score > 0) {
                                    collected->lists[slot] << std::make_pair(score, candidate);
                                }
                            }
                        }
                        merge();
                    });
            }
        }
    }
    if (m_pending == 0) {
        phaseDone();
    }
}

void ModpackBuilder::widenThemes()
{
    QList<int> lacking;
    for (int i = 0; i < m_plan.themes.size(); i++) {
        if (m_themeTaken.value(i) < m_plan.themes.at(i).count) {
            lacking << i;
        }
    }
    if (lacking.isEmpty()) {
        themesDone();
        return;
    }
    QStringList titles;
    for (const int i : lacking) {
        titles << m_plan.themes.at(i).title;
    }
    emit log(tr("Few mods found for: %1. Searching wider...").arg(titles.join(", ")));
    runThemeSearch(lacking, true);
}

void ModpackBuilder::themesDone()
{
    const Theme* firstEmpty = nullptr;
    for (int i = 0; i < m_plan.themes.size(); i++) {
        if (m_themeTaken.value(i) == 0) {
            const auto& theme = m_plan.themes.at(i);
            emit log(tr("Nothing suitable was found for \"%1\".").arg(theme.title));
            m_emptyThemes << theme.title;
            if (!firstEmpty) {
                firstEmpty = &theme;
            }
        }
    }
    if (firstEmpty) {
        suggestSetup(*firstEmpty);
    } else {
        resolveDependencies();
    }
}

void ModpackBuilder::suggestSetup(const Theme& theme)
{
    // look at every version and loader: where are the most mods for this theme?
    m_onPhaseDone = [this] { resolveDependencies(); };
    const auto pattern = themePattern(theme);
    const auto current = m_plan.loader + '|' + m_plan.minecraftVersion;
    get(BuildConfig.MODRINTH_PROD_URL + "/search",
        { { "query", theme.queries.value(0) }, { "facets", R"([["project_type:mod"]])" }, { "index", "relevance" }, { "limit", "100" } },
        false, [this, pattern, current](const QJsonDocument& doc, int status) {
            if (status != 200) {
                return;
            }
            static const QRegularExpression s_release(QStringLiteral("^1\\.\\d+(\\.\\d+)?$"));
            QMap<QString, int> counts;
            for (const auto& value : doc.object()["hits"].toArray()) {
                const auto hit = value.toObject();
                const auto categories = hit["categories"].toArray();
                if (categories.contains("library") || hit["downloads"].toInteger() < s_minimumDownloads ||
                    themeScore(pattern, modrinthCandidate(hit)) == 0) {
                    continue;
                }
                for (const auto* loader : { "neoforge", "forge", "fabric", "quilt" }) {
                    if (!categories.contains(QString(loader))) {
                        continue;
                    }
                    for (const auto& version : hit["versions"].toArray()) {
                        if (s_release.match(version.toString()).hasMatch()) {
                            counts[QString(loader) + '|' + version.toString()]++;
                        }
                    }
                }
            }
            // the most mods wins; on a tie the loader the user chose
            const auto sameLoader = [this](const QString& key) { return key.startsWith(m_plan.loader + '|'); };
            QString best;
            for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
                if (it.key() == current) {
                    continue;
                }
                const int bestCount = counts.value(best);
                if (best.isEmpty() || it.value() > bestCount || (it.value() == bestCount && sameLoader(it.key()) && !sameLoader(best))) {
                    best = it.key();
                }
            }
            if (!best.isEmpty() && counts.value(best) >= 2) {
                const auto parts = best.split('|');
                m_suggestedSetup = loaderDisplayName(parts.at(0)) + ' ' + parts.at(1);
                m_suggestedSetupMods = counts.value(best);
            }
        });
}

void ModpackBuilder::pickFromTheme(int themeIndex, QList<Candidate> candidates)
{
    const auto& theme = m_plan.themes.at(themeIndex);
    while (m_themeTaken.value(themeIndex) < theme.count && !candidates.isEmpty()) {
        const auto candidate = candidates.takeFirst();
        if (m_triedProjects.contains(candidate.id) || findSameMod(candidate)) {
            continue;
        }
        fetchFile(candidate, [this, themeIndex, candidates, candidate](const FileChoice& file) {
            if (addMod(candidate, file, m_plan.themes.at(themeIndex).title, false)) {
                m_themeTaken[themeIndex]++;
            }
            pickFromTheme(themeIndex, candidates);
        });
        return;
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
            if (!present.contains(required) && !m_triedDependencies.contains(required) && !missing.contains(required)) {
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

    for (const auto& requiredId : missing) {
        m_triedDependencies.insert(requiredId);
        if (requiredId.startsWith("mrv:")) {
            // only the version is known: find out which project it belongs to, the next round adds that project
            get(BuildConfig.MODRINTH_PROD_URL + "/version/" + rawId(requiredId), {}, false,
                [this, requiredId](const QJsonDocument& doc, int status) {
                    const auto project = doc.object()["project_id"].toString();
                    if (status != 200 || project.isEmpty()) {
                        return;
                    }
                    for (auto& mod : m_mods) {
                        for (auto& required : mod.requiredProjects) {
                            if (required == requiredId) {
                                required = "mr:" + project;
                            }
                        }
                    }
                });
            continue;
        }
        addLibrary(requiredId);
    }
    if (m_pending == 0) {
        phaseDone();
    }
}

void ModpackBuilder::addLibrary(const QString& requiredId)
{
    findById(requiredId, [this, requiredId](const Candidate& candidate) {
        if (candidate.id.isEmpty()) {
            emit log(tr("A library the mods need (%1) was not found on its platform.").arg(requiredId));
            return;
        }
        m_libraryNames.insert(requiredId, candidate.title);
        // the library may already be in the pack, maybe from the other platform
        if (aliasExisting(candidate, requiredId)) {
            return;
        }
        fetchFile(candidate, [this, requiredId, candidate](const FileChoice& file) {
            if (file.valid && addMod(candidate, file, tr("Libraries"), true)) {
                return;
            }
            if (aliasExisting(candidate, requiredId)) {
                return;
            }
            // no file for this version, or the author blocks downloads outside the CurseForge app: try the other platform
            const auto problem = file.problem.isEmpty() ? tr("no file for this version and loader") : file.problem;
            libraryFromOtherPlatform(requiredId, candidate, problem);
        });
    });
}

void ModpackBuilder::libraryFromOtherPlatform(const QString& requiredId, const Candidate& original, const QString& problem)
{
    const bool curseForge = original.platform == s_modrinth;
    if (curseForge && !m_useCurseForge) {
        emit log(tr("Library %1 is not available: %2.").arg(original.title, problem));
        return;
    }
    const auto otherName = curseForge ? QStringLiteral("CurseForge") : QStringLiteral("Modrinth");
    auto useOther = [this, requiredId, original, problem, otherName](const Candidate& other) {
        if (other.id.isEmpty()) {
            emit log(tr("Library %1 is not available: %2. It is not on %3 either.").arg(original.title, problem, otherName));
            return;
        }
        if (aliasExisting(other, requiredId)) {
            return;
        }
        fetchFile(other, [this, requiredId, original, other, problem, otherName](const FileChoice& file) {
            if (file.valid && addMod(other, file, tr("Libraries"), true)) {
                // mods that asked for the original project now see it as present
                m_mods.last().aliases << requiredId;
                emit log(tr("Library %1 was taken from %2 because %3.").arg(original.title, otherName, problem));
                return;
            }
            if (aliasExisting(other, requiredId)) {
                return;
            }
            emit log(tr("Library %1 is not available: %2. It is not on %3 either.").arg(original.title, problem, otherName));
        });
    };
    // libraries usually keep the same slug on both platforms; if not, the exact name is searched
    findBySlug(original.slug, curseForge, [this, original, curseForge, useOther](const Candidate& bySlug) {
        if (!bySlug.id.isEmpty()) {
            useOther(bySlug);
            return;
        }
        findByName(original.title, curseForge, useOther);
    });
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
                    const auto library = m_libraryNames.value(required);
                    if (library.isEmpty()) {
                        emit log(tr("Removed %1: a library it needs is not available for this version.").arg(m_mods.at(i).title));
                    } else {
                        emit log(tr("Removed %1: it needs %2, which is not available for this version.").arg(m_mods.at(i).title, library));
                    }
                    if (m_mods.at(i).requested) {
                        m_foundRequested.removeAll(m_mods.at(i).title);
                        m_missingRequested << m_mods.at(i).title;
                    }
                    m_mods.removeAt(i);
                    changed = true;
                    break;
                }
            }
        }
    }

    // tell every library which mods need it
    for (auto& library : m_mods) {
        if (!library.dependency) {
            continue;
        }
        library.neededBy.clear();
        for (const auto& mod : m_mods) {
            for (const auto& required : mod.requiredProjects) {
                if ((required == library.projectId || library.aliases.contains(required)) && !library.neededBy.contains(mod.title)) {
                    library.neededBy << mod.title;
                }
            }
        }
    }

    // what the pack is about comes first: the mods the user named, the themes, then optimization and comfort, libraries last
    QStringList order{ tr("You asked for") };
    for (const auto& theme : m_plan.themes) {
        if (!order.contains(theme.title))
            order << theme.title;
    }
    for (const auto& curated : m_plan.curated) {
        if (!order.contains(curated.group))
            order << curated.group;
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
    // libraries are kept only while an enabled mod (directly or through another library) needs them
    QSet<int> included;
    QList<int> queue;
    for (int i = 0; i < m_mods.size(); i++) {
        if (m_mods.at(i).enabled && !m_mods.at(i).dependency) {
            included.insert(i);
            queue << i;
        }
    }
    while (!queue.isEmpty()) {
        const auto& mod = m_mods.at(queue.takeFirst());
        for (const auto& required : mod.requiredProjects) {
            for (int i = 0; i < m_mods.size(); i++) {
                const auto& other = m_mods.at(i);
                if (!included.contains(i) && (other.projectId == required || other.aliases.contains(required))) {
                    included.insert(i);
                    queue << i;
                }
            }
        }
    }

    QJsonArray files;
    for (int i = 0; i < m_mods.size(); i++) {
        const auto& mod = m_mods.at(i);
        if (!included.contains(i)) {
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
