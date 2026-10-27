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
#include "ServerManager.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTimer>
#include <QVersionNumber>

#include "Application.h"
#include "BoreTunnel.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "InviteCode.h"
#include "archive/ArchiveReader.h"
#include "java/JavaInstall.h"
#include "java/JavaInstallList.h"
#include "minecraft/MinecraftInstance.h"
#include "settings/SettingsObject.h"

namespace {
// where the server files come from
const QString s_mojangManifest = "https://piston-meta.mojang.com/mc/game/version_manifest_v2.json";
const QString s_fabricMeta = "https://meta.fabricmc.net/v2";
const QString s_neoforgeMaven = "https://maven.neoforged.net/releases/net/neoforged/neoforge";
const QString s_forgeMaven = "https://maven.minecraftforge.net/net/minecraftforge/forge";
const QString s_tunnelHost = "bore.pub";
const quint16 s_tunnelPort = 7835;

// mods that only make sense on the client; a server would refuse to start with some of them
const QStringList s_clientOnlyPrefixes = {
    "sodium",        "embeddium",       "rubidium",   "iris",           "oculus",        "entityculling", "immediatelyfast",
    "dynamic-fps",   "dynamicfps",      "mousetweaks", "mouse-tweaks",  "modmenu",       "zoomify",       "betterf3",
    "continuity",    "lambdynamiclights", "notenoughanimations", "e4mc", "xaerominimap", "xaeros_minimap", "xaeroworldmap",
    "skinlayers3d",  "3dskinlayers",    "presencefootsteps", "cullleaves", "enhancedblockentities", "fpsreducer",
};

int requiredJavaMajor(const QString& minecraftVersion)
{
    const auto version = QVersionNumber::fromString(minecraftVersion);
    if (version >= QVersionNumber(1, 20, 5)) {
        return 21;
    }
    if (version >= QVersionNumber(1, 18)) {
        return 17;
    }
    if (version >= QVersionNumber(1, 17)) {
        return 16;
    }
    return 8;
}

QString localAddress()
{
    for (const auto& address : QNetworkInterface::allAddresses()) {
        const bool privateUse = address.isInSubnet(QHostAddress("10.0.0.0"), 8) || address.isInSubnet(QHostAddress("172.16.0.0"), 12) ||
                                address.isInSubnet(QHostAddress("192.168.0.0"), 16);
        if (address.protocol() == QAbstractSocket::IPv4Protocol && privateUse) {
            return address.toString();
        }
    }
    return {};
}

bool isClientOnlyMod(const QString& path)
{
    const QString name = QFileInfo(path).fileName().toLower();
    for (const auto& prefix : s_clientOnlyPrefixes) {
        if (name.startsWith(prefix)) {
            return true;
        }
    }
    MMCZip::ArchiveReader archive(path);
    if (auto file = archive.goToFile("fabric.mod.json")) {
        return QJsonDocument::fromJson(file->readAll()).object().value("environment").toString() == "client";
    }
    if (auto file = archive.goToFile("quilt.mod.json")) {
        return QJsonDocument::fromJson(file->readAll()).object().value("minecraft").toObject().value("environment").toString() ==
               "client";
    }
    for (const char* toml : { "META-INF/neoforge.mods.toml", "META-INF/mods.toml" }) {
        if (auto file = archive.goToFile(toml)) {
            const QString text = QString::fromUtf8(file->readAll());
            return text.contains(QRegularExpression(R"(clientSideOnly\s*=\s*true)")) ||
                   text.contains(QRegularExpression(R"(displayTest\s*=\s*"IGNORE_ALL_VERSION")"));
        }
    }
    return false;
}

QString escapeProperty(const QString& value)
{
    // server.properties is read like Java properties: everything outside ASCII as \uXXXX
    QString out;
    for (const QChar c : value) {
        if (c.unicode() < 0x20 || c.unicode() > 0x7e) {
            out += QString("\\u%1").arg(c.unicode(), 4, 16, QLatin1Char('0'));
        } else {
            out += c;
        }
    }
    return out;
}
}  // namespace

QJsonObject ServerSettings::toJson() const
{
    return QJsonObject{
        { "instanceId", instanceId },
        { "gamemode", gamemode },
        { "difficulty", difficulty },
        { "pvp", pvp },
        { "hardcore", hardcore },
        { "allowFlight", allowFlight },
        { "commandBlocks", commandBlocks },
        { "whitelist", whitelist },
        { "internet", internet },
        { "maxPlayers", maxPlayers },
        { "viewDistance", viewDistance },
        { "memoryMb", memoryMb },
        { "motd", motd },
        { "seed", seed },
        { "whitelistNames", QJsonArray::fromStringList(whitelistNames) },
    };
}

ServerSettings ServerSettings::fromJson(const QJsonObject& json)
{
    ServerSettings s;
    s.instanceId = json.value("instanceId").toString();
    s.gamemode = json.value("gamemode").toString(s.gamemode);
    s.difficulty = json.value("difficulty").toString(s.difficulty);
    s.pvp = json.value("pvp").toBool(s.pvp);
    s.hardcore = json.value("hardcore").toBool(s.hardcore);
    s.allowFlight = json.value("allowFlight").toBool(s.allowFlight);
    s.commandBlocks = json.value("commandBlocks").toBool(s.commandBlocks);
    s.whitelist = json.value("whitelist").toBool(s.whitelist);
    s.internet = json.value("internet").toBool(s.internet);
    s.maxPlayers = json.value("maxPlayers").toInt(s.maxPlayers);
    s.viewDistance = json.value("viewDistance").toInt(s.viewDistance);
    s.memoryMb = json.value("memoryMb").toInt(s.memoryMb);
    s.motd = json.value("motd").toString();
    s.seed = json.value("seed").toString();
    for (const auto& name : json.value("whitelistNames").toArray()) {
        s.whitelistNames << name.toString();
    }
    return s;
}

ServerManager* ServerManager::instance()
{
    static QPointer<ServerManager> s_instance;
    if (!s_instance) {
        s_instance = new ServerManager(qApp);
    }
    return s_instance;
}

ServerManager::ServerManager(QObject* parent) : QObject(parent)
{
    m_tunnel = new BoreTunnel(this);
    connect(m_tunnel, &BoreTunnel::opened, this, [this](const QString& host, quint16 port) {
        m_publicAddress = QString("%1:%2").arg(host).arg(port);
        m_inviteCode = InviteCode::encode(host, port);
        emit addressesChanged();
        emit status(tr("The server is open for your friends over the internet. Send them the invite code."));
    });
    connect(m_tunnel, &BoreTunnel::failed, this, [this](const QString& reason) {
        if (m_state != State::Running) {
            return;
        }
        m_publicAddress.clear();
        if (!m_lanAddress.isEmpty()) {
            const auto colon = m_lanAddress.lastIndexOf(':');
            m_inviteCode = InviteCode::encode(m_lanAddress.left(colon), quint16(m_lanAddress.mid(colon + 1).toInt()));
        }
        emit addressesChanged();
        emit status(tr("<b>No internet access for the server:</b> %1<br>Friends in the same network can still join with the code.")
                        .arg(reason));
    });
    // the server must not outlive the launcher: stop it and give it a moment to save the world
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this] {
        if (m_process && m_process->state() != QProcess::NotRunning) {
            m_process->write("stop\n");
            if (!m_process->waitForFinished(20000)) {
                m_process->kill();
            }
        }
    });
}

QString ServerManager::serverDir(const QString& instanceId)
{
    return FS::PathCombine(APPLICATION->dataRoot(), "servers", instanceId);
}

ServerSettings ServerManager::loadSettings(const QString& instanceId)
{
    QFile file(FS::PathCombine(serverDir(instanceId), "sculkshoot-server.json"));
    ServerSettings settings;
    if (file.open(QIODevice::ReadOnly)) {
        settings = ServerSettings::fromJson(QJsonDocument::fromJson(file.readAll()).object());
    }
    settings.instanceId = instanceId;
    return settings;
}

void ServerManager::saveSettings(const ServerSettings& settings)
{
    QDir().mkpath(serverDir(settings.instanceId));
    QFile file(FS::PathCombine(serverDir(settings.instanceId), "sculkshoot-server.json"));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(settings.toJson()).toJson());
    }
}

void ServerManager::setState(State state)
{
    m_state = state;
    emit stateChanged(state);
}

void ServerManager::fail(const QString& reason)
{
    emit status(tr("<b>The server could not be started:</b> %1").arg(reason));
    m_tunnel->close();
    setState(State::Stopped);
}

void ServerManager::start(const ServerSettings& settings)
{
    if (isActive()) {
        return;
    }
    m_settings = settings;
    m_instance = dynamic_cast<MinecraftInstance*>(APPLICATION->instances()->getInstanceById(settings.instanceId));
    if (!m_instance) {
        fail(tr("The instance does not exist anymore."));
        return;
    }
    m_info = Lobby::describe(m_instance);
    if (m_info.minecraftVersion.isEmpty()) {
        fail(tr("The Minecraft version of the instance is unknown. Start the instance once and try again."));
        return;
    }
    if (m_info.loader == "quilt") {
        fail(tr("Quilt servers are not supported yet. Make the server from a Fabric, Forge, NeoForge or vanilla instance."));
        return;
    }
    m_dir = serverDir(settings.instanceId);
    QDir().mkpath(m_dir);
    saveSettings(settings);
    m_inviteCode.clear();
    m_publicAddress.clear();
    m_lanAddress.clear();
    m_players.clear();
    emit addressesChanged();
    emit playersChanged(m_players);
    setState(State::Preparing);
    chooseJava();
}

void ServerManager::chooseJava()
{
    const int required = requiredJavaMajor(m_info.minecraftVersion);
    emit status(tr("Looking for Java %1...").arg(required));

    // the Java the launcher picked for this instance when it was played is the best choice
    const QString instanceJava = m_instance->settings()->get("JavaPath").toString();
    auto javas = APPLICATION->javalist();
    auto task = javas->getLoadTask();
    auto pick = [this, javas, required, instanceJava] {
        if (m_state != State::Preparing) {
            return;
        }
        QString exact, newer;
        for (int i = 0; i < javas->count(); i++) {
            auto java = std::dynamic_pointer_cast<JavaInstall>(javas->at(i));
            if (!java) {
                continue;
            }
            if (java->path == instanceJava && java->id.major() >= required) {
                exact = java->path;
                break;
            }
            if (java->id.major() == required && exact.isEmpty()) {
                exact = java->path;
            } else if (required >= 17 && java->id.major() > required && newer.isEmpty()) {
                newer = java->path;
            }
        }
        m_java = !exact.isEmpty() ? exact : newer;
        if (m_java.isEmpty()) {
            fail(tr("Java %1 is needed for Minecraft %2. Play the instance once: the launcher downloads the right Java, then start the "
                    "server again.")
                     .arg(required)
                     .arg(m_info.minecraftVersion));
            return;
        }
        ensureServerFiles();
    };
    if (task) {
        connect(task.get(), &Task::finished, this, pick);
        if (!task->isRunning()) {
            task->start();
        }
    } else {
        pick();
    }
}

void ServerManager::getJson(const QString& url, JsonHandler handler)
{
    QNetworkRequest request{ QUrl(url) };
    request.setHeader(QNetworkRequest::UserAgentHeader, APPLICATION->getUserAgent());
    request.setTransferTimeout(60000);
    auto* reply = APPLICATION->network()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url, handler] {
        reply->deleteLater();
        if (m_state != State::Preparing) {
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            fail(tr("Could not download %1: %2").arg(url, reply->errorString()));
            return;
        }
        handler(QJsonDocument::fromJson(reply->readAll()));
    });
}

void ServerManager::download(const QString& url, const QString& target, const QString& sha1, std::function<void()> done)
{
    emit status(tr("Downloading %1...").arg(QFileInfo(target).fileName()));
    QNetworkRequest request{ QUrl(url) };
    request.setHeader(QNetworkRequest::UserAgentHeader, APPLICATION->getUserAgent());
    request.setTransferTimeout(300000);
    auto* reply = APPLICATION->network()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url, target, sha1, done] {
        reply->deleteLater();
        if (m_state != State::Preparing) {
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            fail(tr("Could not download %1: %2").arg(url, reply->errorString()));
            return;
        }
        const QByteArray data = reply->readAll();
        if (!sha1.isEmpty() && QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex() != sha1.toLatin1()) {
            fail(tr("%1 is damaged (wrong checksum). Try again.").arg(QFileInfo(target).fileName()));
            return;
        }
        QFile out(target);
        if (!out.open(QIODevice::WriteOnly) || out.write(data) != data.size()) {
            fail(tr("Could not save %1.").arg(target));
            return;
        }
        out.close();
        done();
    });
}

void ServerManager::ensureServerFiles()
{
    if (m_state != State::Preparing) {
        return;  // stopped while preparing
    }
    const QString key = QString("%1|%2|%3").arg(m_info.loader, m_info.minecraftVersion, m_info.loaderVersion);
    QFile marker(FS::PathCombine(m_dir, ".sculkshoot-installed"));
    if (marker.open(QIODevice::ReadOnly) && QString::fromUtf8(marker.readAll()) == key && !serverArguments().isEmpty()) {
        syncMods();
        return;
    }
    marker.close();
    auto installed = [this, key] {
        QFile done(FS::PathCombine(m_dir, ".sculkshoot-installed"));
        if (done.open(QIODevice::WriteOnly)) {
            done.write(key.toUtf8());
        }
        syncMods();
    };

    const QString mc = m_info.minecraftVersion;
    const QString loaderVersion = m_info.loaderVersion;
    if (m_info.loader.isEmpty()) {
        emit status(tr("Getting the Minecraft %1 server...").arg(mc));
        getJson(s_mojangManifest, [this, mc, installed](const QJsonDocument& manifest) {
            QString url;
            for (const auto& value : manifest.object().value("versions").toArray()) {
                if (value.toObject().value("id").toString() == mc) {
                    url = value.toObject().value("url").toString();
                }
            }
            if (url.isEmpty()) {
                fail(tr("Mojang has no server for Minecraft %1.").arg(mc));
                return;
            }
            getJson(url, [this, mc, installed](const QJsonDocument& version) {
                const auto server = version.object().value("downloads").toObject().value("server").toObject();
                if (server.isEmpty()) {
                    fail(tr("Mojang has no server for Minecraft %1.").arg(mc));
                    return;
                }
                download(server.value("url").toString(), FS::PathCombine(m_dir, "server.jar"), server.value("sha1").toString(), installed);
            });
        });
    } else if (m_info.loader == "fabric") {
        emit status(tr("Getting the Fabric server for Minecraft %1...").arg(mc));
        getJson(s_fabricMeta + "/versions/installer", [this, mc, loaderVersion, installed](const QJsonDocument& installers) {
            QString installer;
            for (const auto& value : installers.array()) {
                if (value.toObject().value("stable").toBool()) {
                    installer = value.toObject().value("version").toString();
                    break;
                }
            }
            if (installer.isEmpty()) {
                fail(tr("No Fabric installer was found."));
                return;
            }
            download(QString("%1/versions/loader/%2/%3/%4/server/jar").arg(s_fabricMeta, mc, loaderVersion, installer),
                     FS::PathCombine(m_dir, "fabric-server-launch.jar"), QString(), installed);
        });
    } else {
        const bool neoforge = m_info.loader == "neoforge";
        const QString url = neoforge ? QString("%1/%2/neoforge-%2-installer.jar").arg(s_neoforgeMaven, loaderVersion)
                                     : QString("%1/%2-%3/forge-%2-%3-installer.jar").arg(s_forgeMaven, mc, loaderVersion);
        const QString installer = FS::PathCombine(m_dir, "installer.jar");
        download(url, installer, QString(), [this, installer, installed] {
            runInstaller(installer);
            connect(m_process, &QProcess::finished, this, [this, installer, installed](int code) {
                QFile::remove(installer);
                if (m_state != State::Preparing) {
                    return;
                }
                if (code != 0 || serverArguments().isEmpty()) {
                    fail(tr("The %1 installer did not finish (code %2). See the console for details.").arg(m_info.loaderName).arg(code));
                    return;
                }
                installed();
            });
        });
    }
}

void ServerManager::runInstaller(const QString& installer)
{
    emit status(tr("Installing the %1 %2 server, this takes a minute...").arg(m_info.loaderName, m_info.loaderVersion));
    m_process = new QProcess(this);
    m_process->setWorkingDirectory(m_dir);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, &QProcess::readyRead, this, &ServerManager::readOutput);
    connect(m_process, &QProcess::finished, m_process, &QObject::deleteLater);
    m_process->start(m_java, { "-jar", installer, "--installServer" });
}

QStringList ServerManager::serverArguments() const
{
    if (m_info.loader.isEmpty()) {
        return QFile::exists(FS::PathCombine(m_dir, "server.jar")) ? QStringList{ "-jar", "server.jar", "nogui" } : QStringList();
    }
    if (m_info.loader == "fabric") {
        return QFile::exists(FS::PathCombine(m_dir, "fabric-server-launch.jar")) ? QStringList{ "-jar", "fabric-server-launch.jar", "nogui" }
                                                                                   : QStringList();
    }
#ifdef Q_OS_WIN
    const QString argsFile = "win_args.txt";
#else
    const QString argsFile = "unix_args.txt";
#endif
    // modern Forge and NeoForge put their whole command line into an arguments file
    const QString args = m_info.loader == "neoforge"
                             ? QString("libraries/net/neoforged/neoforge/%1/%2").arg(m_info.loaderVersion, argsFile)
                             : QString("libraries/net/minecraftforge/forge/%1-%2/%3").arg(m_info.minecraftVersion, m_info.loaderVersion, argsFile);
    if (QFile::exists(FS::PathCombine(m_dir, args))) {
        return { "@" + args, "nogui" };
    }
    // older Forge: a forge-*.jar next to the server
    for (const auto& jar : QDir(m_dir).entryList({ "forge-*.jar" }, QDir::Files)) {
        if (!jar.contains("installer")) {
            return { "-jar", jar, "nogui" };
        }
    }
    return {};
}

void ServerManager::syncMods()
{
    if (m_state != State::Preparing) {
        return;  // stopped while preparing
    }
    if (!m_info.loader.isEmpty()) {
        emit status(tr("Copying the mods of the instance..."));
        const QString target = FS::PathCombine(m_dir, "mods");
        QDir().mkpath(target);
        // mods copied last time go away first, so mods removed from the instance disappear from the server too
        QFile manifest(FS::PathCombine(m_dir, ".sculkshoot-mods"));
        if (manifest.open(QIODevice::ReadOnly)) {
            for (const auto& name : QString::fromUtf8(manifest.readAll()).split('\n', Qt::SkipEmptyParts)) {
                QFile::remove(FS::PathCombine(target, name));
            }
            manifest.close();
        }
        QStringList copied, skipped;
        for (const auto& info : QDir(m_instance->modsRoot()).entryInfoList({ "*.jar" }, QDir::Files)) {
            if (isClientOnlyMod(info.filePath())) {
                skipped << info.fileName();
                continue;
            }
            const QString to = FS::PathCombine(target, info.fileName());
            QFile::remove(to);
            if (QFile::copy(info.filePath(), to)) {
                copied << info.fileName();
            }
        }
        if (manifest.open(QIODevice::WriteOnly)) {
            manifest.write(copied.join('\n').toUtf8());
        }
        emit logLine(tr("[SculkShoot] Mods on the server: %1. Client-only, left out: %2.")
                         .arg(copied.isEmpty() ? tr("none") : copied.join(", "), skipped.isEmpty() ? tr("none") : skipped.join(", ")));

        // the configs of the instance, without overwriting what the server already has
        const QString configs = FS::PathCombine(m_instance->gameRoot(), "config");
        QDirIterator it(configs, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString from = it.next();
            const QString to = FS::PathCombine(m_dir, "config", QDir(configs).relativeFilePath(from));
            if (!QFile::exists(to)) {
                QDir().mkpath(QFileInfo(to).absolutePath());
                QFile::copy(from, to);
            }
        }
    }
    writeConfig();
}

void ServerManager::writeConfig()
{
    if (m_state != State::Preparing) {
        return;  // stopped while preparing
    }
    // a free port, 25565 if possible
    for (quint16 port = 25565; port < 25600; port++) {
        QTcpServer probe;
        if (probe.listen(QHostAddress::Any, port)) {
            m_port = port;
            break;
        }
    }

    QFile eula(FS::PathCombine(m_dir, "eula.txt"));
    if (eula.open(QIODevice::WriteOnly)) {
        eula.write("# accepted in SculkShoot Client: https://aka.ms/MinecraftEULA\neula=true\n");
    }

    const QList<std::pair<QString, QString>> values = {
        { "server-port", QString::number(m_port) },
        { "online-mode", "true" },
        { "pvp", m_settings.pvp ? "true" : "false" },
        { "gamemode", m_settings.gamemode },
        { "difficulty", m_settings.hardcore ? "hard" : m_settings.difficulty },
        { "hardcore", m_settings.hardcore ? "true" : "false" },
        { "max-players", QString::number(m_settings.maxPlayers) },
        { "allow-flight", m_settings.allowFlight ? "true" : "false" },
        { "enable-command-block", m_settings.commandBlocks ? "true" : "false" },
        { "view-distance", QString::number(m_settings.viewDistance) },
        { "motd", escapeProperty(m_settings.motd.isEmpty() ? tr("SculkShoot server") : m_settings.motd) },
        { "level-seed", escapeProperty(m_settings.seed) },
        { "white-list", m_settings.whitelist ? "true" : "false" },
        { "enforce-whitelist", m_settings.whitelist ? "true" : "false" },
    };
    QFile file(FS::PathCombine(m_dir, "server.properties"));
    QStringList lines;
    if (file.open(QIODevice::ReadOnly)) {
        lines = QString::fromLatin1(file.readAll()).split('\n');
        file.close();
    }
    for (const auto& [key, value] : values) {
        bool found = false;
        for (auto& line : lines) {
            if (line.startsWith(key + "=")) {
                line = key + "=" + value;
                found = true;
            }
        }
        if (!found) {
            lines << key + "=" + value;
        }
    }
    if (file.open(QIODevice::WriteOnly)) {
        file.write(lines.join('\n').toLatin1());
    }
    launch();
}

void ServerManager::launch()
{
    if (m_state != State::Preparing) {
        return;  // stopped while preparing
    }
    const QStringList arguments = serverArguments();
    if (arguments.isEmpty()) {
        fail(tr("The server files are missing."));
        return;
    }
    emit status(tr("Starting the server..."));
    setState(State::Starting);
    m_process = new QProcess(this);
    m_process->setWorkingDirectory(m_dir);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, &QProcess::readyRead, this, &ServerManager::readOutput);
    connect(m_process, &QProcess::finished, this, [this](int code) {
        const bool unexpected = m_state != State::Stopping;
        m_tunnel->close();
        m_players.clear();
        m_inviteCode.clear();
        m_publicAddress.clear();
        m_lanAddress.clear();
        emit playersChanged(m_players);
        emit addressesChanged();
        setState(State::Stopped);
        emit status(unexpected ? tr("<b>The server stopped</b> (code %1). The console below shows why.").arg(code)
                               : tr("The server is stopped. The world is saved."));
        m_process->deleteLater();
    });
    QStringList jvm = { QString("-Xmx%1M").arg(m_settings.memoryMb), QString("-Xms%1M").arg(qMin(1024, m_settings.memoryMb)) };
    m_process->start(m_java, jvm + arguments);
}

void ServerManager::readOutput()
{
    if (!m_process) {
        return;
    }
    m_outputBuffer += m_process->readAll();
    int end;
    while ((end = m_outputBuffer.indexOf('\n')) >= 0) {
        const QString line = QString::fromUtf8(m_outputBuffer.left(end)).trimmed();
        m_outputBuffer.remove(0, end + 1);
        emit logLine(line);
        handleLine(line);
    }
}

void ServerManager::handleLine(const QString& line)
{
    static const QRegularExpression done(R"(: Done \([\d.,]+s\)!)");
    static const QRegularExpression joined(R"(: (\w{2,16}) joined the game)");
    static const QRegularExpression left(R"(: (\w{2,16}) left the game)");
    if (m_state == State::Starting && done.match(line).hasMatch()) {
        setState(State::Running);
        if (m_settings.whitelist) {
            sendCommand("whitelist on");
            for (const auto& name : m_settings.whitelistNames) {
                sendCommand("whitelist add " + name.trimmed());
            }
        }
        openAccess();
        return;
    }
    if (const auto match = joined.match(line); match.hasMatch() && !m_players.contains(match.captured(1))) {
        m_players << match.captured(1);
        emit playersChanged(m_players);
    } else if (const auto match = left.match(line); match.hasMatch()) {
        m_players.removeAll(match.captured(1));
        emit playersChanged(m_players);
    } else if (line.contains("FAILED TO BIND TO PORT", Qt::CaseInsensitive)) {
        emit status(tr("<b>The port %1 is taken.</b> Close the other server and try again.").arg(m_port));
    }
}

void ServerManager::openAccess()
{
    const QString ip = localAddress();
    m_lanAddress = QString("%1:%2").arg(ip.isEmpty() ? QStringLiteral("127.0.0.1") : ip).arg(m_port);
    m_inviteCode = InviteCode::encode(ip.isEmpty() ? QStringLiteral("127.0.0.1") : ip, m_port);
    emit addressesChanged();
    if (m_settings.internet) {
        emit status(tr("The server is running. Opening it to the internet..."));
        m_tunnel->open(m_port, s_tunnelHost, s_tunnelPort);
    } else {
        emit status(tr("The server is running. Friends in the same network join with the invite code."));
    }
}

void ServerManager::stop()
{
    if (!m_process || m_process->state() == QProcess::NotRunning) {
        setState(State::Stopped);
        return;
    }
    if (m_state == State::Preparing) {
        m_process->kill();  // an installer: nothing to save
        setState(State::Stopped);
        return;
    }
    setState(State::Stopping);
    emit status(tr("Stopping the server and saving the world..."));
    sendCommand("stop");
    QPointer<QProcess> process = m_process;
    QTimer::singleShot(30000, this, [process] {
        if (process && process->state() != QProcess::NotRunning) {
            process->kill();
        }
    });
}

void ServerManager::sendCommand(const QString& command)
{
    if (m_process && m_process->state() == QProcess::Running) {
        QString text = command.trimmed();
        if (text.startsWith('/')) {
            text.remove(0, 1);
        }
        m_process->write((text + "\n").toUtf8());
        emit logLine("> " + text);
    }
}
