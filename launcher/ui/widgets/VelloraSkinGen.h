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
#include <QList>
#include <QString>

/// Arkan's skin and cape generator, the same as the Android app's (SkinGenerator.java): a description in words becomes
/// Minecraft skins (64x64, both layers, slim arms for girls) and capes (64x32): the look (knight, wizard, ninja, pirate,
/// princess, astronaut, demon, cyber...), the colours of clothes, hair, eyes and skin, an emblem, a cape pattern. Works
/// without internet; a new seed gives new variants.
namespace SkinGen {
struct Skin {
    QImage image;
    bool slim = false;
    QString title;
};
QList<Skin> skins(const QString& prompt, int count, qint64 seed);
QList<QImage> capes(const QString& prompt, int count, qint64 seed);
}  // namespace SkinGen
