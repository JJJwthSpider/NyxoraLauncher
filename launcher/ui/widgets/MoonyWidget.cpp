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
// Arkan, the wizard of Vellora Client: the picture is pixel art (resources/sculk_theme/wizard_body.png, drawn by
// android/art/wizard.py), the tesseract above his hand is drawn every frame. HAND and SIZE are in parts of the picture.
constexpr double s_handX = 53.0 / 64, s_handY = 16.0 / 64, s_tesseractSize = 30.0 / 64, s_glowSize = 10.0 / 64;
const QColor s_crimson(255, 70, 84);
const QColor s_corner(255, 200, 205);

const QPixmap& wizardBody()
{
    static const QPixmap body(QStringLiteral(":/sculk_theme/wizard_body.png"));
    return body;
}

/// the 16 corners of a tesseract turned in 4D by `turn`, projected to 2D around the origin
void tesseractCorners(double turn, double size, QPointF out[16])
{
    for (int i = 0; i < 16; i++) {
        const double x = (i & 1) ? 1 : -1, y = (i & 2) ? 1 : -1, z = (i & 4) ? 1 : -1, w = (i & 8) ? 1 : -1;
        const double a = turn, b = turn * 0.7;
        const double x1 = x * std::cos(a) - w * std::sin(a), w1 = x * std::sin(a) + w * std::cos(a);
        const double y1 = y * std::cos(b) - z * std::sin(b), z1 = y * std::sin(b) + z * std::cos(b);
        const double x2 = x1 * std::cos(0.5) - z1 * std::sin(0.5), z2 = x1 * std::sin(0.5) + z1 * std::cos(0.5);
        const double k4 = 1.0 / (3.0 - w1);
        const double k3 = 1.0 / (4.0 - z2 * k4);
        out[i] = QPointF(x2 * k4 * k3 * size, y1 * k4 * k3 * size);
    }
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
    // the wizard floats a little; the pixels stay sharp
    const double side = square.width();
    const QRectF body(square.left(), square.top() + std::sin(time * 1.2) * side * 0.012, side, side);
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    p.drawPixmap(body, wizardBody(), QRectF(wizardBody().rect()));

    // the red tesseract: it turns faster and glows brighter when Arkan is awake or working
    p.setRenderHint(QPainter::Antialiasing, true);
    const double energy = std::clamp(0.35 + 0.65 * std::max(wake, talk), 0.0, 1.0);
    const QPointF c(body.left() + side * s_handX, body.top() + side * s_handY + std::sin(time * 2.0) * side * 0.012);
    const double pulse = 0.75 + 0.25 * std::sin(time * (3.0 + 4.0 * energy));
    const double glowRadius = side * s_glowSize * (1.0 + 0.3 * energy);
    QRadialGradient glow(c, glowRadius);
    QColor inner(255, 40, 60);
    inner.setAlphaF(std::clamp((0.25 + 0.3 * energy) * pulse, 0.0, 1.0));
    QColor outer(255, 40, 60);
    outer.setAlphaF(0.0);
    glow.setColorAt(0, inner);
    glow.setColorAt(1, outer);
    p.setPen(Qt::NoPen);
    p.setBrush(glow);
    p.drawEllipse(c, glowRadius, glowRadius);

    QPointF corners[16];
    tesseractCorners(time * (0.6 + 1.2 * energy), side * s_tesseractSize, corners);
    QColor edge = s_crimson;
    edge.setAlphaF(std::clamp(0.75 + 0.25 * pulse, 0.0, 1.0));
    p.setPen(QPen(edge, std::max(1.0, side / 140.0), Qt::SolidLine, Qt::RoundCap));
    for (int i = 0; i < 16; i++) {
        for (int k = 0; k < 4; k++) {
            const int j = i ^ (1 << k);
            if (i < j) {
                p.drawLine(c + corners[i], c + corners[j]);
            }
        }
    }
    p.setPen(Qt::NoPen);
    p.setBrush(s_corner);
    const double dot = std::max(1.0, side / 110.0);
    for (const auto& corner : corners) {
        p.drawEllipse(c + corner, dot, dot);
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
    painter.setPen(QPen(QColor(255, 90, 110, 230), std::max(1.5, w * 0.045)));
    painter.setBrush(QColor(0x1c, 0x08, 0x0d));
    painter.drawPath(path);

    // three dots in crimson shades
    const QColor dots[] = { QColor(255, 90, 110), QColor(232, 54, 79), QColor(255, 140, 152) };
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
