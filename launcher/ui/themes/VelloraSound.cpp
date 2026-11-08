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
#include "VelloraSound.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QHash>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>

#include "Application.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <mmsystem.h>
#endif

namespace {
QSettings& settings()
{
    static QSettings s(QDir(APPLICATION->dataRoot()).filePath("vellora-ui.ini"), QSettings::IniFormat);
    return s;
}

/// the WAV file in the resources, kept in memory (PlaySound reads it from there while it plays)
const QByteArray& wav(const QString& name)
{
    static QHash<QString, QByteArray> cache;
    if (!cache.contains(name)) {
        QFile file(":/sculk_theme/sounds/" + name + ".wav");
        cache.insert(name, file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray());
    }
    return cache[name];
}

#ifndef Q_OS_WIN
/// the sound as a file in the temporary folder, for the system player
QString wavFile(const QString& name)
{
    const QString dir = QDir(QDir::tempPath()).filePath("vellora-sounds");
    const QString path = QDir(dir).filePath(name + ".wav");
    if (!QFile::exists(path)) {
        QDir().mkpath(dir);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(wav(name)) < 0) {
            return {};
        }
    }
    return path;
}

QString systemPlayer()
{
    static const QString player = [] {
        for (const auto* name : { "paplay", "afplay", "aplay" }) {
            const auto found = QStandardPaths::findExecutable(name);
            if (!found.isEmpty()) {
                return found;
            }
        }
        return QString();
    }();
    return player;
}
#endif
}  // namespace

namespace VelloraSound {
bool enabled()
{
    return settings().value("sounds", true).toBool();
}

void setEnabled(bool on)
{
    settings().setValue("sounds", on);
}

void play(const QString& name)
{
    if (!enabled()) {
        return;
    }
    // the same sound twice within a moment (one click that opens and switches) is played once
    static QElapsedTimer timer;
    static QString last;
    if (name == last && timer.isValid() && timer.elapsed() < 60) {
        return;
    }
    last = name;
    timer.restart();
    const auto& data = wav(name);
    if (data.isEmpty()) {
        return;
    }
#ifdef Q_OS_WIN
    PlaySoundW(reinterpret_cast<LPCWSTR>(data.constData()), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
#else
    const auto player = systemPlayer();
    const auto file = wavFile(name);
    if (player.isEmpty() || file.isEmpty()) {
        return;
    }
    QStringList args;
    if (player.endsWith("aplay")) {
        args << "-q";
    }
    args << file;
    QProcess::startDetached(player, args);
#endif
}
}  // namespace VelloraSound
