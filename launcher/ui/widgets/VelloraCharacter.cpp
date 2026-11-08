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
#include "VelloraCharacter.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QRadialGradient>
#include <QSettings>
#include <algorithm>
#include <cmath>
#include <numbers>

#include "Application.h"
#include "FileSystem.h"

namespace Cosmetics {
const QList<Item>& items()
{
    static const QList<Item> s_items = {
        { "crown", Head, QT_TRANSLATE_NOOP("Cosmetics", "Crown") },
        { "horns", Head, QT_TRANSLATE_NOOP("Cosmetics", "Demon horns") },
        { "halo", Head, QT_TRANSLATE_NOOP("Cosmetics", "Halo") },
        { "wizard_hat", Head, QT_TRANSLATE_NOOP("Cosmetics", "Wizard hat") },
        { "cat_ears", Head, QT_TRANSLATE_NOOP("Cosmetics", "Cat ears") },
        { "headphones", Head, QT_TRANSLATE_NOOP("Cosmetics", "Headphones") },
        { "shades", Head, QT_TRANSLATE_NOOP("Cosmetics", "Shades") },
        { "scarf", Body, QT_TRANSLATE_NOOP("Cosmetics", "Scarf") },
        { "chain", Body, QT_TRANSLATE_NOOP("Cosmetics", "Gold chain") },
        { "tie", Body, QT_TRANSLATE_NOOP("Cosmetics", "Tie") },
        { "bat_wings", Back, QT_TRANSLATE_NOOP("Cosmetics", "Bat wings") },
        { "angel_wings", Back, QT_TRANSLATE_NOOP("Cosmetics", "Angel wings") },
        { "demon_wings", Back, QT_TRANSLATE_NOOP("Cosmetics", "Demon wings") },
        { "crimson_cape", Back, QT_TRANSLATE_NOOP("Cosmetics", "Crimson mantle") },
        { "cube_pet", Pet, QT_TRANSLATE_NOOP("Cosmetics", "Tesseract") },
        { "bat_pet", Pet, QT_TRANSLATE_NOOP("Cosmetics", "Bat") },
        { "slime_pet", Pet, QT_TRANSLATE_NOOP("Cosmetics", "Slime") },
        { "ghost_pet", Pet, QT_TRANSLATE_NOOP("Cosmetics", "Ghost") },
        { "wave", Emote, QT_TRANSLATE_NOOP("Cosmetics", "Wave") },
        { "jump", Emote, QT_TRANSLATE_NOOP("Cosmetics", "Jump") },
        { "dance", Emote, QT_TRANSLATE_NOOP("Cosmetics", "Dance") },
    };
    return s_items;
}

QList<Item> inSlot(const QString& slot)
{
    QList<Item> out;
    for (const auto& item : items()) {
        if (item.slot == slot) {
            out << item;
        }
    }
    return out;
}

QString title(const Item& item)
{
    return QCoreApplication::translate("Cosmetics", item.title);
}

const QList<Outfit>& outfits()
{
    static const QList<Outfit> s_outfits = {
        { QT_TRANSLATE_NOOP("Cosmetics", "Demon lord"), "horns", "demon_wings", "scarf", "" },
        { QT_TRANSLATE_NOOP("Cosmetics", "Fallen angel"), "halo", "angel_wings", "chain", "" },
        { QT_TRANSLATE_NOOP("Cosmetics", "Tesseract mage"), "wizard_hat", "crimson_cape", "chain", "cube_pet" },
        { QT_TRANSLATE_NOOP("Cosmetics", "Night shadow"), "shades", "bat_wings", "tie", "bat_pet" },
        { QT_TRANSLATE_NOOP("Cosmetics", "King"), "crown", "crimson_cape", "chain", "slime_pet" },
    };
    return s_outfits;
}

static QSettings& store()
{
    static QSettings s_settings(FS::PathCombine(APPLICATION->dataRoot(), "wardrobe.ini"), QSettings::IniFormat);
    return s_settings;
}

QString worn(const QString& slot)
{
    return store().value(slot).toString();
}

void wear(const QString& slot, const QString& id)
{
    store().setValue(slot, id);
    store().sync();
}
}  // namespace Cosmetics

namespace {
const QColor s_gold(0xE6, 0xB4, 0x3C), s_goldDark(0x9C, 0x6E, 0x1E), s_ruby(0xE8, 0x36, 0x4F), s_black(0x12, 0x03, 0x06),
    s_white(0xF4, 0xEE, 0xF0);

/// draws in skin pixels relative to the figure's top left corner; `u` is one pixel on screen
struct Pen {
    QPainter& p;
    double ox, oy, u;
    void rect(double x, double y, double w, double h, const QColor& c) const { p.fillRect(QRectF(ox + x * u, oy + y * u, w * u, h * u), c); }
    void sprite(double x, double y, const QStringList& rows, const QString& keys, const QList<QColor>& colors) const
    {
        for (int j = 0; j < rows.size(); j++) {
            for (int i = 0; i < rows[j].size(); i++) {
                const int k = keys.indexOf(rows[j][i]);
                if (k >= 0) {
                    rect(x + i, y + j, 1, 1, colors[k]);
                }
            }
        }
    }
    void glow(double x, double y, double radius, const QColor& c) const
    {
        QRadialGradient g(QPointF(ox + x * u, oy + y * u), radius * u);
        g.setColorAt(0, c);
        g.setColorAt(1, QColor(c.red(), c.green(), c.blue(), 0));
        p.setPen(Qt::NoPen);
        p.setBrush(g);
        p.drawEllipse(QPointF(ox + x * u, oy + y * u), radius * u, radius * u);
    }
};

void drawBack(const Pen& p, const QString& id, double flap, bool fromBehind)
{
    if (id.isEmpty()) {
        return;
    }
    if (id == "crimson_cape") {
        if (fromBehind) {
            p.rect(3, 8, 10, 17, QColor(0x7A, 0x0F, 0x22));
            p.rect(3, 8, 10, 1, QColor(0xB0, 0x1C, 0x38));
            p.rect(4, 23, 8, 2, QColor(0x5A, 0x0A, 0x18));
            p.rect(7, 12, 2, 2, s_gold);
        } else {
            p.rect(2, 9, 1, 15, QColor(0x7A, 0x0F, 0x22));
            p.rect(13, 9, 1, 15, QColor(0x7A, 0x0F, 0x22));
        }
        return;
    }
    QList<QColor> colors;
    if (id == "angel_wings") {
        colors = { s_white, QColor(0xE8, 0xE2, 0xE6), QColor(0xC9, 0xC2, 0xC8) };
    } else if (id == "demon_wings") {
        colors = { s_ruby, QColor(0x9E, 0x16, 0x30), QColor(0x5A, 0x0A, 0x18) };
    } else {
        colors = { QColor(0x4A, 0x1A, 0x26), QColor(0x2E, 0x10, 0x18), QColor(0x14, 0x07, 0x0B) };
    }
    const QStringList wing = { "....aa", "..aabb", ".abbbc", "abbbcc", "abbcc.", "abcc..", ".bc...", ".c...." };
    QStringList mirrored;
    for (const auto& row : wing) {
        QString r = row;
        std::reverse(r.begin(), r.end());
        mirrored << r;
    }
    const double lift = flap * 2;
    p.sprite(-5, 6 - lift, wing, "abc", colors);
    p.sprite(15, 6 - lift, mirrored, "abc", colors);
}

void drawBody(const Pen& p, const QString& id)
{
    if (id == "scarf") {
        p.rect(4, 8, 8, 2, QColor(0xB0, 0x1C, 0x38));
        p.rect(4, 8, 8, 1, s_ruby);
        p.rect(9, 10, 2, 4, QColor(0xB0, 0x1C, 0x38));
    } else if (id == "chain") {
        p.rect(5, 9, 1, 1, s_gold);
        p.rect(6, 10, 1, 1, s_gold);
        p.rect(7, 11, 2, 1, s_gold);
        p.rect(9, 10, 1, 1, s_gold);
        p.rect(10, 9, 1, 1, s_gold);
        p.rect(7, 12, 2, 2, s_ruby);
    } else if (id == "tie") {
        p.rect(7, 8, 2, 1, s_black);
        p.rect(7, 9, 2, 5, QColor(0x7A, 0x0F, 0x22));
    }
}

void drawHead(const Pen& p, const QString& id, double time)
{
    if (id == "crown") {
        p.sprite(3, -3, { "a..a..a..a", "aaaaaaaaaa", "abaacaabaa" }, "abc", { s_gold, s_ruby, QColor(0x5C, 0xB6, 0xFF) });
        p.rect(3, -1, 10, 1, s_goldDark);
    } else if (id == "horns") {
        p.sprite(1, -5, { "b.........b", "a.........a", "aa.......aa", ".aa.....aa." }, "ab", { QColor(0x2A, 0x0A, 0x12), QColor(0x7A, 0x0F, 0x22) });
    } else if (id == "halo") {
        const double bob = std::sin(time * 2) * 0.4;
        p.glow(8, -3 + bob, 5, QColor(255, 224, 138, 70));
        p.rect(4, -4 + bob, 8, 1, s_gold);
        p.rect(3, -3 + bob, 1, 1, s_gold);
        p.rect(12, -3 + bob, 1, 1, s_gold);
        p.rect(4, -2 + bob, 8, 1, s_goldDark);
    } else if (id == "wizard_hat") {
        p.sprite(2, -9,
                 { "......ab....", ".....aab....", ".....aabb...", "....aaabb...", "....aaabbb..", "...aaaabbb..", "...aaaaabbb.",
                   "..aaaaaabbb.", "cccccccccccc", "dddddddddddd" },
                 "abcd", { QColor(0x5A, 0x0A, 0x18), QColor(0x8E, 0x14, 0x28), s_gold, s_black });
    } else if (id == "cat_ears") {
        p.sprite(3, -3, { "a........a", "aa......aa", "aba....aba" }, "ab", { QColor(0x1A, 0x0A, 0x0E), QColor(0xE8, 0x7A, 0x90) });
    } else if (id == "headphones") {
        p.rect(3, -1, 10, 1, s_black);
        p.rect(3, 0, 1, 3, s_black);
        p.rect(12, 0, 1, 3, s_black);
        p.rect(2, 3, 2, 3, s_ruby);
        p.rect(12, 3, 2, 3, s_ruby);
    } else if (id == "shades") {
        p.rect(4, 3, 8, 1, s_black);
        p.rect(5, 4, 2, 1, s_black);
        p.rect(9, 4, 2, 1, s_black);
    }
}

void drawPet(const Pen& p, const QString& id, double time)
{
    if (id.isEmpty()) {
        return;
    }
    const double hop = std::abs(std::sin(time * 3)) * 2;
    const double x = 18;
    double y = 24 - hop;
    if (id == "cube_pet") {
        p.glow(x + 2.5, y + 2.5, 4, QColor(255, 40, 60, 70));
        p.sprite(x, y, { ".aaa.", "abbba", "abcba", "abbba", ".aaa." }, "abc", { s_ruby, QColor(0x7A, 0x0F, 0x22), QColor(0xFF, 0xC8, 0xCD) });
    } else if (id == "bat_pet") {
        const bool up = std::sin(time * 12) > 0;
        y = 14 + std::sin(time * 2) * 2;
        p.sprite(x, y, { up ? "a.....a" : ".......", up ? "aa.b.aa" : "aaabaaa", ".abbba.", "..bcb.." }, "abc",
                 { QColor(0x2E, 0x10, 0x18), s_black, s_ruby });
    } else if (id == "slime_pet") {
        p.sprite(x, y + 2, { ".aaaa.", "abbbba", "abcbca", "abbbba", ".aaaa." }, "abc", { QColor(0x7A, 0x0F, 0x22), QColor(0xC8, 0x1F, 0x3A), s_black });
    } else if (id == "ghost_pet") {
        y = 16 + std::sin(time * 1.5) * 1.5;
        p.sprite(x, y, { ".aaa.", "aaaaa", "abaab", "aaaaa", "aaaaa", "a.a.a" }, "ab", { QColor(0xF4, 0xEE, 0xF0, 204), s_black });
    }
}

void drawSilhouette(const Pen& p, bool wave, double lift, bool back)
{
    const QRectF parts[] = { { 4, 0, 8, 8 }, { 4, 8, 8, 12 }, { 0, 8, 4, 12 }, { 12, wave ? -4.0 : 8.0, 4, 12 }, { 4, 20, 4, 12 }, { 8, 20, 4, 12 } };
    for (const auto& r : parts) {
        p.rect(r.x(), r.y() + lift, r.width(), r.height(), QColor(0x8A, 0x34, 0x42));
        p.rect(r.x() + 0.5, r.y() + 0.5 + lift, r.width() - 1, r.height() - 1, QColor(0x2B, 0x0E, 0x15));
    }
    if (!back) {
        p.rect(6, 4 + lift, 1, 1, QColor(0xFF, 0x46, 0x54));
        p.rect(9, 4 + lift, 1, 1, QColor(0xFF, 0x46, 0x54));
    }
}
}  // namespace

VelloraCharacter::VelloraCharacter(QWidget* parent) : QWidget(parent)
{
    setMinimumSize(120, 160);
    setCursor(Qt::PointingHandCursor);
    setToolTip(tr("Click to turn around"));
    m_clock.start();
    m_timer.setInterval(40);
    connect(&m_timer, &QTimer::timeout, this, qOverload<>(&QWidget::update));
}

void VelloraCharacter::setSkin(const QImage& skin)
{
    m_skin = skin.convertToFormat(QImage::Format_ARGB32);
    update();
}

void VelloraCharacter::setCape(const QImage& cape)
{
    m_cape = cape;
    update();
}

void VelloraCharacter::setBack(bool back)
{
    m_back = back;
    update();
}

void VelloraCharacter::playEmote(const QString& id)
{
    m_emote = id;
    m_emoteStart = m_clock.elapsed();
    m_back = false;
    update();
}

void VelloraCharacter::mouseReleaseEvent(QMouseEvent* event)
{
    m_back = !m_back;
    update();
    QWidget::mouseReleaseEvent(event);
}

void VelloraCharacter::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    m_timer.start();
}

void VelloraCharacter::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    m_timer.stop();
}

void VelloraCharacter::paintItems(QPainter& painter, const QRectF& target, const QString& head, const QString& body, const QString& back,
                                  const QString& pet, const QString& emote)
{
    const double u = std::min(target.width() / 32.0, target.height() / 38.0);
    Pen pen{ painter, target.left() + (target.width() - 32 * u) / 2 + 8 * u, target.top() + (target.height() - 38 * u) / 2 + 6 * u, u };
    drawBack(pen, back, 0.5, false);
    drawSilhouette(pen, emote == "wave", emote == "jump" ? -3 : 0, false);
    drawBody(pen, body);
    drawHead(pen, head, 0);
    drawPet(pen, pet, 0);
}

void VelloraCharacter::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    constexpr double gridW = 32, gridH = 46, figureX = 8, figureY = 10;
    double u = std::floor(std::min(width() / gridW, height() / gridH));
    if (u < 1) {
        u = std::min(width() / gridW, height() / gridH);
    }
    const double time = m_clock.elapsed() / 1000.0;
    const double emoteTime = (m_clock.elapsed() - m_emoteStart) / 1000.0;
    const bool emoting = !m_emote.isEmpty() && emoteTime < 2.4;
    const double breath = std::sin(time * 2) * 0.35;
    const double jump = emoting && m_emote == "jump" ? std::max(0.0, std::sin(emoteTime * std::numbers::pi / 0.6)) * 5 : 0;
    const double sway = emoting && m_emote == "dance" ? std::sin(emoteTime * 9) * 1.5 : 0;
    const double left = (width() - gridW * u) / 2, top = (height() - gridH * u) / 2;
    const double ox = left + (figureX + sway) * u, oy = top + (figureY - jump + breath) * u;

    // the crimson glow on the floor
    const QPointF floor(width() / 2.0, top + (figureY + 32) * u);
    QRadialGradient glow(floor, 14 * u);
    glow.setColorAt(0, QColor(232, 54, 79, 100));
    glow.setColorAt(1, QColor(232, 54, 79, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(glow);
    painter.drawEllipse(floor, 14 * u, 3.5 * u);

    Pen pen{ painter, ox, oy, u };
    const QString back = Cosmetics::worn(Cosmetics::Back);
    const double flap = 0.5 + 0.5 * std::sin(time * (back.contains("wings") ? 2.2 : 0));
    if (!m_back) {
        drawBack(pen, back, flap, false);
    }
    const bool wave = emoting && m_emote == "wave";
    const double armY = wave ? -4 + std::sin(emoteTime * 10) * 0.6 : 8;
    auto part = [&](int sx, int sy, int w, int h, double x, double y, bool mirror) {
        if (sx + w > m_skin.width() || sy + h > m_skin.height()) {
            return;
        }
        QImage piece = m_skin.copy(sx, sy, w, h);
        if (mirror) {
            piece = piece.mirrored(true, false);
        }
        painter.drawImage(QRectF(ox + x * u, oy + y * u, w * u, h * u), piece);
    };
    if (m_skin.isNull()) {
        drawSilhouette(pen, wave, 0, m_back);
    } else {
        const bool modern = m_skin.height() >= 64;
        if (!m_back) {
            part(4, 20, 4, 12, 4, 20, false);
            modern ? part(20, 52, 4, 12, 8, 20, false) : part(4, 20, 4, 12, 8, 20, true);
            part(20, 20, 8, 12, 4, 8, false);
            part(44, 20, 4, 12, 0, 8, false);
            modern ? part(36, 52, 4, 12, 12, armY, false) : part(44, 20, 4, 12, 12, armY, true);
            part(8, 8, 8, 8, 4, 0, false);
            if (modern) {
                part(4, 36, 4, 12, 4, 20, false);
                part(4, 52, 4, 12, 8, 20, false);
                part(20, 36, 8, 12, 4, 8, false);
                part(44, 36, 4, 12, 0, 8, false);
                part(52, 52, 4, 12, 12, armY, false);
            }
            part(40, 8, 8, 8, 4, 0, false);
        } else {
            part(12, 20, 4, 12, 8, 20, false);
            modern ? part(28, 52, 4, 12, 4, 20, false) : part(12, 20, 4, 12, 4, 20, true);
            part(32, 20, 8, 12, 4, 8, false);
            part(52, 20, 4, 12, 12, 8, false);
            modern ? part(44, 52, 4, 12, 0, wave ? -4 : 8, false) : part(52, 20, 4, 12, 0, wave ? -4 : 8, true);
            part(24, 8, 8, 8, 4, 0, false);
            if (modern) {
                part(32, 36, 8, 12, 4, 8, false);
                part(52, 36, 4, 12, 12, 8, false);
                part(12, 36, 4, 12, 8, 20, false);
                part(12, 52, 4, 12, 4, 20, false);
            }
            part(56, 8, 8, 8, 4, 0, false);
        }
    }
    if (m_back) {
        if (!m_cape.isNull() && back != "crimson_cape") {
            const double s = m_cape.width() / 64.0;
            painter.drawImage(QRectF(ox + 3 * u, oy + 8 * u, 10 * u, 16 * u), m_cape, QRectF(1 * s, 1 * s, 10 * s, 16 * s));
        }
        drawBack(pen, back, flap, true);
    } else {
        drawBody(pen, Cosmetics::worn(Cosmetics::Body));
        drawHead(pen, Cosmetics::worn(Cosmetics::Head), time);
    }
    drawPet(pen, Cosmetics::worn(Cosmetics::Pet), time);
}
