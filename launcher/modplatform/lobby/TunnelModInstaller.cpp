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
#include "TunnelModInstaller.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrlQuery>

#include "Application.h"
#include "BuildConfig.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "net/Mode.h"

namespace Lobby {

InstanceInfo describe(MinecraftInstance* instance)
{
    InstanceInfo info;
    auto* profile = instance->getPackProfile();
    info.minecraftVersion = profile->getComponentVersion("net.minecraft");
    if (info.minecraftVersion.isEmpty()) {
        profile->reload(Net::Mode::Offline);
        info.minecraftVersion = profile->getComponentVersion("net.minecraft");
    }
    static const struct {
        const char* uid;
        const char* loader;
        const char* name;
    } loaders[] = {
        { "net.neoforged", "neoforge", "NeoForge" },
        { "net.minecraftforge", "forge", "Forge" },
        { "net.fabricmc.fabric-loader", "fabric", "Fabric" },
        { "org.quiltmc.quilt-loader", "quilt", "Quilt" },
    };
    for (const auto& l : loaders) {
        if (!profile->getComponentVersion(l.uid).isEmpty()) {
            info.loader = l.loader;
            info.loaderName = l.name;
            info.loaderVersion = profile->getComponentVersion(l.uid);
            break;
        }
    }
    return info;
}

bool hasTunnelMod(MinecraftInstance* instance)
{
    const QDir mods(instance->modsRoot());
    for (const auto& file : mods.entryList({ "*.jar" }, QDir::Files)) {
        if (file.startsWith("e4mc", Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

void TunnelModInstaller::install(MinecraftInstance* instance)
{
    m_instance = instance;
    m_info = describe(instance);
    m_seen.clear();
    m_pending = 0;
    m_failed = false;
    if (m_info.loader.isEmpty() || m_info.minecraftVersion.isEmpty()) {
        fail(tr("The instance has no mod loader, so the internet lobby mod can't be installed."));
        return;
    }
    QDir().mkpath(instance->modsRoot());
    emit status(tr("Looking for e4mc for %1 %2...").arg(m_info.loaderName, m_info.minecraftVersion));
    installProject("e4mc", 0);
}

void TunnelModInstaller::installProject(const QString& project, int depth)
{
    if (m_failed || m_seen.contains(project) || depth > 3) {
        return;
    }
    m_seen.insert(project);
    m_pending++;

    QUrl url(BuildConfig.MODRINTH_PROD_URL + "/project/" + project + "/version");
    QUrlQuery query;
    query.addQueryItem("loaders", QString("[\"%1\"]").arg(m_info.loader));
    query.addQueryItem("game_versions", QString("[\"%1\"]").arg(m_info.minecraftVersion));
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, APPLICATION->getUserAgent());
    request.setTransferTimeout(60000);
    auto* reply = APPLICATION->network()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, project, depth] {
        reply->deleteLater();
        if (m_failed) {
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            fail(tr("Modrinth could not be reached: %1").arg(reply->errorString()));
            return;
        }
        const auto versions = QJsonDocument::fromJson(reply->readAll()).array();
        if (versions.isEmpty()) {
            fail(project == "e4mc" ? tr("e4mc has no version for %1 %2 yet.").arg(m_info.loaderName, m_info.minecraftVersion)
                                   : tr("A library e4mc needs has no version for %1 %2.").arg(m_info.loaderName, m_info.minecraftVersion));
            return;
        }
        const auto version = versions.first().toObject();  // Modrinth lists the newest version first
        QJsonObject file;
        for (const auto& value : version.value("files").toArray()) {
            if (file.isEmpty() || value.toObject().value("primary").toBool()) {
                file = value.toObject();
            }
        }
        for (const auto& value : version.value("dependencies").toArray()) {
            const auto dependency = value.toObject();
            if (dependency.value("dependency_type").toString() == "required" && !dependency.value("project_id").toString().isEmpty()) {
                installProject(dependency.value("project_id").toString(), depth + 1);
            }
        }
        if (project != "e4mc" && QFile::exists(QDir(m_instance->modsRoot()).filePath(file.value("filename").toString()))) {
            oneDone();  // the library is already there
            return;
        }
        download(file.value("url").toString(), file.value("filename").toString(), file.value("hashes").toObject().value("sha512").toString());
    });
}

void TunnelModInstaller::download(const QString& url, const QString& fileName, const QString& sha512)
{
    emit status(tr("Downloading %1...").arg(fileName));
    QNetworkRequest request{ QUrl(url) };
    request.setHeader(QNetworkRequest::UserAgentHeader, APPLICATION->getUserAgent());
    request.setTransferTimeout(120000);
    auto* reply = APPLICATION->network()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, fileName, sha512] {
        reply->deleteLater();
        if (m_failed) {
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            fail(tr("Could not download %1: %2").arg(fileName, reply->errorString()));
            return;
        }
        const QByteArray data = reply->readAll();
        if (!sha512.isEmpty() && QCryptographicHash::hash(data, QCryptographicHash::Sha512).toHex() != sha512.toLatin1()) {
            fail(tr("%1 is damaged (wrong checksum). Try again.").arg(fileName));
            return;
        }
        QFile out(QDir(m_instance->modsRoot()).filePath(fileName));
        if (!out.open(QIODevice::WriteOnly) || out.write(data) != data.size()) {
            fail(tr("Could not save %1 to the mods folder.").arg(fileName));
            return;
        }
        oneDone();
    });
}

void TunnelModInstaller::oneDone()
{
    if (--m_pending == 0 && !m_failed) {
        emit finished(true, tr("e4mc is installed."));
    }
}

void TunnelModInstaller::fail(const QString& message)
{
    if (m_failed) {
        return;
    }
    m_failed = true;
    emit finished(false, message);
}

}  // namespace Lobby
