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

#include <QObject>
#include <QSet>
#include <QString>

class MinecraftInstance;
class QNetworkReply;

namespace Lobby {

/// What a lobby needs to know about an instance: the Minecraft version and the mod loader (Modrinth name).
struct InstanceInfo {
    QString minecraftVersion;
    QString loader;  // "fabric", "quilt", "forge", "neoforge" or empty for vanilla
    QString loaderName;  // "Fabric", ... for the user
};
InstanceInfo describe(MinecraftInstance* instance);

/// true when the e4mc mod is already in the mods folder of the instance
bool hasTunnelMod(MinecraftInstance* instance);

/// Installs e4mc (https://modrinth.com/mod/e4mc) into an instance: it turns "Open to LAN" into a public address
/// like "brave-creeper.e4mc.link", so friends can join over the internet without port forwarding or an account.
/// The fitting file is taken from Modrinth for the loader and Minecraft version, together with its required
/// dependencies; every file is checked against its SHA-512 hash.
class TunnelModInstaller : public QObject {
    Q_OBJECT
   public:
    explicit TunnelModInstaller(QObject* parent = nullptr) : QObject(parent) {}
    void install(MinecraftInstance* instance);

   signals:
    void status(QString text);
    void finished(bool ok, QString message);

   private:
    void installProject(const QString& project, int depth);
    void download(const QString& url, const QString& fileName, const QString& sha512);
    void oneDone();
    void fail(const QString& message);

    MinecraftInstance* m_instance = nullptr;
    InstanceInfo m_info;
    QSet<QString> m_seen;
    int m_pending = 0;
    bool m_failed = false;
};

}  // namespace Lobby
