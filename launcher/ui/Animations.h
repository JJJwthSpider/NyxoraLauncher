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

class QWidget;

/// Small, quick UI animations of the Sculk edition. Every animation cleans up after itself,
/// so widgets end up exactly as they would be without it.
namespace Animations {

/// fades the widget in (opacity 0 -> 1)
void fadeIn(QWidget* widget, int durationMs = 200);

/// a new chat message or list row: grows from zero height while fading in
void growIn(QWidget* widget, int durationMs = 260);

}  // namespace Animations
