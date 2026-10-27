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

#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QStringList>
#include <functional>

#include "modplatform/lobby/TunnelModInstaller.h"

class BoreTunnel;
class MinecraftInstance;

/// What the player chose for their server; saved per instance in the server folder.
struct ServerSettings {
    QString instanceId;
    QString gamemode = "survival";  // survival, creative, adventure
    QString difficulty = "normal";  // peaceful, easy, normal, hard
    bool pvp = true;
    bool hardcore = false;
    bool allowFlight = false;
    bool commandBlocks = false;
    bool whitelist = false;
    bool internet = true;  // open a tunnel so friends can join over the internet
    int maxPlayers = 8;
    int viewDistance = 10;
    int memoryMb = 2048;
    QString motd;
    QString seed;
    QStringList whitelistNames;

    QJsonObject toJson() const;
    static ServerSettings fromJson(const QJsonObject& json);
};

/// Runs one Minecraft server for the friends of the player, made from one of their instances.
///
/// start() goes through the steps one after another: find a Java for the Minecraft version, get the server files
/// (the vanilla server, the Fabric server launcher or the Forge / NeoForge installer), copy the mods of the instance
/// without the client-only ones, write server.properties and the EULA, start the server and, once it is up, open a
/// bore tunnel and make an invite code from the address. The manager lives as long as the launcher, so the server
/// keeps running when its window is closed, and it is stopped cleanly when the launcher quits.
class ServerManager : public QObject {
    Q_OBJECT
   public:
    enum class State { Stopped, Preparing, Starting, Running, Stopping };

    static ServerManager* instance();

    State state() const { return m_state; }
    bool isActive() const { return m_state != State::Stopped; }
    QString instanceId() const { return m_settings.instanceId; }
    QString inviteCode() const { return m_inviteCode; }
    QString publicAddress() const { return m_publicAddress; }
    QString lanAddress() const { return m_lanAddress; }
    QStringList players() const { return m_players; }

    static QString serverDir(const QString& instanceId);
    static ServerSettings loadSettings(const QString& instanceId);
    static void saveSettings(const ServerSettings& settings);

    void start(const ServerSettings& settings);
    void stop();
    void sendCommand(const QString& command);

   signals:
    void stateChanged(ServerManager::State state);
    void status(QString text);
    void logLine(QString line);
    void addressesChanged();
    void playersChanged(QStringList players);

   private:
    explicit ServerManager(QObject* parent = nullptr);

    void setState(State state);
    void fail(const QString& reason);
    void chooseJava();
    void ensureServerFiles();
    void runInstaller(const QString& installer);
    void syncMods();
    void writeConfig();
    void launch();
    void readOutput();
    void handleLine(const QString& line);
    void openAccess();
    QStringList serverArguments() const;

    using JsonHandler = std::function<void(const QJsonDocument&)>;
    void getJson(const QString& url, JsonHandler handler);
    void download(const QString& url, const QString& target, const QString& sha1, std::function<void()> done);

    State m_state = State::Stopped;
    ServerSettings m_settings;
    QPointer<MinecraftInstance> m_instance;
    Lobby::InstanceInfo m_info;
    QString m_dir;
    QString m_java;
    quint16 m_port = 25565;
    QPointer<QProcess> m_process;
    QByteArray m_outputBuffer;
    BoreTunnel* m_tunnel = nullptr;
    QString m_inviteCode;
    QString m_publicAddress;
    QString m_lanAddress;
    QStringList m_players;
};
