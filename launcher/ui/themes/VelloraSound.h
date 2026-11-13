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

#include <QString>

/// The interface sounds of Nyxora Launcher, like Millida Launcher's: "nav" when a tab changes, "open" and "close" for
/// windows, "click", "toggle", "wear" when an item is put on and "launch" on Play. The sounds are Nyxora's own
/// (android/art/sounds.py, in the resources). Windows plays them from memory with PlaySound, Linux and macOS with the
/// system player (paplay, aplay or afplay) if there is one. The 🔊 button in the lobby turns them off. The music loops
/// with waveOut on Windows (the sounds play over it) and with the system player elsewhere.
namespace VelloraSound {
void play(const QString& name);
bool enabled();
void setEnabled(bool on);

/// the music: "Crimson Rain" (android/art/sounds.py), a calm loop, quiet; it pauses while a game runs. 🎵 turns it off.
bool musicEnabled();
void setMusicEnabled(bool on);
void startMusic();
void pauseMusic();
}  // namespace VelloraSound
