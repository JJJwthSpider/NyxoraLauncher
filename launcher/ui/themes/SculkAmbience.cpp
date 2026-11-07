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

/* ---------------------------------------------------------------- the lunar night */

namespace {
const QColor s_starlight(255, 255, 255);
const QColor s_cometGlow(214, 228, 255);

/// the particles are made for sculk charge: as stars they are a third of the size
int starSize(int particleSize)
{
    return 1 + particleSize / 3;
}

QRectF twinkleRect(const QRect& area, double x, double y, int size)
{
    const double r = size * 3.5 + 3;
    const QPointF c(area.left() + x * area.width(), area.top() + y * area.height());
    return QRectF(c.x() - r, c.y() - r, 2 * r, 2 * r);
}

quint32 sceneHash(int a, int b, int salt)
{
    quint32 h = quint32(a) * 374761393u + quint32(b) * 668265263u + quint32(salt) * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
}  // namespace

QRectF SculkAmbience::cometRect(const Comet& comet, double t) const
{
    const double age = t - comet.born;
    const QPointF head(m_lastRect.left() + comet.start.x() * m_lastRect.width() + comet.velocity.x() * age,
                       m_lastRect.top() + comet.start.y() * m_lastRect.height() + comet.velocity.y() * age);
    const double speed = std::hypot(comet.velocity.x(), comet.velocity.y());
    const QPointF tail = head - comet.velocity / speed * 190.0;
    return QRectF(head, tail).normalized().adjusted(-20, -20, 20, 20);
}

void SculkAmbience::tickLunar(double t)
{
    if (m_lastRect.isEmpty()) {
        m_target->update();
        return;
    }
    // the stars twinkle all the time, so their small squares are repainted every frame
    QRegion dirty;
    for (const auto& p : m_particles) {
        dirty += twinkleRect(m_lastRect, p.x, p.y, starSize(p.size)).toAlignedRect();
    }
    // now and then a comet flies across the sky, only on big enough pictures
    auto next = [this] {
        m_seed ^= m_seed << 13;
        m_seed ^= m_seed >> 17;
        m_seed ^= m_seed << 5;
        return (m_seed % 10000) / 10000.0;
    };
    if (t >= m_nextComet) {
        m_nextComet = t + 4.0 + next() * 8.0;
        if (m_lastRect.width() > 300 && m_comets.size() < 2) {
            const bool toLeft = next() < 0.7;
            Comet comet;
            comet.start = QPointF(toLeft ? 0.45 + next() * 0.55 : next() * 0.5, -0.02 + next() * 0.3);
            comet.velocity = QPointF((toLeft ? -1 : 1) * (360 + next() * 260), 150 + next() * 140);
            comet.born = t;
            comet.life = 1.3 + next() * 0.8;
            m_comets.append(comet);
        }
    }
    const double previous = t - 0.05;
    for (int i = m_comets.size() - 1; i >= 0; i--) {
        const auto& comet = m_comets.at(i);
        dirty += cometRect(comet, std::max(comet.born, previous)).toAlignedRect();
        dirty += cometRect(comet, t).toAlignedRect();
        if (t - comet.born > comet.life + 0.1) {
            m_comets.removeAt(i);
        }
    }
    m_target->update(dirty);
}

void SculkAmbience::paintLunar(QPainter* painter, double t)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // twinkling stars: a soft glow, a bright core and four thin rays that grow when the star flares up
    for (const auto& p : m_particles) {
        const double wave = 0.5 + 0.5 * std::sin(t * (1.2 + p.speed * 30.0) + p.sway);
        const double brightness = 0.15 + 0.85 * wave * wave;
        const int size = starSize(p.size);
        const QPointF c = twinkleRect(m_lastRect, p.x, p.y, size).center();
        const int radius = size * 3;
        painter->setOpacity(0.55 * brightness);
        painter->drawPixmap(QPointF(c.x() - radius, c.y() - radius), haloSprite(s_starlight, radius));
        painter->setOpacity(brightness);
        QColor ray = s_starlight;
        ray.setAlphaF(0.7);
        painter->setPen(QPen(ray, 1.0));
        const double length = size * (1.2 + 1.8 * brightness);
        painter->drawLine(QPointF(c.x() - length, c.y()), QPointF(c.x() + length, c.y()));
        painter->drawLine(QPointF(c.x(), c.y() - length), QPointF(c.x(), c.y() + length));
        painter->setPen(Qt::NoPen);
        painter->setBrush(s_starlight);
        painter->drawEllipse(c, 1.3, 1.3);
    }

    // comets: a glowing head with a tail that fades out behind it
    for (const auto& comet : m_comets) {
        const double age = t - comet.born;
        if (age < 0 || age > comet.life) {
            continue;
        }
        const double fade = std::sin(std::numbers::pi * age / comet.life);
        const QPointF head(m_lastRect.left() + comet.start.x() * m_lastRect.width() + comet.velocity.x() * age,
                           m_lastRect.top() + comet.start.y() * m_lastRect.height() + comet.velocity.y() * age);
        const double speed = std::hypot(comet.velocity.x(), comet.velocity.y());
        const QPointF tail = head - comet.velocity / speed * 190.0 * std::min(1.0, age * 3.0);
        QLinearGradient gradient(head, tail);
        QColor bright = s_cometGlow;
        bright.setAlphaF(0.9 * fade);
        QColor gone = s_cometGlow;
        gone.setAlphaF(0.0);
        gradient.setColorAt(0, bright);
        gradient.setColorAt(1, gone);
        painter->setOpacity(1.0);
        painter->setPen(QPen(QBrush(gradient), 3.2, Qt::SolidLine, Qt::RoundCap));
        painter->drawLine(head, tail);
        painter->setOpacity(fade);
        painter->drawPixmap(QPointF(head.x() - 18, head.y() - 18), haloSprite(s_cometGlow, 18));
        painter->setPen(Qt::NoPen);
        painter->setBrush(Qt::white);
        painter->drawEllipse(head, 2.2, 2.2);
    }
    painter->restore();
}

QPixmap SculkAmbience::moonScene(const QSize& size, qreal dpr)
{
    QPixmap pixmap(size * dpr);
    pixmap.setDevicePixelRatio(dpr);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing, true);
    const double w = size.width();
    const double h = size.height();

    // the night sky, a little lighter towards the horizon
    QLinearGradient sky(0, 0, 0, h);
    sky.setColorAt(0, QColor(0x05, 0x06, 0x0d));
    sky.setColorAt(0.6, QColor(0x0b, 0x0d, 0x1c));
    sky.setColorAt(1, QColor(0x14, 0x17, 0x2c));
    p.fillRect(QRectF(0, 0, w, h), sky);

    // still stars; the twinkling ones are drawn on top by the ambience
    const int stars = int(w * h / 2600);
    p.setPen(Qt::NoPen);
    for (int i = 0; i < stars; i++) {
        const double x = sceneHash(i, 1, 77) % 10000 / 10000.0 * w;
        const double y = sceneHash(i, 2, 77) % 10000 / 10000.0 * h;
        QColor c(255, 255, 255);
        c.setAlphaF(0.2 + (sceneHash(i, 3, 77) % 60) / 100.0);
        p.setBrush(c);
        const double r = sceneHash(i, 4, 77) % 100 < 85 ? 0.6 : 1.0;
        p.drawEllipse(QPointF(x, y), r, r);
    }

    // a see-through rainbow arching over the bottom of the sky
    const QPointF arcCenter(w * 0.42, h * 1.18);
    const double arcRadius = std::max(w, h) * 0.78;
    const double band = arcRadius * 0.028;
    const QColor bands[] = { QColor(168, 128, 255), QColor(92, 182, 255), QColor(108, 224, 140),
                             QColor(255, 230, 102), QColor(255, 168, 76), QColor(255, 92, 120) };
    QRadialGradient rainbow(arcCenter, arcRadius);
    const double inner = 1.0 - 6 * band / arcRadius;
    rainbow.setColorAt(0, Qt::transparent);
    rainbow.setColorAt(std::max(0.0, inner - 0.02), Qt::transparent);
    for (int i = 0; i < 6; i++) {
        QColor c = bands[i];
        c.setAlphaF(0.17);
        rainbow.setColorAt(inner + (i + 0.5) * band / arcRadius, c);
    }
    rainbow.setColorAt(1.0, Qt::transparent);
    p.setBrush(rainbow);
    p.drawEllipse(arcCenter, arcRadius, arcRadius);

    // the big moon with its soft glow, gray seas and craters
    const double radius = std::min(w, h) * 0.27;
    const QPointF moon(w * 0.74, h * 0.36);
    QRadialGradient halo(moon, radius * 2.4);
    halo.setColorAt(0, QColor(255, 255, 255, 60));
    halo.setColorAt(0.42, QColor(220, 228, 255, 26));
    halo.setColorAt(1, Qt::transparent);
    p.setBrush(halo);
    p.drawEllipse(moon, radius * 2.4, radius * 2.4);

    QRadialGradient body(moon - QPointF(radius * 0.35, radius * 0.35), radius * 1.45);
    body.setColorAt(0, QColor(0xfb, 0xfb, 0xff));
    body.setColorAt(0.55, QColor(0xe2, 0xe4, 0xee));
    body.setColorAt(0.85, QColor(0xc4, 0xc7, 0xd6));
    body.setColorAt(1, QColor(0x9a, 0x9e, 0xb3));
    p.setBrush(body);
    p.drawEllipse(moon, radius, radius);

    QPainterPath disk;
    disk.addEllipse(moon, radius, radius);
    p.setClipPath(disk);
    // the seas: big soft gray patches
    for (int i = 0; i < 7; i++) {
        const QPointF c = moon + QPointF((int(sceneHash(i, 5, 91) % 140) - 70) / 100.0 * radius, (int(sceneHash(i, 6, 91) % 140) - 70) / 100.0 * radius);
        const double r = radius * (0.2 + (sceneHash(i, 7, 91) % 25) / 100.0);
        QRadialGradient sea(c, r);
        sea.setColorAt(0, QColor(0x8a, 0x8f, 0xa6, 150));
        sea.setColorAt(0.6, QColor(0x8a, 0x8f, 0xa6, 95));
        sea.setColorAt(1, QColor(0x8a, 0x8f, 0xa6, 0));
        p.setBrush(sea);
        p.drawEllipse(c, r, r);
    }
    // craters: a darker bowl with a light rim on the side the light comes from
    for (int i = 0; i < 18; i++) {
        const QPointF c = moon + QPointF((int(sceneHash(i, 8, 92) % 170) - 85) / 100.0 * radius, (int(sceneHash(i, 9, 92) % 170) - 85) / 100.0 * radius);
        const double r = radius * (0.03 + (sceneHash(i, 10, 92) % 10) / 100.0);
        p.setBrush(QColor(0x7e, 0x82, 0x98, 150));
        p.drawEllipse(c, r, r);
        p.setBrush(QColor(255, 255, 255, 120));
        p.drawEllipse(c - QPointF(r * 0.25, r * 0.25), r * 0.75, r * 0.75);
        p.setBrush(QColor(0xa4, 0xa8, 0xbb, 140));
        p.drawEllipse(c + QPointF(r * 0.12, r * 0.12), r * 0.7, r * 0.7);
    }
    p.setClipping(false);
    p.end();
    return pixmap;
}
