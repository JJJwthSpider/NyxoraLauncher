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
#include "WardenWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QRandomGenerator>
#include <algorithm>
#include <cmath>

namespace {

// The same head as the launcher icon (program_info/warden_icon.py).
// . transparent  K outline  D dark skin  M skin  L light skin  E horn  G horn glow
// S eye socket   J mouth    T teeth      C glowing sculk spot
const char* s_head[] = {
    "EE................EE",  //
    "EGE..............EGE",  //
    "EGGE............EGGE",  //
    ".EGGE..........EGGE.",  //
    "..EGEE........EEGE..",  //
    "...EEKKKKKKKKKKEE...",  //
    "...KDDDMMMMMMDDDK...",  //
    "...KDMMLLLLLLMMDK...",  //
    "...KDSSSMLLMSSSDK...",  //
    "...KDSSSMMMMSSSDK...",  //
    "...KDCMMMMMMMMCDK...",  //
    "...KDMJJJJJJJJMDK...",  //
    "...KDJTTJTTJTTJDK...",  //
    "...KDJJJJJJJJJJDK...",  //
    "...KDDMMMMMMMMDDK...",  //
    "....KDDDDDDDDDDK....",  //
    ".....KKKKKKKKKK.....",  //
};
constexpr double s_pi = 3.14159265358979323846;
constexpr int s_cols = 20;
constexpr int s_rows = 17;
// rows 0..4 (and the horn cells of row 5) belong to the horns, which sway around their base
constexpr int s_hornRows = 6;
const QPointF s_leftHornBase(4.5, 5.0);
const QPointF s_rightHornBase(15.5, 5.0);

QColor colorOf(char c)
{
    switch (c) {
        case 'K':
            return QColor(0x04, 0x15, 0x1a);
        case 'D':
            return QColor(0x0e, 0x3a, 0x44);
        case 'M':
            return QColor(0x15, 0x54, 0x62);
        case 'L':
            return QColor(0x1f, 0x6f, 0x7c);
        case 'E':
            return QColor(0x1a, 0x8c, 0x96);
        case 'S':
            return QColor(0x02, 0x0a, 0x0c);
        case 'J':
            return QColor(0x08, 0x2a, 0x31);
        case 'T':
            return QColor(0xcf, 0xe3, 0xdc);
        default:
            return QColor(0x29, 0xdf, 0xeb);
    }
}

bool isHornCell(int x, int y)
{
    const char c = s_head[y][x];
    return y < s_hornRows && (c == 'E' || c == 'G');
}

QColor mix(const QColor& a, const QColor& b, double t)
{
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t, a.greenF() + (b.greenF() - a.greenF()) * t,
                            a.blueF() + (b.blueF() - a.blueF()) * t);
}

}  // namespace

WardenWidget::WardenWidget(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setMinimumSize(64, 64);
    m_timer.setInterval(40);  // 25 frames per second is plenty for a calm animation
    connect(&m_timer, &QTimer::timeout, this, &WardenWidget::tick);
    m_clock.start();
}

void WardenWidget::setExcited(bool excited)
{
    m_excited = excited;
}

void WardenWidget::setBubbleVisible(bool visible)
{
    m_bubble = visible;
    update();
}

void WardenWidget::setTalking(bool talking)
{
    m_talking = talking;
}

void WardenWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    m_timer.start();
}

void WardenWidget::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    m_timer.stop();  // no animation work while the launcher is hidden or minimized
}

double WardenWidget::heartbeat(double phase) const
{
    // "lub-dub": two quick beats, then a rest, like the Warden's heart sound
    const double p = std::fmod(phase, 1.0);
    auto pulse = [](double x, double center, double width) { return std::exp(-std::pow((x - center) / width, 2)); };
    return pulse(p, 0.10, 0.045) + 0.7 * pulse(p, 0.28, 0.045);
}

void WardenWidget::tick()
{
    const double now = m_clock.elapsed() / 1000.0;
    const double dt = std::min(now - m_lastFrame, 0.1);
    m_lastFrame = now;
    m_time = now;
    // the heart races while the Warden "senses" the mouse
    m_beatPhase += dt * (m_excited ? 1.6 : 0.75);

    auto* random = QRandomGenerator::global();
    m_particles.erase(std::remove_if(m_particles.begin(), m_particles.end(), [now](const Particle& p) { return now - p.born > p.life; }),
                      m_particles.end());
    for (auto& p : m_particles) {
        p.pos.ry() -= p.speed * dt;
    }
    const double spawnChance = m_excited ? 0.35 : 0.12;
    if (random->generateDouble() < spawnChance && m_particles.size() < 24) {
        m_particles << Particle{ QPointF(2.0 + random->generateDouble() * 16.0, s_rows - 1.0 + random->generateDouble()),
                                 1.5 + random->generateDouble() * 2.0, now, 2.5 + random->generateDouble() * 2.0 };
    }
    update();
}

void WardenWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // fit the 20x17 head (plus room for the swaying horns and the speech bubble) into the widget
    const double room = m_bubble ? 0.78 : 1.0;
    const double cell = std::floor(std::min(width() * room / (s_cols + 2.0), height() * room / (s_rows + 3.0)) * 4.0) / 4.0;
    const QPointF origin(m_bubble ? cell : (width() - s_cols * cell) / 2.0,
                         m_bubble ? height() - (s_rows + 1.0) * cell : (height() - s_rows * cell) / 2.0 + cell * 0.5);
    const double bob = std::sin(m_time * 1.3) * cell * 0.25;
    painter.translate(origin.x(), origin.y() + bob);

    const double beat = heartbeat(m_beatPhase);
    const double glow = 0.5 + 0.5 * std::sin(m_time * 2.4);
    const QColor glowDim(0x29, 0xdf, 0xeb);
    const QColor glowBright(0xb8, 0xfb, 0xff);

    // halo behind the head, beating with the heart
    {
        const QPointF center(s_cols * cell / 2.0, s_rows * cell * 0.55);
        const double radius = s_cols * cell * (0.55 + 0.06 * beat);
        QRadialGradient gradient(center, radius);
        QColor inner(0x29, 0xdf, 0xeb);
        inner.setAlphaF(std::min(1.0, (m_excited ? 0.38 : 0.24) + 0.25 * beat));
        QColor outer = inner;
        outer.setAlphaF(0.0);
        gradient.setColorAt(0.0, inner);
        gradient.setColorAt(1.0, outer);
        painter.setPen(Qt::NoPen);
        painter.setBrush(gradient);
        painter.drawEllipse(center, radius, radius);
    }

    auto drawCell = [&painter, cell](int x, int y, const QColor& color) {
        // a hair larger than the cell, so no seams show between pixels when antialiased
        painter.fillRect(QRectF(x * cell, y * cell, cell + 0.6, cell + 0.6), color);
    };
    auto glowCell = [&painter, cell](double x, double y, double strength) {
        const QPointF center((x + 0.5) * cell, (y + 0.5) * cell);
        QRadialGradient gradient(center, cell * 2.2);
        QColor c(0x8f, 0xf6, 0xfb);
        c.setAlphaF(std::clamp(strength, 0.0, 1.0));
        QColor clear = c;
        clear.setAlphaF(0);
        gradient.setColorAt(0, c);
        gradient.setColorAt(1, clear);
        painter.setPen(Qt::NoPen);
        painter.setBrush(gradient);
        painter.drawEllipse(center, cell * 2.2, cell * 2.2);
    };

    // the head
    painter.setRenderHint(QPainter::Antialiasing, false);
    for (int y = 0; y < s_rows; y++) {
        for (int x = 0; x < s_cols; x++) {
            const char c = s_head[y][x];
            if (c == '.' || isHornCell(x, y)) {
                continue;
            }
            if (c == 'C') {
                drawCell(x, y, mix(glowDim, glowBright, 0.3 + 0.7 * beat));
            } else {
                drawCell(x, y, colorOf(c));
            }
        }
    }

    // the horns sway around their base, in opposite directions
    const double sway = std::sin(m_time * (m_excited ? 3.2 : 1.8)) * (m_excited ? 9.0 : 5.0);
    for (int side = 0; side < 2; side++) {
        const QPointF base = side == 0 ? s_leftHornBase : s_rightHornBase;
        painter.save();
        painter.translate(base.x() * cell, base.y() * cell);
        painter.rotate(side == 0 ? -sway : sway);
        painter.translate(-base.x() * cell, -base.y() * cell);
        painter.setRenderHint(QPainter::Antialiasing, true);
        for (int y = 0; y < s_hornRows; y++) {
            for (int x = side == 0 ? 0 : s_cols / 2; x < (side == 0 ? s_cols / 2 : s_cols); x++) {
                if (!isHornCell(x, y)) {
                    continue;
                }
                const char c = s_head[y][x];
                drawCell(x, y, c == 'G' ? mix(glowDim, glowBright, glow) : colorOf(c));
            }
        }
        for (int y = 0; y < s_hornRows; y++) {
            for (int x = side == 0 ? 0 : s_cols / 2; x < (side == 0 ? s_cols / 2 : s_cols); x++) {
                if (s_head[y][x] == 'G') {
                    glowCell(x, y, 0.25 + 0.35 * glow + (m_excited ? 0.2 : 0.0));
                }
            }
        }
        painter.restore();
    }

    // glow of the sculk spots on the cheeks
    painter.setRenderHint(QPainter::Antialiasing, true);
    for (int y = 0; y < s_rows; y++) {
        for (int x = 0; x < s_cols; x++) {
            if (s_head[y][x] == 'C') {
                glowCell(x, y, 0.2 + 0.6 * beat);
            }
        }
    }

    // soul particles drifting up
    painter.setPen(Qt::NoPen);
    for (const auto& p : m_particles) {
        const double age = (m_time - p.born) / p.life;
        QColor c(0x8f, 0xf6, 0xfb);
        c.setAlphaF(std::clamp(std::sin(age * s_pi), 0.0, 1.0) * 0.8);
        const double size = cell * (0.55 - 0.25 * age);
        painter.fillRect(QRectF(p.pos.x() * cell, p.pos.y() * cell - bob, size, size), c);
    }

    if (m_bubble) {
        paintBubble(painter);
    }
}

void WardenWidget::paintBubble(QPainter& painter)
{
    painter.resetTransform();
    painter.setRenderHint(QPainter::Antialiasing, true);
    const double w = width() * 0.42;
    const double h = w * 0.5;
    const QRectF box(width() - w - 2, 2, w, h);
    QPainterPath path;
    path.addRoundedRect(box, h * 0.3, h * 0.3);
    QPolygonF tail;
    tail << QPointF(box.left() + w * 0.3, box.bottom() - 1) << QPointF(box.left() + w * 0.12, box.bottom() + h * 0.4)
         << QPointF(box.left() + w * 0.5, box.bottom() - 1);
    path.addPolygon(tail);
    path = path.simplified();
    painter.setPen(QPen(QColor(0x29, 0xdf, 0xeb), std::max(1.5, w * 0.04)));
    painter.setBrush(QColor(0x0b, 0x1f, 0x25));
    painter.drawPath(path);

    const QColor dots[] = { QColor(0x8f, 0xf6, 0xfb), QColor(0x29, 0xdf, 0xeb), QColor(0x1a, 0x8c, 0x96) };
    const double r = h * 0.12;
    for (int i = 0; i < 3; i++) {
        double lift = 0;
        QColor color = dots[i];
        if (m_talking) {
            // the dots jump one after another, like someone typing
            lift = std::max(0.0, std::sin(m_time * 7.0 - i * 0.9)) * h * 0.18;
            color = QColor(0x8f, 0xf6, 0xfb);
        }
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawEllipse(QPointF(box.left() + w * (0.28 + i * 0.22), box.center().y() - lift), r, r);
    }
}
