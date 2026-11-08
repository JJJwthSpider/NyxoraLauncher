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
#include <QStringList>
#include <optional>

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
/// What was chosen with the buttons under the description: it wins over the words. 0 or nullopt is "as the words say,
/// or at random". The same options with the same seed give the same skins, so a click on a colour repaints the variants.
struct Options {
    QRgb clothes = 0, hair = 0, eyes = 0, skin = 0;
    int gender = 0;                     // 0: as said, 1: a boy, 2: a girl
    std::optional<QString> emblem;      // "" is none
    std::optional<QString> hairStyle;   // short, long, spiky, bun, mohawk, bald
    QRgb second = 0;                    // capes: the second colour
    std::optional<QString> pattern;     // capes: gradient, stripes, checker, diagonal, border, galaxy, flames, ice, leaves, rainbow
};
QList<Skin> skins(const QString& prompt, int count, qint64 seed, const Options& options = {});
QList<QImage> capes(const QString& prompt, int count, qint64 seed, const Options& options = {});
/// the emblems a button can choose and their 5x5 pictures ('#' is a pixel)
QStringList emblemIds();
QStringList emblemRows(const QString& id);
/// a description at random, for the 🎲 button: "викинг в синем с бородой", "фиолетовая галактика со звездой"
QString surprise(bool cape);
}  // namespace SkinGen
