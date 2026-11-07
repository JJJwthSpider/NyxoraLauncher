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

#include "SkinArt.h"

#include <QColor>
#include <QCoreApplication>
#include <functional>
#include <random>

namespace SkinArt {

namespace {

QString tr(const char* text)
{
    return QCoreApplication::translate("SkinArt", text);
}

// indices of skinBoxes()
enum SkinBox { Head, Hat, Body, Jacket, RightArm, RightSleeve, LeftArm, LeftSleeve, RightLeg, RightPants, LeftLeg, LeftPants };
// indices of capeBoxes()
enum CapeBox { Cape, Elytra };

QList<Face> boxFaces(int index, const Box& b)
{
    const int x = b.origin.x();
    const int y = b.origin.y();
    const int w = b.width;
    const int h = b.height;
    const int d = b.depth;
    return {
        { index, Side::Top, QRect(x + d, y, w, d), b.overlay },
        { index, Side::Bottom, QRect(x + d + w, y, w, d), b.overlay },
        { index, Side::Right, QRect(x, y + d, d, h), b.overlay },
        { index, Side::Front, QRect(x + d, y + d, w, h), b.overlay },
        { index, Side::Left, QRect(x + d + w, y + d, d, h), b.overlay },
        { index, Side::Back, QRect(x + 2 * d + w, y + d, w, h), b.overlay },
    };
}

const Face* findFace(const QList<Face>& all, int box, Side side)
{
    for (const auto& face : all) {
        if (face.box == box && face.side == side) {
            return &face;
        }
    }
    return nullptr;
}

/* ---------------------------------------------------------------- painting helpers */

class Brush {
   public:
    Brush(QImage& image, quint32 seed) : m_image(image), m_rng(seed) {}

    int random(int from, int to) { return std::uniform_int_distribution<int>(from, to)(m_rng); }
    bool chance(double p) { return std::uniform_real_distribution<double>(0, 1)(m_rng) < p; }

    /// the color a little lighter or darker, like the noise on Minecraft's own textures
    QColor vary(const QColor& c, int amount)
    {
        if (amount <= 0) {
            return c;
        }
        const int factor = 100 + random(-amount, amount);
        return factor >= 100 ? c.lighter(factor) : c.darker(10000 / factor);
    }

    void dot(int x, int y, const QColor& c, int noise = 6)
    {
        if (QRect(0, 0, m_image.width(), m_image.height()).contains(x, y)) {
            m_image.setPixelColor(x, y, c.alpha() == 0 ? c : vary(c, noise));
        }
    }

    /// paints a face pixel by pixel; the function gets the position inside the face and returns
    /// the color (an invalid color leaves the pixel as it is)
    void face(const Face* f, const std::function<QColor(int lx, int ly, int w, int h)>& color, int noise = 6)
    {
        if (!f) {
            return;
        }
        for (int ly = 0; ly < f->rect.height(); ly++) {
            for (int lx = 0; lx < f->rect.width(); lx++) {
                const auto c = color(lx, ly, f->rect.width(), f->rect.height());
                if (c.isValid()) {
                    dot(f->rect.x() + lx, f->rect.y() + ly, c, noise);
                }
            }
        }
    }

    void fill(const Face* f, const QColor& c, int noise = 6)
    {
        face(f, [c](int, int, int, int) { return c; }, noise);
    }

    QImage& image() { return m_image; }

   private:
    QImage& m_image;
    std::mt19937 m_rng;
};

/// the colors of a person
struct Look {
    QColor skin, hair, eyes, shirt, trim, pants, shoes, mouth;
    int hairLength = 1;  // 0 bald, 1 short, 2 long
    bool longSleeves = false;
    bool beard = false;
};

const QColor s_skinTones[] = { QColor(0xf3, 0xd2, 0xb3), QColor(0xe0, 0xac, 0x8a), QColor(0xc6, 0x8a, 0x62), QColor(0x9c, 0x63, 0x3e),
                               QColor(0x6e, 0x43, 0x2a), QColor(0xf6, 0xe0, 0xcf) };
const QColor s_hairColors[] = { QColor(0x2b, 0x1d, 0x14), QColor(0x5a, 0x3a, 0x22), QColor(0xb8, 0x86, 0x3b), QColor(0xe8, 0xd3, 0x8c),
                                QColor(0x8c, 0x2f, 0x1c), QColor(0x15, 0x15, 0x18), QColor(0x6d, 0x6d, 0x72) };
const QColor s_eyeColors[] = { QColor(0x3b, 0x5d, 0xa8), QColor(0x4f, 0x8a, 0x3b), QColor(0x5a, 0x3a, 0x22), QColor(0x2a, 0x2a, 0x2a),
                               QColor(0x6a, 0x4b, 0x9a) };
const QColor s_clothes[] = { QColor(0x2f, 0x9e, 0xa8), QColor(0xb0, 0x3a, 0x2e), QColor(0x3a, 0x6b, 0x35), QColor(0x2f, 0x4a, 0x7a),
                             QColor(0x7a, 0x4a, 0x9a), QColor(0xd8, 0xa0, 0x2c), QColor(0x55, 0x55, 0x5c), QColor(0xe8, 0xe8, 0xe8),
                             QColor(0x1b, 0x1b, 0x1f), QColor(0x8a, 0x5a, 0x34) };

template <typename T, size_t N>
const T& pick(Brush& brush, const T (&array)[N])
{
    return array[brush.random(0, int(N) - 1)];
}

/// the plain body every style starts from: skin, hair, face, shirt, trousers, shoes
void paintPerson(Brush& b, const QList<Face>& all, const Look& look)
{
    // head
    b.fill(findFace(all, Head, Side::Top), look.hairLength > 0 ? look.hair : look.skin);
    b.fill(findFace(all, Head, Side::Bottom), look.skin);
    for (auto side : { Side::Right, Side::Left, Side::Back }) {
        const int hairRows = side == Side::Back ? (look.hairLength == 2 ? 7 : 4) : (look.hairLength == 2 ? 5 : 2);
        b.face(findFace(all, Head, side), [&](int, int ly, int, int) {
            return look.hairLength > 0 && ly < hairRows ? look.hair : look.skin;
        });
    }
    b.face(findFace(all, Head, Side::Front), [&](int lx, int ly, int, int) -> QColor {
        if (look.hairLength > 0 && (ly < 2 || (look.hairLength == 2 && ly < 5 && (lx == 0 || lx == 7)))) {
            return look.hair;
        }
        if (ly == 4) {
            if (lx == 1 || lx == 6)
                return QColor(0xf4, 0xf4, 0xf4);
            if (lx == 2 || lx == 5)
                return look.eyes;
        }
        if (ly == 3 && (lx == 1 || lx == 2 || lx == 5 || lx == 6) && look.hairLength > 0) {
            return look.hair.darker(110);  // eyebrows
        }
        if (ly == 5 && (lx == 3 || lx == 4)) {
            return look.skin.darker(118);  // nose
        }
        if (look.beard && ly >= 5 && !(ly == 5 && lx >= 2 && lx <= 5)) {
            return look.hair;
        }
        if (ly == 6 && lx >= 3 && lx <= 4) {
            return look.mouth;
        }
        return look.skin;
    });

    // body: shirt with the waist of the trousers at the bottom
    for (auto side : { Side::Front, Side::Back, Side::Right, Side::Left }) {
        b.face(findFace(all, Body, side), [&](int lx, int ly, int w, int h) -> QColor {
            if (ly >= h - 2)
                return look.pants;
            if (side == Side::Front && ly == 0 && (lx == w / 2 - 1 || lx == w / 2))
                return look.skin;  // collar
            if (ly == h - 3)
                return look.trim;  // belt
            return look.shirt;
        });
    }
    b.fill(findFace(all, Body, Side::Top), look.shirt);
    b.fill(findFace(all, Body, Side::Bottom), look.pants);

    // arms: sleeves and hands
    const int sleeve = look.longSleeves ? 10 : 4;
    for (auto arm : { RightArm, LeftArm }) {
        for (auto side : { Side::Front, Side::Back, Side::Right, Side::Left }) {
            b.face(findFace(all, arm, side), [&](int, int ly, int, int) { return ly < sleeve ? look.shirt : look.skin; });
        }
        b.fill(findFace(all, arm, Side::Top), look.shirt);
        b.fill(findFace(all, arm, Side::Bottom), look.skin);
    }

    // legs: trousers and shoes
    for (auto leg : { RightLeg, LeftLeg }) {
        for (auto side : { Side::Front, Side::Back, Side::Right, Side::Left }) {
            b.face(findFace(all, leg, side), [&](int, int ly, int, int h) { return ly >= h - 3 ? look.shoes : look.pants; });
        }
        b.fill(findFace(all, leg, Side::Top), look.pants);
        b.fill(findFace(all, leg, Side::Bottom), look.shoes);
    }
}

/// glowing sculk dots and veins on the second layer
void paintSculkGlow(Brush& b, const QList<Face>& all, double density)
{
    const QColor glow(0x29, 0xdf, 0xeb);
    const QColor dim(0x12, 0x8a, 0x95);
    for (const auto& face : all) {
        if (!face.overlay || face.box == Hat) {
            continue;
        }
        b.face(&face, [&](int, int, int, int) -> QColor {
            if (b.chance(density))
                return glow;
            if (b.chance(density * 0.6))
                return dim;
            return {};
        }, 10);
    }
    // a vein that runs down the chest like the Warden's ribs
    if (const auto* chest = findFace(all, Jacket, Side::Front)) {
        int x = chest->rect.x() + b.random(2, 5);
        for (int y = chest->rect.y() + 1; y < chest->rect.bottom() - 1; y++) {
            b.dot(x, y, glow, 8);
            x = std::clamp(x + b.random(-1, 1), chest->rect.x() + 1, chest->rect.right() - 1);
        }
    }
}

/// the second layer of the head as a hood or helmet; `face` tells which pixels of its front stay open
void paintHood(Brush& b, const QList<Face>& all, const QColor& color, const std::function<bool(int lx, int ly)>& open)
{
    for (auto side : { Side::Top, Side::Right, Side::Left, Side::Back }) {
        b.fill(findFace(all, Hat, side), color, 8);
    }
    b.face(findFace(all, Hat, Side::Front), [&](int lx, int ly, int, int) -> QColor { return open(lx, ly) ? QColor() : color; }, 8);
}

QImage paintSkin(int style, quint32 seed, bool slim)
{
    QImage image = blankSkin();
    Brush b(image, seed);
    const auto all = faces(skinBoxes(slim));

    Look look;
    look.skin = pick(b, s_skinTones);
    look.hair = pick(b, s_hairColors);
    look.eyes = pick(b, s_eyeColors);
    look.shirt = pick(b, s_clothes);
    look.trim = pick(b, s_clothes);
    look.pants = pick(b, s_clothes).darker(130);
    look.shoes = QColor(0x3a, 0x2a, 0x1e);
    look.mouth = look.skin.darker(150);
    look.hairLength = b.random(0, 9) == 0 ? 0 : b.random(1, 2);
    look.longSleeves = b.chance(0.4);
    look.beard = b.chance(0.12);

    switch (style) {
        case 1: {  // sculk
            look.shirt = QColor(0x10, 0x34, 0x3b);
            look.trim = QColor(0x29, 0xdf, 0xeb);
            look.pants = QColor(0x0a, 0x23, 0x28);
            look.shoes = QColor(0x05, 0x13, 0x16);
            look.eyes = QColor(0x29, 0xdf, 0xeb);
            look.skin = QColor(0xd9, 0xc8, 0xb8);
            look.mouth = look.skin.darker(150);
            look.hair = QColor(0x10, 0x16, 0x18);
            look.hairLength = 1;
            look.beard = false;
            look.longSleeves = true;
            paintPerson(b, all, look);
            paintHood(b, all, QColor(0x0d, 0x2c, 0x33), [](int, int ly) { return ly >= 2; });
            paintSculkGlow(b, all, 0.025);
            break;
        }
        case 2: {  // knight
            look.shirt = QColor(0x8e, 0x95, 0x9c);
            look.trim = QColor(0xd8, 0xa0, 0x2c);
            look.pants = QColor(0x6f, 0x75, 0x7c);
            look.shoes = QColor(0x4a, 0x4f, 0x55);
            look.longSleeves = true;
            paintPerson(b, all, look);
            const QColor steel(0xa7, 0xad, 0xb3);
            paintHood(b, all, steel, [](int, int) { return false; });
            b.face(findFace(all, Hat, Side::Front), [](int lx, int ly, int, int) -> QColor {
                return ly == 4 && lx >= 1 && lx <= 6 ? QColor(0x20, 0x20, 0x24) : QColor();  // visor slit
            }, 0);
            // a red tabard with a golden stripe over the armor
            b.face(findFace(all, Jacket, Side::Front), [](int lx, int ly, int w, int h) -> QColor {
                if (ly >= h - 3)
                    return {};
                return lx == w / 2 - 1 || lx == w / 2 ? QColor(0xd8, 0xa0, 0x2c) : QColor(0xa8, 0x23, 0x23);
            });
            b.face(findFace(all, Jacket, Side::Back), [](int, int ly, int, int h) -> QColor {
                return ly < h - 3 ? QColor(0xa8, 0x23, 0x23) : QColor();
            });
            for (auto box : { RightSleeve, LeftSleeve, RightPants, LeftPants }) {
                for (auto side : { Side::Front, Side::Back, Side::Right, Side::Left, Side::Top }) {
                    b.face(findFace(all, box, side), [&](int, int ly, int, int) -> QColor {
                        return (box == RightSleeve || box == LeftSleeve) && ly > 9 ? QColor() : steel;
                    }, 8);
                }
            }
            break;
        }
        case 3: {  // ninja
            look.shirt = QColor(0x1b, 0x1b, 0x1f);
            look.trim = QColor(0xb0, 0x1e, 0x1e);
            look.pants = QColor(0x16, 0x16, 0x1a);
            look.shoes = QColor(0x0e, 0x0e, 0x10);
            look.longSleeves = true;
            paintPerson(b, all, look);
            // a mask that leaves only the eyes, with a red headband
            paintHood(b, all, QColor(0x1b, 0x1b, 0x1f), [](int, int ly) { return ly == 3 || ly == 4; });
            for (auto side : { Side::Front, Side::Right, Side::Left, Side::Back }) {
                b.face(findFace(all, Hat, side), [](int, int ly, int, int) { return ly == 2 ? QColor(0xb0, 0x1e, 0x1e) : QColor(); });
            }
            b.face(findFace(all, Hat, Side::Back), [](int lx, int ly, int, int) {
                return ly >= 2 && ly <= 4 && (lx == 3 || lx == 4) ? QColor(0xb0, 0x1e, 0x1e) : QColor();  // knot
            });
            break;
        }
        case 4: {  // miner
            look.shirt = QColor(0x44, 0x50, 0x5c);  // a dark work jacket
            look.trim = QColor(0xe3, 0x7b, 0x1c);
            look.pants = QColor(0x2f, 0x4a, 0x7a);
            paintPerson(b, all, look);
            const QColor helmet(0xe3, 0xb8, 0x22);
            for (auto side : { Side::Top, Side::Right, Side::Left, Side::Back, Side::Front }) {
                b.face(findFace(all, Hat, side), [&](int lx, int ly, int, int) -> QColor {
                    if (side == Side::Top)
                        return helmet;
                    if (side == Side::Front && ly <= 1 && (lx == 3 || lx == 4))
                        return QColor(0xff, 0xf7, 0xa0);  // lamp
                    return ly <= 1 ? helmet : QColor();
                }, 6);
            }
            // dirt on the clothes
            for (auto box : { Body, RightArm, LeftArm, RightLeg, LeftLeg }) {
                for (auto side : { Side::Front, Side::Back }) {
                    b.face(findFace(all, box, side), [&](int, int, int, int) {
                        return b.chance(0.08) ? QColor(0x4a, 0x3a, 0x2a) : QColor();
                    }, 10);
                }
            }
            break;
        }
        case 5: {  // wizard
            look.shirt = QColor(0x4b, 0x2a, 0x7a);
            look.trim = QColor(0xd8, 0xa0, 0x2c);
            look.pants = QColor(0x3a, 0x20, 0x60);
            look.shoes = QColor(0x2a, 0x18, 0x40);
            look.hair = QColor(0xe8, 0xe8, 0xe8);
            look.beard = true;
            look.hairLength = 2;
            look.longSleeves = true;
            paintPerson(b, all, look);
            // the beard runs down the chest, stars on the robe and a hat band
            b.face(findFace(all, Body, Side::Front), [](int lx, int ly, int w, int) -> QColor {
                return ly <= 3 && lx >= 2 && lx < w - 2 - ly / 2 ? QColor(0xe8, 0xe8, 0xe8) : QColor();
            });
            for (auto side : { Side::Top, Side::Right, Side::Left, Side::Back, Side::Front }) {
                b.face(findFace(all, Hat, side), [&](int lx, int ly, int, int) -> QColor {
                    if (side == Side::Top)
                        return QColor(0x4b, 0x2a, 0x7a);
                    if (ly == 1)
                        return QColor(0xd8, 0xa0, 0x2c);
                    return ly == 0 && lx % 3 != 1 ? QColor(0x4b, 0x2a, 0x7a) : QColor();
                });
            }
            for (auto box : { Jacket, RightSleeve, LeftSleeve }) {
                for (auto side : { Side::Front, Side::Back, Side::Right, Side::Left }) {
                    b.face(findFace(all, box, side), [&](int, int, int, int) {
                        return b.chance(0.05) ? QColor(0xff, 0xe0, 0x6a) : QColor();
                    }, 8);
                }
            }
            break;
        }
        case 6: {  // zombie
            look.skin = QColor(0x4f, 0x8a, 0x3b);
            look.hair = QColor(0x2e, 0x55, 0x22);
            look.eyes = QColor(0x10, 0x10, 0x10);
            look.shirt = QColor(0x00, 0xa8, 0xa8);
            look.trim = QColor(0x00, 0x86, 0x86);
            look.pants = QColor(0x46, 0x39, 0x7a);
            look.shoes = QColor(0x35, 0x2b, 0x5c);
            look.mouth = QColor(0x24, 0x40, 0x1c);
            look.hairLength = 1;
            look.beard = false;
            paintPerson(b, all, look);
            // torn clothes: skin shows through here and there
            for (auto box : { Body, RightArm, LeftArm, RightLeg, LeftLeg }) {
                for (auto side : { Side::Front, Side::Back, Side::Right, Side::Left }) {
                    b.face(findFace(all, box, side), [&](int, int ly, int, int) {
                        return ly > 3 && b.chance(0.07) ? look.skin : QColor();
                    }, 8);
                }
            }
            break;
        }
        default:  // random person
            paintPerson(b, all, look);
            if (b.chance(0.35)) {
                // a cap in a color of the clothes
                const auto cap = look.trim;
                for (auto side : { Side::Top, Side::Right, Side::Left, Side::Back, Side::Front }) {
                    b.face(findFace(all, Hat, side), [&](int, int ly, int, int) { return side == Side::Top || ly <= 1 ? cap : QColor(); });
                }
            }
            break;
    }
    return image;
}

/* ---------------------------------------------------------------- capes */

/// the pattern of a cape in the coordinates of its outer face (10 x 16)
using Pattern = std::function<QColor(double u, double v)>;

void paintCape(Brush& b, const Pattern& pattern, const QColor& edge, const QColor& inner)
{
    const auto all = faces(capeBoxes());
    // outside of the cape (seen from behind the player) and the inner side
    b.face(findFace(all, Cape, Side::Front), [&](int lx, int ly, int w, int h) { return pattern((lx + 0.5) / w, (ly + 0.5) / h); });
    b.fill(findFace(all, Cape, Side::Back), inner, 4);
    for (auto side : { Side::Top, Side::Bottom, Side::Right, Side::Left }) {
        b.fill(findFace(all, Cape, side), edge, 4);
    }
    // the elytra uses the same pattern
    b.face(findFace(all, Elytra, Side::Front), [&](int lx, int ly, int w, int h) { return pattern((lx + 0.5) / w, (ly + 0.5) / h); });
    b.face(findFace(all, Elytra, Side::Back), [&](int lx, int ly, int w, int h) { return pattern(1 - (lx + 0.5) / w, (ly + 0.5) / h); });
    for (auto side : { Side::Top, Side::Bottom, Side::Right, Side::Left }) {
        b.fill(findFace(all, Elytra, side), edge, 4);
    }
}

QImage paintCapeStyle(int style, quint32 seed)
{
    QImage image = blankCape();
    Brush b(image, seed);
    std::mt19937 shapes(seed ^ 0x9e3779b9u);
    auto random = [&shapes](int from, int to) { return std::uniform_int_distribution<int>(from, to)(shapes); };

    if (style == 0) {
        style = random(1, int(capeStyles().size()) - 1);
    }
    const QColor main = s_clothes[random(0, int(std::size(s_clothes)) - 1)];
    QColor second = s_clothes[random(0, int(std::size(s_clothes)) - 1)];
    if (second == main) {
        second = main.lighter(150);
    }

    switch (style) {
        case 1: {  // sculk: dark with glowing veins and dots
            const QColor dark(0x0b, 0x26, 0x2c);
            const QColor glow(0x29, 0xdf, 0xeb);
            QList<QPointF> veins;
            // a few thin veins that branch down the cape, one pixel wide
            for (int i = 0; i < 3; i++) {
                double u = (i + 0.5) / 3.0 + random(-8, 8) / 100.0;
                for (double v = random(0, 30) / 100.0; v < 1; v += 1.0 / 32) {
                    veins << QPointF(u, v);
                    u = std::clamp(u + random(-5, 5) / 100.0, 0.05, 0.95);
                }
            }
            paintCape(b, [&](double u, double v) -> QColor {
                for (const auto& p : veins) {
                    if (std::abs(p.x() - u) < 0.045 && std::abs(p.y() - v) < 0.02)
                        return glow;
                }
                return b.chance(0.03) ? QColor(0x12, 0x8a, 0x95) : dark;
            }, QColor(0x07, 0x1a, 0x1f), QColor(0x0e, 0x30, 0x37));
            break;
        }
        case 2:  // gradient
            paintCape(b, [&](double, double v) {
                return QColor::fromRgbF(main.redF() * (1 - v) + second.redF() * v, main.greenF() * (1 - v) + second.greenF() * v,
                                        main.blueF() * (1 - v) + second.blueF() * v);
            }, main.darker(160), main.darker(130));
            break;
        case 3:  // stripes
            paintCape(b, [&](double, double v) { return int(v * 8) % 2 == 0 ? main : second; }, main.darker(160), main.darker(130));
            break;
        case 4:  // checkers
            paintCape(b, [&](double u, double v) { return (int(u * 5) + int(v * 8)) % 2 == 0 ? main : second; }, main.darker(160),
                      second.darker(130));
            break;
        case 5:  // flame: red and orange tongues rising from the bottom
            paintCape(b, [&](double u, double v) -> QColor {
                const double tongue = 0.45 + 0.25 * std::sin(u * 12.0 + seed % 7) + 0.1 * std::sin(u * 31.0);
                if (v > tongue + 0.25)
                    return QColor(0xff, 0xd2, 0x3a);
                if (v > tongue)
                    return QColor(0xf2, 0x6a, 0x1b);
                return QColor(0x3a, 0x0e, 0x0a);
            }, QColor(0x22, 0x08, 0x06), QColor(0x30, 0x0c, 0x08));
            break;
        default: {  // emblem: a star in a circle on a plain cape
            paintCape(b, [&](double u, double v) -> QColor {
                const double x = (u - 0.5) * 10.0;
                const double y = (v - 0.4) * 16.0;
                const double r = std::sqrt(x * x + y * y);
                const double angle = std::atan2(y, x);
                const double star = 2.0 + 1.3 * std::cos(5 * angle);
                if (r < star)
                    return second;
                if (r > 3.6 && r < 4.4)
                    return second.darker(120);
                return main;
            }, main.darker(160), main.darker(130));
            break;
        }
    }
    return image;
}

}  // namespace

/* ---------------------------------------------------------------- geometry */

QList<Box> skinBoxes(bool slim)
{
    const int arm = slim ? 3 : 4;
    return {
        { tr("Head"), { 0, 0 }, 8, 8, 8, false, -1 },
        { tr("Hat (2nd layer)"), { 32, 0 }, 8, 8, 8, true, -1 },
        { tr("Body"), { 16, 16 }, 8, 12, 4, false, -1 },
        { tr("Jacket (2nd layer)"), { 16, 32 }, 8, 12, 4, true, -1 },
        { tr("Right arm"), { 40, 16 }, arm, 12, 4, false, LeftArm },
        { tr("Right sleeve (2nd layer)"), { 40, 32 }, arm, 12, 4, true, LeftSleeve },
        { tr("Left arm"), { 32, 48 }, arm, 12, 4, false, RightArm },
        { tr("Left sleeve (2nd layer)"), { 48, 48 }, arm, 12, 4, true, RightSleeve },
        { tr("Right leg"), { 0, 16 }, 4, 12, 4, false, LeftLeg },
        { tr("Right trouser leg (2nd layer)"), { 0, 32 }, 4, 12, 4, true, LeftPants },
        { tr("Left leg"), { 16, 48 }, 4, 12, 4, false, RightLeg },
        { tr("Left trouser leg (2nd layer)"), { 0, 48 }, 4, 12, 4, true, RightPants },
    };
}

QList<Box> capeBoxes()
{
    return {
        { tr("Cape"), { 0, 0 }, 10, 16, 1, false, -1 },
        { tr("Elytra"), { 22, 0 }, 10, 20, 2, true, -1 },
    };
}

QList<Face> faces(const QList<Box>& boxes)
{
    QList<Face> all;
    for (int i = 0; i < boxes.size(); i++) {
        all << boxFaces(i, boxes.at(i));
    }
    return all;
}

const Face* faceAt(const QList<Face>& all, QPoint pixel)
{
    for (const auto& face : all) {
        if (face.rect.contains(pixel)) {
            return &face;
        }
    }
    return nullptr;
}

QString sideName(Side side)
{
    switch (side) {
        case Side::Top:
            return tr("top");
        case Side::Bottom:
            return tr("bottom");
        case Side::Right:
            return tr("right side");
        case Side::Front:
            return tr("front");
        case Side::Left:
            return tr("left side");
        case Side::Back:
            return tr("back");
    }
    return {};
}

QString describe(const QList<Box>& boxes, const Face& face)
{
    const auto& box = boxes.at(face.box);
    // the cape is seen from behind: its "front" is the outside, its "back" lies on the player's back
    if (boxes.size() == 2 && face.box == Cape) {
        if (face.side == Side::Front)
            return tr("Cape — outside (seen from behind)");
        if (face.side == Side::Back)
            return tr("Cape — inside");
        return tr("Cape — edge");
    }
    if (boxes.size() == 2) {
        return tr("Elytra — %1").arg(face.side == Side::Front || face.side == Side::Back ? tr("wing") : tr("edge"));
    }
    if (face.box == Head && face.side == Side::Front) {
        return tr("Head — face");
    }
    return QString("%1 — %2").arg(box.name, sideName(face.side));
}

QPoint mirrorPixel(const QList<Box>& boxes, const QList<Face>& all, QPoint pixel)
{
    const auto* face = faceAt(all, pixel);
    if (!face) {
        return { -1, -1 };
    }
    const auto& box = boxes.at(face->box);
    const int targetBox = box.mirror >= 0 ? box.mirror : face->box;
    Side targetSide = face->side;
    if (targetSide == Side::Right) {
        targetSide = Side::Left;
    } else if (targetSide == Side::Left) {
        targetSide = Side::Right;
    }
    const auto* target = findFace(all, targetBox, targetSide);
    if (!target || target->rect.size() != face->rect.size()) {
        return { -1, -1 };
    }
    // the faces of a box run around it, so mirroring always flips the x position inside the face
    const int lx = pixel.x() - face->rect.x();
    const int ly = pixel.y() - face->rect.y();
    return { target->rect.x() + face->rect.width() - 1 - lx, target->rect.y() + ly };
}

/* ---------------------------------------------------------------- generators */

QStringList skinStyles()
{
    return { tr("Random"), tr("Sculk"), tr("Knight"), tr("Ninja"), tr("Miner"), tr("Wizard"), tr("Zombie") };
}

QStringList capeStyles()
{
    return { tr("Random"), tr("Sculk"), tr("Gradient"), tr("Stripes"), tr("Checkers"), tr("Flame"), tr("Emblem") };
}

QImage generateSkin(int style, quint32 seed, bool slim)
{
    return paintSkin(style, seed, slim);
}

QImage generateCape(int style, quint32 seed)
{
    return paintCapeStyle(style, seed);
}

QImage blankSkin()
{
    QImage image(64, 64, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    return image;
}

QImage blankCape()
{
    QImage image(64, 32, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    return image;
}

}  // namespace SkinArt
