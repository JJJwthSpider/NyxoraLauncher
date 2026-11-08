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
#include <QtEndian>

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

/* ------------------------------------------------------------ the music */

bool musicEnabled()
{
    return settings().value("music", true).toBool();
}

void setMusicEnabled(bool on)
{
    settings().setValue("music", on);
    if (on) {
        startMusic();
    } else {
        pauseMusic();
    }
}

#ifdef Q_OS_WIN
namespace {
HWAVEOUT g_music = nullptr;
WAVEHDR g_header{};
QByteArray g_pcm;
bool g_paused = false;
}  // namespace

void startMusic()
{
    if (!musicEnabled()) {
        return;
    }
    if (g_music) {
        if (g_paused) {
            waveOutRestart(g_music);
            g_paused = false;
        }
        return;
    }
    QFile file(":/sculk_theme/music/vellora_theme.wav");
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    const QByteArray wav = file.readAll();
    // the PCM data chunk of the WAV file (22050 Hz, 16-bit, mono, as sounds.py writes it)
    int at = 12;
    while (at + 8 <= wav.size()) {
        const QByteArray id = wav.mid(at, 4);
        const quint32 size = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(wav.constData() + at + 4));
        if (id == "data") {
            g_pcm = wav.mid(at + 8, int(size));
            break;
        }
        at += 8 + int(size) + (size & 1);
    }
    if (g_pcm.isEmpty()) {
        return;
    }
    WAVEFORMATEX format{};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 1;
    format.nSamplesPerSec = 22050;
    format.wBitsPerSample = 16;
    format.nBlockAlign = 2;
    format.nAvgBytesPerSec = 44100;
    if (waveOutOpen(&g_music, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        g_music = nullptr;
        return;
    }
    waveOutSetVolume(g_music, 0x5000 | (0x5000u << 16));  // about a third: in the background
    g_header = {};
    g_header.lpData = g_pcm.data();
    g_header.dwBufferLength = DWORD(g_pcm.size());
    waveOutPrepareHeader(g_music, &g_header, sizeof(g_header));
    g_header.dwFlags |= WHDR_BEGINLOOP | WHDR_ENDLOOP;  // the whole piece, again and again
    g_header.dwLoops = 0xFFFFFFF;
    waveOutWrite(g_music, &g_header, sizeof(g_header));
    g_paused = false;
}

void pauseMusic()
{
    if (g_music && !g_paused) {
        waveOutPause(g_music);
        g_paused = true;
    }
}
#else
namespace {
/// the system player playing the piece; when it ends, it starts again
class MusicLoop : public QObject {
   public:
    QProcess* process = nullptr;
    bool wanted = false;
    void start()
    {
        wanted = true;
        if (process && process->state() != QProcess::NotRunning) {
            return;
        }
        const auto player = systemPlayer();
        if (player.isEmpty()) {
            return;
        }
        const QString dir = QDir(QDir::tempPath()).filePath("vellora-sounds");
        const QString path = QDir(dir).filePath("vellora_theme.wav");
        if (!QFile::exists(path)) {
            QDir().mkpath(dir);
            QFile::copy(":/sculk_theme/music/vellora_theme.wav", path);
        }
        QStringList args;
        if (player.endsWith("paplay")) {
            args << "--volume=22000";
        } else if (player.endsWith("afplay")) {
            args << "-v" << "0.35";
        } else {
            args << "-q";
        }
        args << path;
        if (!process) {
            process = new QProcess(this);
            connect(process, &QProcess::finished, this, [this] {
                if (wanted) {
                    start();
                }
            });
        }
        process->start(player, args);
    }
    void stop()
    {
        wanted = false;
        if (process && process->state() != QProcess::NotRunning) {
            process->kill();
        }
    }
};
MusicLoop& loop()
{
    static MusicLoop* l = new MusicLoop;
    return *l;
}
}  // namespace

void startMusic()
{
    if (musicEnabled()) {
        loop().start();
    }
}

void pauseMusic()
{
    loop().stop();
}
#endif
}  // namespace VelloraSound
