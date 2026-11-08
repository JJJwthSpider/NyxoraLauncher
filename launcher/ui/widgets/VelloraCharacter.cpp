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
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include "settings/SettingsObject.h"
#include <algorithm>
#include <cmath>
#include <numbers>

#include "Application.h"
#include "FileSystem.h"

namespace Cosmetics {
namespace {
struct Data {
    QList<Item> items;
    QList<Outfit> outfits;
};

QColor hexColor(const QString& hex)
{
    QColor c(hex.left(7));
    if (hex.size() >= 9) {
        c.setAlpha(hex.mid(7, 2).toInt(nullptr, 16));
    }
    return c;
}

/// the wardrobe from cosmetics.json (written by android/art/cosmetics.py; the Android app draws the same file)
const Data& data()
{
    static const Data s_data = [] {
        Data d;
        QFile file(":/sculk_theme/cosmetics.json");
        if (!file.open(QIODevice::ReadOnly)) {
            return d;
        }
        const auto root = QJsonDocument::fromJson(file.readAll()).object();
        for (const auto& value : root.value("items").toArray()) {
            const auto o = value.toObject();
            Item item;
            item.id = o.value("id").toString();
            item.slot = o.value("slot").toString();
            item.titleRu = o.value("title").toString();
            item.titleEn = o.value("titleEn").toString();
            item.rarity = o.value("rarity").toString("common");
            for (const auto& lv : o.value("layers").toArray()) {
                const auto l = lv.toObject();
                Layer layer;
                layer.x = l.value("x").toDouble();
                layer.y = l.value("y").toDouble();
                for (const auto& row : l.value("rows").toArray()) {
                    layer.rows << row.toString();
                }
                const auto colors = l.value("colors").toObject();
                for (auto it = colors.begin(); it != colors.end(); ++it) {
                    layer.colors.insert(it.key().at(0), hexColor(it.value().toString()));
                }
                layer.view = l.value("view").toString("front");
                layer.anim = l.value("anim").toString();
                item.layers << layer;
            }
            const auto particles = o.value("particles").toObject();
            for (const auto& c : particles.value("colors").toArray()) {
                item.particleColors << hexColor(c.toString());
            }
            item.particleCount = particles.value("count").toInt(10);
            item.motion = particles.value("motion").toString();
            item.shape = particles.value("shape").toString();
            d.items << item;
        }
        for (const auto& value : root.value("outfits").toArray()) {
            const auto o = value.toObject();
            d.outfits << Outfit{ o.value("title").toString(), o.value("titleEn").toString(), o.value("head").toString(),
                                 o.value("back").toString(),  o.value("body").toString(),    o.value("pet").toString(),
                                 o.value("effect").toString() };
        }
        return d;
    }();
    return s_data;
}

bool russian()
{
    return APPLICATION->settings()->get("Language").toString().startsWith("ru");
}
}  // namespace

const QList<Item>& items()
{
    return data().items;
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

const Item* find(const QString& id)
{
    if (id.isEmpty()) {
        return nullptr;
    }
    for (const auto& item : items()) {
        if (item.id == id) {
            return &item;
        }
    }
    return nullptr;
}

QString title(const Item& item)
{
    return russian() || item.titleEn.isEmpty() ? item.titleRu : item.titleEn;
}

QString title(const Outfit& outfit)
{
    return russian() || outfit.titleEn.isEmpty() ? outfit.titleRu : outfit.titleEn;
}

QColor rarityColor(const QString& rarity)
{
    if (rarity == "legendary") {
        return QColor(0xFF, 0xB4, 0x3C);
    }
    if (rarity == "epic") {
        return QColor(0xB0, 0x4C, 0xFF);
    }
    if (rarity == "rare") {
        return QColor(0x3C, 0x8C, 0xFF);
    }
    return QColor(0x8A, 0x93, 0xA0);
}

const QList<Outfit>& outfits()
{
    return data().outfits;
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

void wearOutfit(const Outfit& outfit)
{
    wear(Head, outfit.head);
    wear(Back, outfit.back);
    wear(Body, outfit.body);
    wear(Pet, outfit.pet);
    wear(Effect, outfit.effect);
}
}  // namespace Cosmetics

namespace {
/// draws in skin pixels relative to the figure's top left corner; `u` is one pixel on screen
struct Pen {
    QPainter& p;
    double ox, oy, u;
    void rect(double x, double y, double w, double h, const QColor& c) const { p.fillRect(QRectF(ox + x * u, oy + y * u, w * u, h * u), c); }
};

/// the layers of an item seen from this side, with their animation
void drawItem(const Pen& p, const Cosmetics::Item* item, bool behind, double time)
{
    if (!item) {
        return;
    }
    for (const auto& layer : item->layers) {
        if (layer.view != "both" && layer.view != (behind ? "back" : "front")) {
            continue;
        }
        double dy = 0;
        if (layer.anim == "bob") {
            dy = std::sin(time * 2) * 0.5;
        } else if (layer.anim == "flap") {
            dy = -(0.5 + 0.5 * std::sin(time * 2.2)) * 2;
        } else if (layer.anim == "hop") {
            dy = -std::abs(std::sin(time * 3)) * 2;
        } else if (layer.anim == "float") {
            dy = std::sin(time * 2) * 1.5;
        }
        for (int j = 0; j < layer.rows.size(); j++) {
            const auto& row = layer.rows[j];
            for (int i = 0; i < row.size(); i++) {
                const auto it = layer.colors.constFind(row[i]);
                if (it != layer.colors.constEnd()) {
                    p.rect(layer.x + i, layer.y + j + dy, 1, 1, *it);
                }
            }
        }
    }
}

double unitHash(int a, int b)
{
    quint32 h = quint32(a) * 374761393u + quint32(b) * 668265263u + 0x2545F491u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return ((h ^ (h >> 16)) % 10000) / 10000.0;
}

/// the particles of an effect around the figure (rise, fall, orbit, spark)
void drawParticles(const Pen& p, const Cosmetics::Item* item, double time)
{
    if (!item || item->particleColors.isEmpty()) {
        return;
    }
    for (int i = 0; i < item->particleCount; i++) {
        QColor color = item->particleColors[i % item->particleColors.size()];
        const double speed = 4 + unitHash(i, 1) * 5;
        const double phase = unitHash(i, 2) * 40;
        double x, y, alpha;
        if (item->motion == "orbit") {
            const double angle = time * 1.4 + i * std::numbers::pi * 2 / item->particleCount;
            x = 8 + std::cos(angle) * 12;
            y = 16 + std::sin(angle) * 4 + std::sin(time * 2 + i) * 6;
            alpha = 0.6 + 0.4 * std::sin(angle);
        } else if (item->motion == "fall") {
            x = -6 + unitHash(i, 3) * 28;
            y = -8 + std::fmod(time * speed + phase, 44.0);
            alpha = 0.8;
        } else if (item->motion == "spark") {
            x = -4 + unitHash(i, 3) * 24;
            y = -4 + unitHash(i, 4) * 38;
            alpha = std::max(0.0, std::sin(time * (2 + unitHash(i, 5) * 3) + phase));
        } else {
            x = -2 + unitHash(i, 3) * 20;
            const double travel = std::fmod(time * speed + phase, 40.0);
            y = 34 - travel;
            alpha = std::min(1.0, travel / 6) * std::max(0.0, 1 - travel / 40);
        }
        color.setAlphaF(std::clamp(color.alphaF() * alpha, 0.0, 1.0));
        if (item->shape == "heart") {
            p.rect(x, y, 1, 1, color);
            p.rect(x + 2, y, 1, 1, color);
            p.rect(x, y + 1, 3, 1, color);
            p.rect(x + 1, y + 2, 1, 1, color);
        } else if (item->shape == "drop") {
            p.rect(x, y, 0.6, 1.6, color);
        } else {
            p.rect(x, y, 0.9, 0.9, color);
        }
    }
}

void drawSilhouette(const Pen& p, bool leftUp, bool rightUp, double lift, bool back)
{
    const QRectF parts[] = { { 4, 0, 8, 8 },   { 4, 8, 8, 12 }, { 0, rightUp ? -4.0 : 8.0, 4, 12 }, { 12, leftUp ? -4.0 : 8.0, 4, 12 },
                             { 4, 20, 4, 12 }, { 8, 20, 4, 12 } };
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
                                  const QString& pet, const QString& emote, const QString& effect, double time)
{
    const double u = std::min(target.width() / 32.0, target.height() / 40.0);
    Pen pen{ painter, target.left() + (target.width() - 32 * u) / 2 + 8 * u, target.top() + (target.height() - 40 * u) / 2 + 8 * u, u };
    drawItem(pen, Cosmetics::find(back), false, time);
    drawSilhouette(pen, emote == "wave" || emote == "cheer", emote == "cheer", emote == "jump" ? -3 : emote == "bow" ? 1 : 0, false);
    drawItem(pen, Cosmetics::find(body), false, time);
    drawItem(pen, Cosmetics::find(head), false, time);
    drawItem(pen, Cosmetics::find(pet), false, time);
    drawParticles(pen, Cosmetics::find(effect), time);
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
    double jump = emoting && m_emote == "jump" ? std::max(0.0, std::sin(emoteTime * std::numbers::pi / 0.6)) * 5 : 0;
    if (emoting && m_emote == "bow") {
        jump = -std::max(0.0, std::sin(emoteTime * std::numbers::pi / 1.2)) * 2;  // a nod down
    }
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
    const bool savedBack = m_back;
    if (emoting && m_emote == "spin") {
        m_back = int(emoteTime * 5) % 2 == 1;  // turns round and round
    }
    if (!m_back) {
        drawItem(pen, Cosmetics::find(back), false, time);
    }
    const bool wave = emoting && (m_emote == "wave" || m_emote == "cheer");
    const bool cheer = emoting && m_emote == "cheer";
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
        drawSilhouette(pen, wave, cheer, 0, m_back);
    } else {
        const bool modern = m_skin.height() >= 64;
        if (!m_back) {
            part(4, 20, 4, 12, 4, 20, false);
            modern ? part(20, 52, 4, 12, 8, 20, false) : part(4, 20, 4, 12, 8, 20, true);
            part(20, 20, 8, 12, 4, 8, false);
            part(44, 20, 4, 12, 0, cheer ? armY : 8, false);
            modern ? part(36, 52, 4, 12, 12, armY, false) : part(44, 20, 4, 12, 12, armY, true);
            part(8, 8, 8, 8, 4, 0, false);
            if (modern) {
                part(4, 36, 4, 12, 4, 20, false);
                part(4, 52, 4, 12, 8, 20, false);
                part(20, 36, 8, 12, 4, 8, false);
                part(44, 36, 4, 12, 0, cheer ? armY : 8, false);
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
        if (!m_cape.isNull() && back != "crimson_cape" && back != "backpack") {
            const double s = m_cape.width() / 64.0;
            painter.drawImage(QRectF(ox + 3 * u, oy + 8 * u, 10 * u, 16 * u), m_cape, QRectF(1 * s, 1 * s, 10 * s, 16 * s));
        }
        drawItem(pen, Cosmetics::find(back), true, time);
    }
    drawItem(pen, Cosmetics::find(Cosmetics::worn(Cosmetics::Body)), m_back, time);
    drawItem(pen, Cosmetics::find(Cosmetics::worn(Cosmetics::Head)), m_back, time);
    drawItem(pen, Cosmetics::find(Cosmetics::worn(Cosmetics::Pet)), m_back, time);
    drawParticles(pen, Cosmetics::find(Cosmetics::worn(Cosmetics::Effect)), time);
    m_back = savedBack;
}
