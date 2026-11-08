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
#include <QRandomGenerator>
#include <QtMath>
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
#include "ui/themes/VelloraSound.h"

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
                                 o.value("effect").toString(), o.value("hand").toString() };
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
    wear(Hand, outfit.hand);
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
void drawFlatItem(const Pen& p, const Cosmetics::Item* item, bool behind, double time)
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
void drawFlatParticles(const Pen& p, const Cosmetics::Item* item, double time)
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
        } else if (item->shape == "note") {
            p.rect(x + 1, y, 1.6, 0.6, color);
            p.rect(x + 1, y, 0.6, 2.4, color);
            p.rect(x, y + 2, 1.4, 1, color);
        } else if (item->shape == "star") {
            p.rect(x + 0.6, y, 0.8, 2, color);
            p.rect(x, y + 0.6, 2, 0.8, color);
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

namespace {
constexpr int HEAD = 0, BODY = 1, RARM = 2, LARM = 3, RLEG = 4, LLEG = 5, WORLD = 6, RWING = 7, LWING = 8, CAPE = 9;
constexpr float BASE_YAW = 0.42f;  // about 24 degrees: the player turned a little, like Millida's
const char* const EMOTES[] = { "wave", "jump", "dance", "spin", "bow", "cheer", "clap", "salute", "shrug", "point", "dab", "flip", "think", "punch" };

using Mat = std::array<float, 12>;

/// a * b (b applied first)
Mat mul(const Mat& a, const Mat& b)
{
    Mat r{};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
            float v = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j];
            if (j == 3) {
                v += a[i * 4 + 3];
            }
            r[i * 4 + j] = v;
        }
    }
    return r;
}

/// a rotation about a pivot: first z, then x, then y
Mat rot(float px, float py, float pz, float rx, float ry, float rz)
{
    const float cx = std::cos(rx), sx = std::sin(rx), cy = std::cos(ry), sy = std::sin(ry), cz = std::cos(rz), sz = std::sin(rz);
    const Mat z{ cz, -sz, 0, 0, sz, cz, 0, 0, 0, 0, 1, 0 };
    const Mat x{ 1, 0, 0, 0, 0, cx, -sx, 0, 0, sx, cx, 0 };
    const Mat y{ cy, 0, sy, 0, 0, 1, 0, 0, -sy, 0, cy, 0 };
    Mat r = mul(y, mul(x, z));
    r[3] = px - (r[0] * px + r[1] * py + r[2] * pz);
    r[7] = py - (r[4] * px + r[5] * py + r[6] * pz);
    r[11] = pz - (r[8] * px + r[9] * py + r[10] * pz);
    return r;
}

Mat translate(float x, float y, float z)
{
    return { 1, 0, 0, x, 0, 1, 0, y, 0, 0, 1, z };
}

float mixf(float a, float b, float t)
{
    return a + (b - a) * t;
}

/// a dark figure for when there is no skin: crimson rim, red eyes
const QImage& shadowSkin()
{
    static QImage t = [] {
        QImage img(64, 64, QImage::Format_ARGB32);
        img.fill(Qt::transparent);
        const int boxes[][5] = { { 0, 0, 8, 8, 8 }, { 16, 16, 8, 12, 4 }, { 40, 16, 4, 12, 4 }, { 0, 16, 4, 12, 4 }, { 32, 48, 4, 12, 4 }, { 16, 48, 4, 12, 4 } };
        for (const auto& b : boxes) {
            const int u = b[0], v = b[1], w = b[2], h = b[3], d = b[4];
            for (int y = v; y < v + d + h; y++) {
                for (int x = u; x < u + 2 * (d + w); x++) {
                    if (y < v + d && (x < u + d || x >= u + d + 2 * w)) {
                        continue;
                    }
                    const bool edge = y == v || y == v + d + h - 1 || x == u || x == u + 2 * (d + w) - 1 || x == u + d || x == u + d + w - 1;
                    img.setPixel(x, y, edge ? 0xFF8A3442 : 0xFF2B0E15);
                }
            }
        }
        img.setPixel(10, 12, 0xFFFF4654);
        img.setPixel(13, 12, 0xFFFF4654);
        return img;
    }();
    return t;
}

bool filled(const Cosmetics::Layer& layer, QChar ch)
{
    return layer.colors.contains(ch);
}
}  // namespace

VelloraCharacter::VelloraCharacter(QWidget* parent) : QWidget(parent)
{
    setMinimumSize(120, 160);
    setCursor(Qt::OpenHandCursor);
    setToolTip(tr("Drag to turn, click for an animation"));
    m_clock.start();
    m_timer.setInterval(33);
    connect(&m_timer, &QTimer::timeout, this, qOverload<>(&QWidget::update));
}

void VelloraCharacter::setSkin(const QImage& skin)
{
    if (skin.isNull() || skin.width() < 64 || skin.height() < 32) {
        m_skin = QImage();
    } else {
        m_skin = (skin.width() == 64 ? skin : skin.scaled(64, skin.height() * 64 / skin.width())).convertToFormat(QImage::Format_ARGB32);
    }
    update();
}

void VelloraCharacter::setCape(const QImage& cape)
{
    m_cape = cape.isNull() ? QImage() : (cape.width() == 64 ? cape : cape.scaled(64, 32)).convertToFormat(QImage::Format_ARGB32);
    update();
}

void VelloraCharacter::setSlim(bool slim)
{
    m_slim = slim;
    update();
}

void VelloraCharacter::setBack(bool back)
{
    m_back = back;
    m_lastTouch = -100000;
    update();
}

void VelloraCharacter::playEmote(const QString& id)
{
    m_emote = id;
    m_emoteStart = m_clock.elapsed();
    update();
}

void VelloraCharacter::turnTo(double degrees)
{
    m_yaw = float(qDegreesToRadians(degrees));
    m_yawSpeed = 0;
    m_lastTouch = m_clock.elapsed();
    update();
}

void VelloraCharacter::setAnimated(bool animated)
{
    m_animated = animated;
    if (!animated) {
        m_timer.stop();
    } else if (isVisible()) {
        m_timer.start();
    }
    update();
}

void VelloraCharacter::setCloseUp(bool closeUp)
{
    m_closeUp = closeUp;
    update();
}

void VelloraCharacter::setShowItems(bool show)
{
    m_showItems = show;
    update();
}

void VelloraCharacter::mousePressEvent(QMouseEvent* event)
{
    m_down = event->position().toPoint();
    m_lastX = m_down.x();
    m_downTime = m_clock.elapsed();
    m_dragging = false;
    m_yawSpeed = 0;
    setCursor(Qt::ClosedHandCursor);
}

void VelloraCharacter::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }
    const QPoint p = event->position().toPoint();
    if (!m_dragging && (p - m_down).manhattanLength() > 4) {
        m_dragging = true;
    }
    if (m_dragging) {
        const float dx = p.x() - m_lastX;
        m_yaw += dx * 0.012f;
        m_yawSpeed = dx * 0.012f;
        m_pitch = std::clamp(m_pitch + (p.y() - m_down.y()) * 0.0002f, -0.5f, 0.25f);
        m_lastX = p.x();
        m_lastTouch = m_clock.elapsed();
        update();
    }
}

void VelloraCharacter::mouseReleaseEvent(QMouseEvent* event)
{
    setCursor(Qt::OpenHandCursor);
    m_lastTouch = m_clock.elapsed();
    if (!m_dragging && m_clock.elapsed() - m_downTime < 400) {
        // a click: an animation, never the same twice in a row
        QString next;
        do {
            next = EMOTES[QRandomGenerator::global()->bounded(int(std::size(EMOTES)))];
        } while (next == m_emote);
        playEmote(next);
        VelloraSound::play("toggle");
    }
    m_dragging = false;
    QWidget::mouseReleaseEvent(event);
}

void VelloraCharacter::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (m_animated) {
        m_timer.start();
    }
}

void VelloraCharacter::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    m_timer.stop();
}

QPointF VelloraCharacter::project(const Mat& m, float x, float y, float z, float* depth) const
{
    const float X = m[0] * x + m[1] * y + m[2] * z + m[3];
    const float Y = m[4] * x + m[5] * y + m[6] * z + m[7];
    if (depth) {
        *depth = m[8] * x + m[9] * y + m[10] * z + m[11];
    }
    return { m_cx + X * m_scale, m_cy - Y * m_scale };
}

void VelloraCharacter::drawBox(const Mat& m, float x, float y, float z, float w, float h, float d, int u, int v, const QImage& tex, float inflate,
                               bool mirror)
{
    const float x0 = x - inflate, x1 = x + w + inflate, y0 = y - inflate, y1 = y + h + inflate, z0 = z - inflate, z1 = z + d + inflate;
    const int iw = qRound(w), ih = qRound(h), id = qRound(d);
    const QImage* t = &tex;
    face(m, x0, y1, z1, x1, y1, z1, x0, y0, z1, t, u + id, v + id, iw, ih, 0, mirror);
    face(m, x1, y1, z0, x0, y1, z0, x1, y0, z0, t, u + 2 * id + iw, v + id, iw, ih, 0, mirror);
    if (mirror) {  // the left limb of an old skin: the right one's sides swap
        face(m, x1, y1, z1, x1, y1, z0, x1, y0, z1, t, u, v + id, id, ih, 0, true);
        face(m, x0, y1, z0, x0, y1, z1, x0, y0, z0, t, u + id + iw, v + id, id, ih, 0, true);
    } else {
        face(m, x0, y1, z0, x0, y1, z1, x0, y0, z0, t, u, v + id, id, ih, 0, false);
        face(m, x1, y1, z1, x1, y1, z0, x1, y0, z1, t, u + id + iw, v + id, id, ih, 0, false);
    }
    face(m, x0, y1, z0, x1, y1, z0, x0, y1, z1, t, u + id, v, iw, id, 0, mirror);
    face(m, x0, y0, z1, x1, y0, z1, x0, y0, z0, t, u + id + iw, v, iw, id, 0, mirror);
}

void VelloraCharacter::face(const Mat& m, float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz, const QImage* tex, int su,
                            int sv, int sw, int sh, QRgb solid, bool mirror)
{
    const float AX = m[0] * ax + m[1] * ay + m[2] * az + m[3], AY = m[4] * ax + m[5] * ay + m[6] * az + m[7], AZ = m[8] * ax + m[9] * ay + m[10] * az + m[11];
    const float BX = m[0] * bx + m[1] * by + m[2] * bz + m[3], BY = m[4] * bx + m[5] * by + m[6] * bz + m[7], BZ = m[8] * bx + m[9] * by + m[10] * bz + m[11];
    const float CX = m[0] * cx + m[1] * cy + m[2] * cz + m[3], CY = m[4] * cx + m[5] * cy + m[6] * cz + m[7], CZ = m[8] * cx + m[9] * cy + m[10] * cz + m[11];
    const float ex = BX - AX, ey = BY - AY, ez = BZ - AZ, fx = CX - AX, fy = CY - AY, fz = CZ - AZ;
    const float nx = fy * ez - fz * ey, ny = fz * ex - fx * ez, nz = fx * ey - fy * ex;
    const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
    if (len < 1e-6f || nz / len <= 0.001f) {
        return;  // it faces away from us
    }
    const float light = (nx * -0.35f + ny * 0.8f + nz * 0.5f) / len / 0.995f;
    float bright = 0.56f + 0.46f * std::max(0.f, light);
    if (ny / len < -0.5f) {
        bright *= 0.85f;
    }
    const int b256 = qRound(std::min(1.08f, bright) * 256);
    const float sAx = m_cx + AX * m_scale, sAy = m_cy - AY * m_scale;
    const float e1x = ex * m_scale, e1y = -ey * m_scale, e2x = fx * m_scale, e2y = -fy * m_scale;
    const float det = e1x * e2y - e1y * e2x;
    if (std::abs(det) < 1e-4f) {
        return;
    }
    const float minX = std::min({ sAx, sAx + e1x, sAx + e2x, sAx + e1x + e2x }), maxX = std::max({ sAx, sAx + e1x, sAx + e2x, sAx + e1x + e2x });
    const float minY = std::min({ sAy, sAy + e1y, sAy + e2y, sAy + e1y + e2y }), maxY = std::max({ sAy, sAy + e1y, sAy + e2y, sAy + e1y + e2y });
    const int x0 = std::max(0, int(std::floor(minX))), x1 = std::min(m_bufW - 1, int(std::ceil(maxX)));
    const int y0 = std::max(0, int(std::floor(minY))), y1 = std::min(m_bufH - 1, int(std::ceil(maxY)));
    const float dsdx = e2y / det, dsdy = -e2x / det, dtdx = -e1y / det, dtdy = e1x / det;
    const float dz1 = BZ - AZ, dz2 = CZ - AZ;
    const float eps = 0.02f;
    const int tw = tex ? tex->width() : 0, th = tex ? tex->height() : 0;
    for (int py = y0; py <= y1; py++) {
        const float ry = py + 0.5f - sAy, rx = x0 + 0.5f - sAx;
        float s = rx * dsdx + ry * dsdy, t = rx * dtdx + ry * dtdy;
        const int row = py * m_bufW;
        for (int px = x0; px <= x1; px++, s += dsdx, t += dtdx) {
            if (s < -eps || s >= 1 + eps || t < -eps || t >= 1 + eps) {
                continue;
            }
            const float z = AZ + s * dz1 + t * dz2;
            const int i = row + px;
            if (z <= m_depth[i]) {
                continue;
            }
            QRgb c;
            if (tex) {
                int tu = int(std::clamp(s, 0.f, 0.999f) * sw), tv = int(std::clamp(t, 0.f, 0.999f) * sh);
                if (mirror) {
                    tu = sw - 1 - tu;
                }
                const int ux = su + tu, vy = sv + tv;
                if (ux < 0 || vy < 0 || ux >= tw || vy >= th) {
                    continue;
                }
                c = reinterpret_cast<const QRgb*>(tex->constScanLine(vy))[ux];
            } else {
                c = solid;
            }
            if (qAlpha(c) < 128) {
                continue;
            }
            m_color[i] = qRgb(std::min(255, (qRed(c) * b256) >> 8), std::min(255, (qGreen(c) * b256) >> 8), std::min(255, (qBlue(c) * b256) >> 8));
            m_depth[i] = z;
        }
    }
}

void VelloraCharacter::solidBox(const Mat& m, const Box3& b)
{
    face(m, b.x0, b.y1, b.z1, b.x1, b.y1, b.z1, b.x0, b.y0, b.z1, nullptr, 0, 0, 1, 1, b.color, false);
    face(m, b.x1, b.y1, b.z0, b.x0, b.y1, b.z0, b.x1, b.y0, b.z0, nullptr, 0, 0, 1, 1, b.color, false);
    face(m, b.x0, b.y1, b.z0, b.x0, b.y1, b.z1, b.x0, b.y0, b.z0, nullptr, 0, 0, 1, 1, b.color, false);
    face(m, b.x1, b.y1, b.z1, b.x1, b.y1, b.z0, b.x1, b.y0, b.z1, nullptr, 0, 0, 1, 1, b.color, false);
    face(m, b.x0, b.y1, b.z0, b.x1, b.y1, b.z0, b.x0, b.y1, b.z1, nullptr, 0, 0, 1, 1, b.color, false);
    face(m, b.x0, b.y0, b.z1, b.x1, b.y0, b.z1, b.x0, b.y0, b.z0, nullptr, 0, 0, 1, 1, b.color, false);
}

/// an item's 3D shape from its pixels: around the head, plates on the clothes, on the back, beside the player
QList<VelloraCharacter::Box3> VelloraCharacter::build(const Cosmetics::Item& item, const QString& slot)
{
    QList<Box3> out;
    const bool ring = item.id == "crown" || item.id == "devil_crown" || item.id == "flower_crown" || item.id == "halo";
    bool hasFront = false, hasBack = false;
    for (const auto& l : item.layers) {
        hasFront |= l.view != "back";
        hasBack |= l.view == "back";
    }
    auto add = [&out](int part, float x0, float y0, float z0, float x1, float y1, float z1, QRgb c) { out.append({ part, x0, y0, z0, x1, y1, z1, c }); };
    for (const auto& layer : item.layers) {
        const bool backView = layer.view == "back";
        for (int j = 0; j < layer.rows.size(); j++) {
            const QString& row = layer.rows[j];
            int i = 0;
            while (i < row.size()) {
                const QChar ch = row[i];
                if (!filled(layer, ch)) {
                    i++;
                    continue;
                }
                int end = i;
                while (end < row.size() && row[end] == ch) {
                    end++;
                }
                int segStart = i, segEnd = end;
                while (segStart > 0 && filled(layer, row[segStart - 1])) {
                    segStart--;
                }
                while (segEnd < row.size() && filled(layer, row[segEnd])) {
                    segEnd++;
                }
                const QRgb c = layer.colors.value(ch).rgba();
                const float gx0 = float(layer.x + i), gx1 = float(layer.x + end), gy = float(layer.y + j);
                const int segWidth = segEnd - segStart;
                const bool segLeft = segStart == i, segRight = segEnd == end;
                float x0 = gx0 - 8, x1 = gx1 - 8, frontGx = (gx0 + gx1) / 2;
                if (backView) {
                    x0 = 8 - gx1;
                    x1 = 8 - gx0;
                    frontGx = 16 - frontGx;
                }
                const float y1 = 32 - gy, y0 = y1 - 1;
                if (slot == Cosmetics::Head) {
                    const bool onFace = frontGx >= 4 && frontGx < 12 && gy >= 0 && gy < 8;
                    if (onFace || gy >= 8) {
                        if (backView) {
                            add(HEAD, x0, y0, -4.65f, x1, y1, -4.05f, c);
                        } else {
                            add(HEAD, x0, y0, 4.05f, x1, y1, 4.65f, c);
                        }
                    } else if (!backView) {
                        if (segWidth >= 6) {
                            if (ring) {
                                add(HEAD, x0, y0, -4.7f, x1, y1, -3.7f, c);
                                add(HEAD, x0, y0, 3.7f, x1, y1, 4.7f, c);
                                if (segLeft) {
                                    add(HEAD, x0, y0, -4.7f, x0 + 1, y1, 4.7f, c);
                                }
                                if (segRight) {
                                    add(HEAD, x1 - 1, y0, -4.7f, x1, y1, 4.7f, c);
                                }
                            } else {
                                add(HEAD, x0, y0, -4.7f, x1, y1, 4.7f, c);
                            }
                        } else {
                            add(HEAD, x0, y0, -1.2f, x1, y1, 1.2f, c);
                        }
                    }
                } else if (slot == Cosmetics::Body) {
                    int part = BODY;
                    float zf = 2.05f, zb = 2.65f;
                    if (gy < 8) {
                        part = HEAD;
                        zf = 4.05f;
                        zb = 4.65f;
                    } else if (gy >= 20) {
                        part = frontGx < 8 ? RLEG : LLEG;
                    } else if (frontGx < 4) {
                        part = RARM;
                    } else if (frontGx >= 12) {
                        part = LARM;
                    }
                    if (backView) {
                        add(part, x0, y0, -zb, x1, y1, -zf, c);
                    } else {
                        add(part, x0, y0, zf, x1, y1, zb, c);
                    }
                } else if (slot == Cosmetics::Back) {
                    if (item.id.contains("wings")) {
                        if (!(backView && hasFront)) {
                            add((x0 + x1) / 2 < 0 ? RWING : LWING, x0, y0, -3.2f, x1, y1, -2.5f, c);
                        }
                    } else if (item.id.contains("pack")) {
                        if (backView || !hasBack || frontGx < 4 || frontGx >= 12) {
                            add(BODY, x0, y0, -5.2f, x1, y1, -2.05f, c);
                        }
                    } else if (item.id.contains("cape")) {
                        if (backView || !hasBack) {
                            add(CAPE, x0, y0, -3.1f, x1, y1, -2.5f, c);
                        }
                    } else if (backView || !hasBack || frontGx < 4 || frontGx >= 12) {
                        add(BODY, x0, y0, -3.2f, x1, y1, -2.3f, c);
                    }
                } else if (slot == Cosmetics::Hand && !backView) {
                    add(RARM, x0, y0, 2.1f, x1, y1, 2.9f, c);  // held in the right hand, in front of it: it moves with the arm
                } else if (slot == Cosmetics::Pet && !backView) {
                    add(WORLD, x0 + 1, y0, 1.5f, x1 + 1, y1, 4.5f, c);
                }
                i = end;
            }
        }
    }
    return out;
}

void VelloraCharacter::drawItem(const Cosmetics::Item* item, const QString& slot, double time)
{
    if (!item) {
        return;
    }
    static QHash<QString, QList<Box3>> meshes;
    auto it = meshes.find(item->id);
    if (it == meshes.end()) {
        it = meshes.insert(item->id, build(*item, slot));
    }
    float bob = 0;
    const QString anim = item->layers.isEmpty() ? QString() : item->layers.first().anim;
    if (m_animated && anim == "bob") {
        bob = std::sin(time * 2) * 0.5f;
    } else if (m_animated && anim == "hop") {
        bob = std::abs(std::sin(time * 3)) * 2;
    } else if (m_animated && anim == "float") {
        bob = std::sin(time * 2) * 1.2f;
    }
    for (const auto& b : *it) {
        const Mat m = bob != 0 ? mul(m_mats[b.part], translate(0, bob, 0)) : m_mats[b.part];
        solidBox(m, b);
    }
}

void VelloraCharacter::dot(float sx, float sy, float z, int size, QColor color, double alpha)
{
    const int a = qRound(color.alpha() * std::min(1.0, alpha));
    const int x0 = qRound(sx - size / 2.0), y0 = qRound(sy - size / 2.0);
    for (int y = y0; y < y0 + size; y++) {
        for (int x = x0; x < x0 + size; x++) {
            if (x < 0 || y < 0 || x >= m_bufW || y >= m_bufH) {
                continue;
            }
            const int i = y * m_bufW + x;
            if (z < m_depth[i]) {
                continue;
            }
            const QRgb under = m_color[i];
            if (qAlpha(under) == 0) {
                m_color[i] = qRgba(color.red(), color.green(), color.blue(), a);
            } else {
                auto blend = [a](int u, int o) { return (u * (255 - a) + o * a) / 255; };
                m_color[i] = qRgb(blend(qRed(under), color.red()), blend(qGreen(under), color.green()), blend(qBlue(under), color.blue()));
            }
        }
    }
}

void VelloraCharacter::drawParticles(const Cosmetics::Item* item, double time)
{
    if (!item || item->particleColors.isEmpty()) {
        return;
    }
    for (int i = 0; i < item->particleCount; i++) {
        const QColor color = item->particleColors[i % item->particleColors.size()];
        const double speed = 4 + unitHash(i, 1) * 5, phase = unitHash(i, 2) * 40;
        double x, y, z, alpha;
        if (item->motion == "orbit") {
            const double angle = time * 1.4 + i * std::numbers::pi * 2 / item->particleCount;
            x = std::cos(angle) * 11;
            z = std::sin(angle) * 11;
            y = 16 + std::sin(time * 2 + i) * 7;
            alpha = 1;
        } else if (item->motion == "fall") {
            x = -14 + unitHash(i, 3) * 28;
            z = -10 + unitHash(i, 6) * 20;
            y = 40 - std::fmod(time * speed + phase, 44.0);
            alpha = 0.85;
        } else if (item->motion == "spark") {
            x = -12 + unitHash(i, 3) * 24;
            z = -8 + unitHash(i, 6) * 16;
            y = 2 + unitHash(i, 4) * 36;
            alpha = std::max(0.0, std::sin(time * (2 + unitHash(i, 5) * 3) + phase));
        } else {
            x = -10 + unitHash(i, 3) * 20;
            z = -7 + unitHash(i, 6) * 14;
            const double travel = std::fmod(time * speed + phase, 40.0);
            y = travel - 2;
            alpha = std::min(1.0, travel / 6) * std::max(0.0, 1 - travel / 40);
        }
        if (alpha <= 0.05) {
            continue;
        }
        float depth = 0;
        const QPointF p = project(m_mats[WORLD], float(x), float(y), float(z), &depth);
        const int size =
            std::max(1, qRound(m_scale * (item->shape == "heart" ? 0.45f : (item->shape == "note" || item->shape == "star") ? 1.2f : 0.8f)));
        if (item->shape == "heart") {
            const int heart[][2] = { { 0, 0 }, { 2, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 }, { 1, 2 } };
            for (const auto& h : heart) {
                dot(float(p.x()) + (h[0] - 1) * size, float(p.y()) + (h[1] - 1) * size, depth, size, color, alpha);
            }
        } else if (item->shape == "drop") {
            dot(float(p.x()), float(p.y()), depth, size, color, alpha);
            dot(float(p.x()), float(p.y()) + size, depth, size, color, alpha);
        } else if (item->shape == "note") {
            const int q = std::max(1, size / 2);
            for (int k = 0; k < 4; k++) {
                dot(float(p.x()) + q, float(p.y()) - k * q, depth, q, color, alpha);
            }
            dot(float(p.x()) + 2 * q, float(p.y()) - 3 * q, depth, q, color, alpha);
            dot(float(p.x()), float(p.y()), depth, q * 2, color, alpha);
        } else if (item->shape == "star") {
            const int q = std::max(1, size / 2);
            const int offsets[][2] = { { 0, 0 }, { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };
            for (const auto& o : offsets) {
                dot(float(p.x()) + o[0] * q, float(p.y()) + o[1] * q, depth, q, color, alpha);
            }
        } else {
            dot(float(p.x()), float(p.y()), depth, size, color, alpha);
        }
    }
}

void VelloraCharacter::paintItems(QPainter& painter, const QRectF& target, const QString& head, const QString& body, const QString& back,
                                  const QString& pet, const QString& emote, const QString& effect, double time, const QString& hand)
{
    const double u = std::min(target.width() / 32.0, target.height() / 40.0);
    Pen pen{ painter, target.left() + (target.width() - 32 * u) / 2 + 8 * u, target.top() + (target.height() - 40 * u) / 2 + 8 * u, u };
    drawFlatItem(pen, Cosmetics::find(back), false, time);
    drawSilhouette(pen, emote == "wave" || emote == "cheer", emote == "cheer", emote == "jump" ? -3 : emote == "bow" ? 1 : 0, false);
    drawFlatItem(pen, Cosmetics::find(body), false, time);
    drawFlatItem(pen, Cosmetics::find(head), false, time);
    drawFlatItem(pen, Cosmetics::find(pet), false, time);
    drawFlatItem(pen, Cosmetics::find(hand), false, time);
    drawFlatParticles(pen, Cosmetics::find(effect), time);
}


void VelloraCharacter::paintEvent(QPaintEvent*)
{
    const int w = width(), h = height();
    if (w <= 0 || h <= 0) {
        return;
    }
    const int k = std::max(1, int(std::ceil(h / 360.0)));
    const int bw = std::max(1, w / k), bh = std::max(1, h / k);
    if (bw != m_bufW || bh != m_bufH) {
        m_bufW = bw;
        m_bufH = bh;
        m_color.assign(size_t(bw) * bh, 0);
        m_depth.assign(size_t(bw) * bh, -1e9f);
    }
    const qint64 now = m_clock.elapsed();
    const double time = now / 1000.0;
    const double emoteTime = (now - m_emoteStart) / 1000.0;
    const bool emoting = !m_emote.isEmpty() && emoteTime < 2.4;

    // turning: the drag slows down, then the player turns back to face us (or shows the back)
    if (!m_dragging) {
        m_yaw += m_yawSpeed;
        m_yawSpeed *= 0.92f;
        if (std::abs(m_yawSpeed) < 0.002f) {
            m_yawSpeed = 0;
        }
        if (m_yawSpeed == 0 && now - m_lastTouch > 2500) {
            const float target = BASE_YAW + (m_back ? float(std::numbers::pi) : 0.f);
            const float diff = std::atan2(std::sin(target - m_yaw), std::cos(target - m_yaw));
            m_yaw += diff * 0.06f;
            m_pitch += (-0.17f - m_pitch) * 0.06f;
        }
    }

    // the pose
    float breath = std::sin(time * 2) * 0.25f, lift = 0, spin = 0, bow = 0, sway = 0, flipAngle = 0;
    float rArmX = std::sin(time * 1.3) * 0.07f, lArmX = -rArmX;
    float rArmZ = -0.05f - std::sin(time * 2) * 0.02f, lArmZ = -rArmZ;
    float rLegX = 0, lLegX = 0;
    float headYaw = std::sin(time * 0.6) * 0.22f, headPitch = std::sin(time * 0.45) * 0.07f;
    if (!m_animated) {
        breath = rArmX = lArmX = headYaw = headPitch = 0;
    }
    if (emoting) {
        const float p = float(emoteTime / 2.4);
        const float fade = float(std::min(1.0, std::min(emoteTime * 4, (2.4 - emoteTime) * 4)));
        const float et = float(emoteTime);
        if (m_emote == "wave") {
            rArmZ = mixf(rArmZ, -2.55f + std::sin(et * 12) * 0.28f, fade);
            headYaw *= 1 - fade;
        } else if (m_emote == "cheer") {
            const float wob = std::sin(et * 14) * 0.2f;
            rArmZ = mixf(rArmZ, -2.7f + wob, fade);
            lArmZ = mixf(lArmZ, 2.7f - wob, fade);
            lift = std::abs(std::sin(et * 7)) * 1.2f * fade;
        } else if (m_emote == "jump") {
            const float j = std::max(0.f, float(std::sin(et * std::numbers::pi / 0.6)));
            lift = j * 7;
            rArmZ = mixf(rArmZ, -0.9f, j);
            lArmZ = mixf(lArmZ, 0.9f, j);
            rLegX = -j * 0.5f;
            lLegX = j * 0.3f;
        } else if (m_emote == "dance") {
            const float b = std::sin(et * 9);
            sway = b * 0.35f * fade;
            rArmX = mixf(rArmX, -1.6f + b * 0.9f, fade);
            lArmX = mixf(lArmX, -1.6f - b * 0.9f, fade);
            rArmZ = mixf(rArmZ, -0.4f, fade);
            lArmZ = mixf(lArmZ, 0.4f, fade);
            rLegX = b * 0.35f * fade;
            lLegX = -b * 0.35f * fade;
            lift = std::abs(std::cos(et * 9)) * fade;
        } else if (m_emote == "spin") {
            const float e = std::min(1.f, p * 1.6f);
            spin = float(std::numbers::pi * 2) * (e * e * (3 - 2 * e));
            rArmZ = mixf(rArmZ, -1.2f, fade);
            lArmZ = mixf(lArmZ, 1.2f, fade);
        } else if (m_emote == "bow") {
            bow = float(std::sin(std::min(1.f, p * 1.4f) * std::numbers::pi)) * 0.6f;
            rArmX = mixf(rArmX, -0.4f, fade);
            lArmX = mixf(lArmX, 0.3f, fade);
        } else if (m_emote == "clap") {
            const float c = std::abs(std::sin(et * 9));
            rArmX = mixf(rArmX, -1.25f, fade);
            lArmX = mixf(lArmX, -1.25f, fade);
            rArmZ = mixf(rArmZ, 0.15f + c * 0.35f, fade);
            lArmZ = mixf(lArmZ, -0.15f - c * 0.35f, fade);
        } else if (m_emote == "salute") {
            rArmZ = mixf(rArmZ, -2.35f, fade);
            rArmX = mixf(rArmX, -0.55f, fade);
            headPitch = mixf(headPitch, -0.08f, fade);
            headYaw *= 1 - fade;
        } else if (m_emote == "shrug") {
            const float u = std::abs(std::sin(et * 3));
            rArmZ = mixf(rArmZ, -0.8f - u * 0.2f, fade);
            lArmZ = mixf(lArmZ, 0.8f + u * 0.2f, fade);
            rArmX = mixf(rArmX, -0.6f, fade);
            lArmX = mixf(lArmX, -0.6f, fade);
            headPitch = mixf(headPitch, 0.12f, fade);
            headYaw = mixf(headYaw, std::sin(et * 3) * 0.25f, fade);
        } else if (m_emote == "point") {
            rArmX = mixf(rArmX, -1.6f, fade);
            rArmZ = mixf(rArmZ, 0.05f, fade);
            headYaw *= 1 - fade;
        } else if (m_emote == "dab") {
            rArmZ = mixf(rArmZ, -2.3f, fade);
            rArmX = mixf(rArmX, -0.3f, fade);
            lArmX = mixf(lArmX, -1.3f, fade);
            lArmZ = mixf(lArmZ, -0.7f, fade);
            headPitch = mixf(headPitch, 0.45f, fade);
            headYaw = mixf(headYaw, 0.45f, fade);
        } else if (m_emote == "flip") {
            const float e = std::min(1.f, p * 1.5f);
            const float arc = float(std::sin(e * std::numbers::pi));
            lift = arc * 9;
            flipAngle = -float(std::numbers::pi * 2) * (e * e * (3 - 2 * e));
            rArmZ = mixf(rArmZ, -2.6f, arc);
            lArmZ = mixf(lArmZ, 2.6f, arc);
        } else if (m_emote == "think") {
            rArmX = mixf(rArmX, -2.0f, fade);
            rArmZ = mixf(rArmZ, 0.55f, fade);
            headPitch = mixf(headPitch, 0.18f, fade);
            headYaw = mixf(headYaw, -0.2f, fade);
        } else if (m_emote == "punch") {
            const float a = std::max(0.f, float(std::sin(et * 10))), b = std::max(0.f, float(-std::sin(et * 10)));
            rArmX = mixf(rArmX, -0.6f - a, fade);
            lArmX = mixf(lArmX, -0.6f - b, fade);
            sway = (a - b) * 0.15f * fade;
        }
    }
    if (m_animated && !emoting && !m_dragging) {
        rLegX = std::sin(time * 1.3) * 0.03f;
        lLegX = -rLegX;
    }

    Mat camera = mul(rot(0, 0, 0, m_pitch, 0, 0), rot(0, 0, 0, 0, m_yaw + spin + sway * 0.4f, 0));
    camera = mul(camera, translate(0, lift + breath * 0.3f, 0));
    if (flipAngle != 0) {
        camera = mul(camera, rot(0, 16, 0, flipAngle, 0, 0));  // a backflip about the middle of the body
    }
    const Mat upper = mul(camera, rot(0, 12, 0, bow, sway * 0.6f, 0));
    const float armW = m_slim ? 3 : 4;
    m_mats[HEAD] = mul(upper, rot(0, 24, 0, headPitch + bow * 0.5f, headYaw, 0));
    m_mats[BODY] = upper;
    m_mats[RARM] = mul(upper, rot(-4 - armW / 2, 22, 0, rArmX, 0, rArmZ));
    m_mats[LARM] = mul(upper, rot(4 + armW / 2, 22, 0, lArmX, 0, lArmZ));
    m_mats[RLEG] = mul(camera, rot(-2, 12, 0, rLegX, 0, 0));
    m_mats[LLEG] = mul(camera, rot(2, 12, 0, lLegX, 0, 0));
    m_mats[WORLD] = camera;
    const float flap = m_animated ? float(0.35 + 0.35 * std::sin(time * 2.4)) : 0.4f;
    m_mats[RWING] = mul(upper, rot(-1, 20, -2.6f, 0, -flap, 0));
    m_mats[LWING] = mul(upper, rot(1, 20, -2.6f, 0, flap, 0));
    const float capeTilt = 0.12f + (m_animated ? float(std::sin(time * 1.7)) * 0.04f : 0.f) + lift * 0.03f + std::abs(m_yawSpeed) * 2;
    m_mats[CAPE] = mul(upper, rot(0, 24, -2.9f, capeTilt, 0, 0));

    if (m_closeUp) {
        m_scale = std::min(m_bufW / 24.f, m_bufH / 19.f);
        m_cx = m_bufW / 2.f;
        m_cy = m_bufH * 0.55f + 25 * m_scale;
    } else {
        m_scale = std::min(m_bufW / 34.f, m_bufH / 44.f);
        m_cx = m_bufW / 2.f;
        m_cy = m_bufH * 0.5f + 15 * m_scale;
    }
    std::fill(m_color.begin(), m_color.end(), 0u);
    std::fill(m_depth.begin(), m_depth.end(), -1e9f);

    const QImage& tex = m_skin.isNull() ? shadowSkin() : m_skin;
    const bool modern = tex.height() >= 64;
    drawBox(m_mats[HEAD], -4, 24, -4, 8, 8, 8, 0, 0, tex, 0);
    drawBox(m_mats[BODY], -4, 12, -2, 8, 12, 4, 16, 16, tex, 0);
    drawBox(m_mats[RARM], -4 - armW, 12, -2, armW, 12, 4, 40, 16, tex, 0);
    drawBox(m_mats[RLEG], -4, 0, -2, 4, 12, 4, 0, 16, tex, 0);
    if (modern) {
        drawBox(m_mats[LARM], 4, 12, -2, armW, 12, 4, 32, 48, tex, 0);
        drawBox(m_mats[LLEG], 0, 0, -2, 4, 12, 4, 16, 48, tex, 0);
        drawBox(m_mats[HEAD], -4, 24, -4, 8, 8, 8, 32, 0, tex, 0.5f);
        drawBox(m_mats[BODY], -4, 12, -2, 8, 12, 4, 16, 32, tex, 0.25f);
        drawBox(m_mats[RARM], -4 - armW, 12, -2, armW, 12, 4, 40, 32, tex, 0.25f);
        drawBox(m_mats[LARM], 4, 12, -2, armW, 12, 4, 48, 48, tex, 0.25f);
        drawBox(m_mats[RLEG], -4, 0, -2, 4, 12, 4, 0, 32, tex, 0.25f);
        drawBox(m_mats[LLEG], 0, 0, -2, 4, 12, 4, 0, 48, tex, 0.25f);
    } else {
        drawBox(m_mats[LARM], 4, 12, -2, armW, 12, 4, 40, 16, tex, 0, true);
        drawBox(m_mats[LLEG], 0, 0, -2, 4, 12, 4, 0, 16, tex, 0, true);
        drawBox(m_mats[HEAD], -4, 24, -4, 8, 8, 8, 32, 0, tex, 0.5f);
    }
    const QString backItem = m_showItems ? Cosmetics::worn(Cosmetics::Back) : QString();
    if (!m_cape.isNull() && !backItem.contains("cape") && backItem != "backpack" && backItem != "jetpack") {
        drawBox(mul(m_mats[CAPE], rot(0, 24, -2.9f, 0, float(std::numbers::pi), 0)), -5, 8, -3.4f, 10, 16, 1, 0, 0, m_cape, 0);
    }
    if (m_showItems) {
        drawItem(Cosmetics::find(backItem), Cosmetics::Back, time);
        drawItem(Cosmetics::find(Cosmetics::worn(Cosmetics::Body)), Cosmetics::Body, time);
        drawItem(Cosmetics::find(Cosmetics::worn(Cosmetics::Head)), Cosmetics::Head, time);
        drawItem(Cosmetics::find(Cosmetics::worn(Cosmetics::Pet)), Cosmetics::Pet, time);
        drawItem(Cosmetics::find(Cosmetics::worn(Cosmetics::Hand)), Cosmetics::Hand, time);
        drawParticles(Cosmetics::find(Cosmetics::worn(Cosmetics::Effect)), time);
    }

    QPainter painter(this);
    // the crimson light on the floor, then the frame, sharp
    const QPointF foot = project(m_mats[WORLD], 0, 0, 0) * k + QPointF(0, lift * m_scale * k);
    const double radius = 16 * m_scale * k;
    QRadialGradient glow(foot, radius);
    glow.setColorAt(0, QColor(232, 54, 79, 112));
    glow.setColorAt(1, QColor(232, 54, 79, 0));
    painter.save();
    painter.translate(foot);
    painter.scale(1, 0.25);
    painter.translate(-foot);
    painter.setPen(Qt::NoPen);
    painter.setBrush(glow);
    painter.drawEllipse(foot, radius, radius);
    painter.restore();
    const QImage frame(reinterpret_cast<const uchar*>(m_color.data()), m_bufW, m_bufH, m_bufW * 4, QImage::Format_ARGB32);
    painter.drawImage(QRect(0, 0, m_bufW * k, m_bufH * k), frame);
}
