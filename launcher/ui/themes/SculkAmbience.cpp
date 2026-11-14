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
#include "SculkAmbience.h"

#include <QHash>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QRegion>
#include <QWidget>
#include <cmath>
#include <numbers>

#include "Application.h"
#include "SculkTheme.h"
#include "settings/SettingsObject.h"

SculkAmbience::SculkAmbience(QWidget* target, int particles) : QObject(target), m_target(target)
{
    m_clock.start();
    m_seed = quint32(reinterpret_cast<quintptr>(target) >> 4) | 1;
    for (int i = 0; i < particles; i++) {
        m_particles.append(spawn(true));
    }
    m_timer.setInterval(40);
    connect(&m_timer, &QTimer::timeout, this, &SculkAmbience::tick);
    m_timer.start();
}

bool SculkAmbience::enabled()
{
    return SculkTheme::isActive() && APPLICATION->settings()->get("SculkBackgroundAnimation").toBool();
}

SculkAmbience::Particle SculkAmbience::spawn(bool anywhere)
{
    // xorshift, so every widget gets its own but repeatable set of particles
    auto next = [this] {
        m_seed ^= m_seed << 13;
        m_seed ^= m_seed >> 17;
        m_seed ^= m_seed << 5;
        return (m_seed % 10000) / 10000.0;
    };
    Particle p;
    p.x = next();
    p.y = anywhere ? next() : 1.0 + next() * 0.1;
    p.speed = 0.025 + next() * 0.05;
    p.sway = next() * 6.28;
    p.size = 4 + int(next() * 4) * 2;
    return p;
}

double SculkAmbience::pulse(double t)
{
    // the heartbeat of the Warden: a strong and a weak beat, then a pause
    const double beatTime = std::fmod(t, 2.4);
    auto beat = [](double x, double at) { return std::exp(-std::pow((x - at) / 0.11, 2)); };
    return beat(beatTime, 0.15) + 0.55 * beat(beatTime, 0.5);
}

QRectF SculkAmbience::particleRect(const Particle& p, double t) const
{
    const double x = m_lastRect.left() + p.x * m_lastRect.width() + std::sin(t * 1.3 + p.sway) * 6.0;
    const double y = m_lastRect.top() + p.y * m_lastRect.height();
    const double r = p.size * 2.2 + 2;
    return QRectF(x - r, y - r, 2 * r, 2 * r);
}

void SculkAmbience::tick()
{
    const qint64 now = m_clock.elapsed();
    const double dt = std::min(0.1, (now - m_lastTick) / 1000.0);
    m_lastTick = now;
    if (!m_target || !m_target->isVisible() || !enabled()) {
        return;
    }
    const double t = now / 1000.0;
    if (SculkTheme::isLunar()) {
        tickLunar(t);
        return;
    }
    const double previous = (now - dt * 1000.0) / 1000.0;
    // while the veins pulse everything changes; between the beats only the particles move
    const double currentPulse = pulse(t);
    const bool fullRepaint = m_lastRect.isEmpty() || std::abs(currentPulse - m_lastPulse) > 0.004;
    QRegion dirty;
    for (auto& p : m_particles) {
        if (!fullRepaint) {
            dirty += particleRect(p, previous).toAlignedRect();
        }
        p.y -= p.speed * dt;
        if (p.y < -0.05) {
            p = spawn(false);
        }
        if (!fullRepaint) {
            dirty += particleRect(p, t).toAlignedRect();
        }
    }
    if (fullRepaint) {
        m_target->update();
    } else {
        m_target->update(dirty);
    }
}

namespace {
// the soft glow around a particle, rendered once per color and size: blitting it is much cheaper than an antialiased ellipse
const QPixmap& haloSprite(const QColor& glow, int radius)
{
    static QHash<QString, QPixmap> cache;
    const QString key = glow.name() + QString::number(radius);
    auto it = cache.find(key);
    if (it == cache.end()) {
        QPixmap sprite(radius * 2, radius * 2);
        sprite.fill(Qt::transparent);
        QPainter p(&sprite);
        p.setRenderHint(QPainter::Antialiasing, true);
        QRadialGradient gradient(QPointF(radius, radius), radius);
        QColor inner = glow;
        inner.setAlphaF(0.45);
        QColor outer = glow;
        outer.setAlphaF(0.0);
        gradient.setColorAt(0, inner);
        gradient.setColorAt(1, outer);
        p.setPen(Qt::NoPen);
        p.setBrush(gradient);
        p.drawEllipse(QRectF(0, 0, radius * 2, radius * 2));
        it = cache.insert(key, sprite);
    }
    return *it;
}
}  // namespace

void SculkAmbience::paint(QPainter* painter, const QRect& rect, const QPoint& textureOffset, const QColor& glow, int pixelSize)
{
    if (!enabled() || rect.isEmpty()) {
        return;
    }
    const double t = m_clock.elapsed() / 1000.0;
    if (SculkTheme::isLunar()) {
        m_lastRect = rect;
        paintLunar(painter, t);
        return;
    }
    painter->save();

    m_lastRect = rect;
    m_lastPulse = pulse(t);
    painter->setOpacity(0.08 + 0.42 * m_lastPulse);
    painter->drawTiledPixmap(rect, SculkTheme::veinTexture(glow, pixelSize), textureOffset);

    // sculk charge: square pixels that fade in at the bottom, sway and fade out at the top
    painter->setPen(Qt::NoPen);
    for (const auto& p : m_particles) {
        const double fade = std::clamp(std::min((1.05 - p.y) * 4.0, p.y * 3.0), 0.0, 1.0);
        if (fade <= 0) {
            continue;
        }
        const QPointF center = particleRect(p, t).center();
        const double x = center.x();
        const double y = center.y();
        const double flicker = 0.75 + 0.25 * std::sin(t * 5.0 + p.sway * 3.0);
        const int radius = int(p.size * 2.2);
        painter->setOpacity(0.7 * fade * flicker);
        painter->drawPixmap(QPointF(x - radius, y - radius), haloSprite(glow, radius));
        painter->setOpacity(0.95 * fade * flicker);
        painter->fillRect(QRect(int(std::round(x - p.size / 2.0)), int(std::round(y - p.size / 2.0)), p.size, p.size), glow.lighter(125));
    }
    painter->restore();
}

/* ---------------------------------------------------------------- Nyxora: the neon rain (the old "lunar" theme id) */

namespace {
quint32 sceneHash(int a, int b, int salt)
{
    quint32 h = quint32(a) * 374761393u + quint32(b) * 668265263u + quint32(salt) * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
}  // namespace

void SculkAmbience::tickLunar([[maybe_unused]] double t)
{
    // Nyxora: the neon rain moves everywhere at once, so the whole picture is repainted every frame
    m_target->update();
}

void SculkAmbience::paintLunar(QPainter* painter, double t)
{
    // Nyxora Launcher: neon rain falling slantwise, small splashes on the ground and now and then a faint violet flash,
    // the same as the Android app (android/src/.../RainDrawable.java)
    const QRectF area(m_lastRect);
    const double w = area.width(), h = area.height();
    if (w <= 0 || h <= 0) {
        return;
    }
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    const double cycle = std::fmod(t, 9.5);
    if (cycle < 0.35) {
        const double flash = (1 - cycle / 0.35) * (cycle < 0.08 ? cycle / 0.08 : 1.0);
        painter->fillRect(area, QColor(134, 72, 255, int(40 * flash)));
    }
    constexpr double wind = 0.16;
    const int drops = int(std::min(420.0, w * h / 900.0));
    for (int i = 0; i < drops; i++) {
        const bool near = sceneHash(i, 1, 51) % 100 < 30;
        const double speed = h * (near ? 1.5 : 0.95) * (0.85 + (sceneHash(i, 2, 51) % 1000) / 1000.0 * 0.3);
        const double length = (near ? 26.0 : 14.0) * (0.7 + (sceneHash(i, 3, 51) % 1000) / 1000.0 * 0.6);
        const double span = h + length;
        const double y = std::fmod(t * speed + (sceneHash(i, 4, 51) % 1000) / 1000.0 * span, span) - length;
        const double x = (sceneHash(i, 5, 51) % 10000) / 10000.0 * (w + h * wind) - h * wind + y * wind;
        const int alpha = near ? 150 + int(sceneHash(i, 6, 51) % 90) : 60 + int(sceneHash(i, 6, 51) % 70);
        // the colours of the icon: two drops in three violet, one blue
        const bool blue = sceneHash(i, 7, 51) % 3 == 0;
        const QColor drop = blue ? QColor(86, near ? 190 : 170, 255, alpha) : QColor(near ? 150 : 130, near ? 100 : 90, 255, alpha);
        painter->setPen(QPen(drop, near ? 1.8 : 1.0, Qt::SolidLine, Qt::RoundCap));
        const QPointF top(area.left() + x, area.top() + y);
        painter->drawLine(top, top + QPointF(length * wind, length));
        const double hit = (y + length) - h;
        if (near && hit > 0 && hit < speed * 0.25) {
            const double k = hit / (speed * 0.25);
            painter->setPen(QPen(QColor(163, 115, 255, int(170 * (1 - k))), 1.2));
            painter->setBrush(Qt::NoBrush);
            const double r = 3 + 9 * k;
            painter->drawEllipse(QPointF(area.left() + x + length * wind, area.bottom() - 4), r, r * 0.3);
        }
    }
    painter->restore();
}

QPixmap SculkAmbience::moonScene(const QSize& size, qreal dpr)
{
    // Nyxora Launcher: a dark navy night with a violet glow from below; the rain is drawn on top by the ambience
    QPixmap pixmap(size * dpr);
    pixmap.setDevicePixelRatio(dpr);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing, true);
    const double w = size.width();
    const double h = size.height();
    QLinearGradient sky(0, 0, 0, h);
    sky.setColorAt(0, QColor(0x02, 0x02, 0x07));
    sky.setColorAt(0.55, QColor(0x06, 0x05, 0x16));
    sky.setColorAt(1, QColor(0x0b, 0x09, 0x2a));
    p.fillRect(QRectF(0, 0, w, h), sky);
    QRadialGradient glow(QPointF(w * 0.5, h * 1.15), std::max(w, h) * 0.75);
    glow.setColorAt(0, QColor(0x5e, 0x30, 0xdb, 0x66));
    glow.setColorAt(0.5, QColor(0x3e, 0x1b, 0x9e, 0x22));
    glow.setColorAt(1, Qt::transparent);
    p.fillRect(QRectF(0, 0, w, h), glow);
    p.end();
    return pixmap;
}
