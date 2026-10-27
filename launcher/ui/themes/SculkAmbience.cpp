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
#include <QPainter>
#include <QRadialGradient>
#include <QRegion>
#include <QWidget>
#include <cmath>

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
