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

#include <QMap>
#include <QString>

class BaseInstance;
class MinecraftInstance;

/// The player's look in the game: the Nyxora Cosmetics mod (cosmetics-mod/, Fabric) shows the skin, the cape and the
/// accessories of the head, body and back slots from the launcher on the player of the game. Before every start the
/// launcher puts the mod into a Fabric or Quilt instance of Minecraft 1.20.1 and writes the look into
/// config/vellora-cosmetics/; from any other instance the mod is taken out (Fabric would refuse to start with it).
namespace VelloraLook {
/// the Minecraft version the mod is made for
inline const QString MinecraftVersion = "1.20.1";
/// the mod's file in the mods folder
inline const QString ModFile = "vellora-cosmetics-1.20.1.jar";

/// the components of an instance (uid -> version): from the loaded profile, or from the instance's mmc-pack.json
QMap<QString, QString> components(MinecraftInstance* instance);
/// whether the mod fits an instance with these components (Fabric or Quilt on Minecraft 1.20.1)
bool fits(const QMap<QString, QString>& components);
/// before a start: the mod and the look in the instance, or the mod out of an instance it does not fit
void prepare(BaseInstance* instance);
}  // namespace VelloraLook
