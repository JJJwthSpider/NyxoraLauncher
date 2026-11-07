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

#include "MoonyWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <algorithm>
#include <cmath>

namespace {
const QColor s_night(0x1c, 0x1f, 0x33);
const QColor s_nightEdge(0x0e, 0x10, 0x1c);
const QColor s_moon(0xf6, 0xf7, 0xfc);
const QColor s_moonEdge(0xc9, 0xcc, 0xdb);
const QColor s_glow(220, 228, 255);

// the sparkles around Moony: position (in moon radii from its center), size, phase
struct Sparkle {
    double x, y, size, phase;
};
const Sparkle s_sparkles[] = {
    { -1.32, -0.78, 0.13, 0.0 }, { 1.30, -0.95, 0.10, 1.7 }, { 1.42, 0.55, 0.12, 3.1 }, { -1.38, 0.62, 0.09, 4.4 }, { 0.15, -1.38, 0.08, 2.3 },
};

// one of the see-through rainbow colors, for the blush on the cheeks
const QColor s_blush(255, 150, 175);

void drawSparkle(QPainter& p, const QPointF& c, double size, double brightness)
{
    QColor color = s_glow;
    color.setAlphaF(std::clamp(brightness, 0.0, 1.0));
    QPainterPath star;
    // four pointed star: thin rays with a round middle
    star.moveTo(c.x(), c.y() - size);
    star.quadTo(c.x(), c.y(), c.x() + size, c.y());
    star.quadTo(c.x(), c.y(), c.x(), c.y() + size);
    star.quadTo(c.x(), c.y(), c.x() - size, c.y());
    star.quadTo(c.x(), c.y(), c.x(), c.y() - size);
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawPath(star);
}
}  // namespace

MoonyWidget::MoonyWidget(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setMinimumSize(48, 48);
    m_timer.setInterval(40);  // 25 frames per second is plenty for a calm animation
    connect(&m_timer, &QTimer::timeout, this, &MoonyWidget::tick);
    m_clock.start();
}

void MoonyWidget::setExcited(bool excited)
{
    m_excited = excited;
}

void MoonyWidget::setBubbleVisible(bool visible)
{
    m_bubble = visible;
    update();
}

void MoonyWidget::setTalking(bool talking)
{
    m_talking = talking;
}

void MoonyWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    m_timer.start();
}

void MoonyWidget::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    m_timer.stop();  // no animation work while the launcher is hidden or minimized
}

void MoonyWidget::tick()
{
    const double now = m_clock.elapsed() / 1000.0;
    const double dt = std::min(now - m_lastFrame, 0.1);
    m_lastFrame = now;
    m_time = now;
    // the eyes open and close softly
    const double wakeTarget = (m_excited || m_talking) ? 1.0 : 0.0;
    m_wake += (wakeTarget - m_wake) * std::min(1.0, dt * 6.0);
    m_talk = m_talking ? 0.5 + 0.5 * std::sin(now * 11.0) : std::max(0.0, m_talk - dt * 4.0);
    update();
}

void MoonyWidget::paintMoon(QPainter& p, const QRectF& square, double time, double wake, double talk)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    const double r = square.width() * 0.34;
    const QPointF c = square.center() + QPointF(0, std::sin(time * 1.2) * r * 0.05);

    // the glow breathes slowly and gets brighter when Moony is awake
    const double breath = 0.5 + 0.5 * std::sin(time * 1.6);
    QRadialGradient halo(c, r * 1.55);
    QColor inner = s_glow;
    inner.setAlphaF(0.20 + 0.15 * breath + 0.2 * wake);
    QColor outer = s_glow;
    outer.setAlphaF(0.0);
    halo.setColorAt(0.55, inner);
    halo.setColorAt(1.0, outer);
    p.setPen(Qt::NoPen);
    p.setBrush(halo);
    p.drawEllipse(c, r * 1.55, r * 1.55);

    // sparkles twinkle around the moon
    for (const auto& s : s_sparkles) {
        const double twinkle = 0.5 + 0.5 * std::sin(time * 2.3 + s.phase);
        drawSparkle(p, c + QPointF(s.x * r, s.y * r), s.size * r * (0.7 + 0.5 * twinkle), 0.35 + 0.65 * twinkle);
    }

    // one half night, the other half moon white
    QPainterPath disk;
    disk.addEllipse(c, r, r);
    p.save();
    p.setClipPath(disk);
    QRadialGradient dark(c - QPointF(r * 0.3, r * 0.3), r * 1.4);
    dark.setColorAt(0, s_night.lighter(130));
    dark.setColorAt(1, s_nightEdge);
    p.setBrush(dark);
    p.drawRect(QRectF(c.x() - r, c.y() - r, r, 2 * r));
    QRadialGradient light(c + QPointF(r * 0.2, -r * 0.3), r * 1.3);
    light.setColorAt(0, s_moon);
    light.setColorAt(1, s_moonEdge);
    p.setBrush(light);
    p.drawRect(QRectF(c.x(), c.y() - r, r, 2 * r));
    // craters: light ones on the dark half, gray ones on the white half
    struct Crater {
        double x, y, size;
    };
    const Crater craters[] = { { -0.55, -0.45, 0.13 }, { -0.62, 0.40, 0.09 }, { 0.55, 0.50, 0.12 }, { 0.62, -0.50, 0.08 } };
    for (const auto& cr : craters) {
        p.setBrush(cr.x < 0 ? QColor(255, 255, 255, 26) : QColor(0x9a, 0x9e, 0xb4, 70));
        p.drawEllipse(c + QPointF(cr.x * r, cr.y * r), cr.size * r, cr.size * r);
    }
    // a tiny star on the night half
    drawSparkle(p, c + QPointF(-0.35 * r, -0.62 * r), 0.09 * r, 0.6 + 0.4 * breath);
    p.restore();

    // the line between the halves and the rim
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(255, 255, 255, 120), std::max(1.0, r * 0.04)));
    p.drawEllipse(c, r, r);

    // the face: on each half its features are in the color of the other half
    const double eyeY = c.y() - r * 0.05;
    const double eyeX = r * 0.38;
    const double eyeW = r * 0.2;
    const QColor onDark = s_moon;
    const QColor onLight = s_nightEdge;
    const double pen = std::max(1.2, r * 0.07);
    for (int side = -1; side <= 1; side += 2) {
        const QPointF e(c.x() + side * eyeX, eyeY);
        const QColor color = side < 0 ? onDark : onLight;
        if (wake < 0.5) {
            // sleeping: closed eyes are soft arcs
            p.setPen(QPen(color, pen, Qt::SolidLine, Qt::RoundCap));
            p.setBrush(Qt::NoBrush);
            QPainterPath arc;
            arc.moveTo(e.x() - eyeW, e.y());
            arc.quadTo(e.x(), e.y() + eyeW * (0.9 - wake), e.x() + eyeW, e.y());
            p.drawPath(arc);
        } else {
            // awake: round eyes with a twinkle in them
            const double open = (wake - 0.5) * 2.0;
            p.setPen(Qt::NoPen);
            p.setBrush(color);
            p.drawEllipse(e, eyeW * 0.65, eyeW * (0.25 + 0.55 * open));
            p.setBrush(side < 0 ? onLight : onDark);
            p.drawEllipse(e + QPointF(eyeW * 0.2, -eyeW * 0.2 * open), eyeW * 0.18 * open, eyeW * 0.18 * open);
        }
        // a little blush in a rainbow pink
        QColor blush = s_blush;
        blush.setAlphaF(side < 0 ? 0.35 : 0.45);
        p.setPen(Qt::NoPen);
        p.setBrush(blush);
        p.drawEllipse(QPointF(c.x() + side * r * 0.55, eyeY + r * 0.28), r * 0.13, r * 0.07);
    }
    // the smile runs across both halves: each half in the other color
    const double mouthY = c.y() + r * 0.3;
    const double mouthW = r * 0.22;
    const double open = talk * r * 0.12;
    for (int side = -1; side <= 1; side += 2) {
        p.save();
        p.setClipRect(side < 0 ? QRectF(c.x() - r, c.y() - r, r, 2 * r) : QRectF(c.x(), c.y() - r, r, 2 * r));
        const QColor color = side < 0 ? onDark : onLight;
        if (open > 0.5) {
            p.setPen(Qt::NoPen);
            p.setBrush(color);
            p.drawEllipse(QPointF(c.x(), mouthY + open * 0.3), mouthW * 0.55, open);
        } else {
            p.setPen(QPen(color, pen, Qt::SolidLine, Qt::RoundCap));
            p.setBrush(Qt::NoBrush);
            QPainterPath smile;
            smile.moveTo(c.x() - mouthW, mouthY);
            smile.quadTo(c.x(), mouthY + mouthW * 0.8, c.x() + mouthW, mouthY);
            p.drawPath(smile);
        }
        p.restore();
    }
    p.restore();
}

void MoonyWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    // with the bubble the moon sits a little lower and to the left, the bubble above it on the right
    const double side = m_bubble ? std::min(width() * 0.78, height() * 0.86) : std::min(width(), height());
    const QRectF square(m_bubble ? 0 : (width() - side) / 2.0, height() - side, side, side);
    paintMoon(painter, square, m_time, m_wake, m_talk);
    if (m_bubble) {
        const double h = side * 0.3;
        const double w = h * 1.9;
        paintBubble(painter, QRectF(std::min(width() - w - 1.0, square.center().x() + side * 0.12), 1, w, h));
    }
}

void MoonyWidget::paintBubble(QPainter& painter, const QRectF& box)
{
    const double w = box.width();
    const double h = box.height();
    QPainterPath path;
    path.addRoundedRect(box, h * 0.35, h * 0.35);
    QPolygonF tail;
    tail << QPointF(box.left() + w * 0.18, box.bottom() - 1) << QPointF(box.left() + w * 0.02, box.bottom() + h * 0.4)
         << QPointF(box.left() + w * 0.38, box.bottom() - 1);
    path.addPolygon(tail);
    path = path.simplified();
    painter.setPen(QPen(QColor(255, 255, 255, 220), std::max(1.5, w * 0.045)));
    painter.setBrush(QColor(0x14, 0x15, 0x1f));
    painter.drawPath(path);

    // three dots in see-through rainbow colors
    const QColor dots[] = { QColor(255, 168, 196), QColor(255, 236, 160), QColor(160, 210, 255) };
    const double r = h * 0.12;
    for (int i = 0; i < 3; i++) {
        double lift = 0;
        if (m_talking) {
            // the dots jump one after another, like someone typing
            lift = std::max(0.0, std::sin(m_time * 7.0 - i * 0.9)) * h * 0.18;
        }
        painter.setPen(Qt::NoPen);
        painter.setBrush(dots[i]);
        painter.drawEllipse(QPointF(box.left() + w * (0.28 + i * 0.22), box.center().y() - lift), r, r);
    }
}
