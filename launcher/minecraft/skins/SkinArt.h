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
#include <QPoint>
#include <QRect>
#include <QString>
#include <QStringList>

/// Everything the skin and cape editor knows about Minecraft textures: where each face of the
/// body (or the cape) lies on the texture, which pixel mirrors which, and generators that paint
/// a whole skin or cape in a chosen style.
namespace SkinArt {

enum class Side { Top, Bottom, Right, Front, Left, Back };

/// One face of a box on the texture, e.g. the face of the head or the back of the left sleeve.
struct Face {
    int box = 0;          // index into boxes()
    Side side = Side::Front;
    QRect rect;           // pixels on the texture
    bool overlay = false; // second layer (hat, jacket, sleeves, trousers) or a part of the elytra
};

/// A box of the model as it is unfolded on the texture (Minecraft's usual layout).
struct Box {
    QString name;    // translated
    QPoint origin;   // top left corner of the unfolded box
    int width;       // x size
    int height;      // y size
    int depth;       // z size
    bool overlay;
    int mirror;      // index of the box on the other side of the body, -1 for head and body (they mirror onto themselves)
};

QList<Box> skinBoxes(bool slim);
QList<Box> capeBoxes();

/// All faces of the boxes.
QList<Face> faces(const QList<Box>& boxes);

/// The face under a texture pixel, or nullptr.
const Face* faceAt(const QList<Face>& faces, QPoint pixel);

/// "Head — face", "Left sleeve (2nd layer) — back" ...
QString describe(const QList<Box>& boxes, const Face& face);
QString sideName(Side side);

/// The pixel that mirrors `pixel` on the other side of the body (or of the same part), or (-1, -1).
QPoint mirrorPixel(const QList<Box>& boxes, const QList<Face>& faces, QPoint pixel);

/// Styles the generators know; the first one picks a random look.
QStringList skinStyles();
QStringList capeStyles();

/// A new 64x64 skin in a style (index into skinStyles()); the same seed gives the same skin.
QImage generateSkin(int style, quint32 seed, bool slim);

/// A new 64x32 cape with matching elytra in a style (index into capeStyles()).
QImage generateCape(int style, quint32 seed);

/// An empty texture of the right size (all transparent).
QImage blankSkin();
QImage blankCape();

}  // namespace SkinArt
