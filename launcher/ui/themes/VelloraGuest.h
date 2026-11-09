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

#include <QImage>
#include <QString>

class QWidget;

/// The guest of Vellora Client, like Millida's: a nickname without a Microsoft account. A guest uses the launcher
/// (instances, mods, the catalog, the wardrobe), but not the AI (Arkan's agent, the AI skin and cape generators); the
/// game itself checks the purchase through the Microsoft account, as Prism Launcher always did.
namespace VelloraGuest {
/// the AI works with a Microsoft account in the list
bool aiAllowed();
/// the guest's nickname (empty: not a guest yet)
QString name();
void setName(const QString& name);
/// "Player2773"
QString randomName();
/// 3 to 16 letters, digits or "_"
bool validName(const QString& name);
/// the guest's look: a skin made from the nickname, the same every time
QImage skin(bool* slim = nullptr);
/// asks for a nickname and makes the player a guest; true when done
bool askName(QWidget* parent);
/// for a guest: explains that the AI needs a Microsoft account and offers the sign-in; true when the AI may be used
bool requireAi(QWidget* parent);
}  // namespace VelloraGuest
