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

// The detailed head of the AI agent, also drawn by program_info/warden_icon.py (agent.svg).
// . transparent  K outline      D dark skin   M skin         L light skin  H highlight  N brow shadow
// S eye socket   X socket rim   J jaw         Q mouth        O soul glow in the throat
// T tooth        U tooth shade  C soul spot   B bright soul  V sculk vein
// E horn         F horn rim     G horn glow   W bright horn glow
const char* s_head[] = {
    "..FWF....................FWF..",  //
    "..FEWEF................FEWEF..",  //
    "..FEGEF...G........G...FEGEF..",  //
    "...FEGEF.FE........EF.FEGEF...",  //
    "..FEEGEEFE.F......F.EFEEGEEF..",  //
    "...FEEGEEF..........FEEGEEF...",  //
    "....FEEEEEF........FEEEEEF....",  //
    ".....FEEEEEKKKKKKKKEEEEEF.....",  //
    ".....KDDMMMMMMMMMMMMMMDDK.....",  //
    ".....KDDLHHHHHHHHHHVHLDDK.....",  //
    ".....KDDLLVLLLLLLLVLLLDDK.....",  //
    ".....KDDMMMVVMMMMVMMMMDDK.....",  //
    ".....KDDMLLLLLLLLLLLLMDDK.....",  //
    ".....KDDMLLHHHHHHHHLLMDDK.....",  //
    ".....KDCCLLHHHHHHHHLLCCDK.....",  //
    ".....KDCBLLHHHHHHHHLLBCDK.....",  //
    ".....KDCCLLLLLLLLLLLLCCDK.....",  //
    ".....KDDMNNNNNNNNNNNNMDDK.....",  //
    ".....KDDMJJJJJJJJJJJJMDDK.....",  //
    ".....KDDMJTUTUTUTUTUJMDDK.....",  //
    ".....KDDMJTQQTOOTQQTJMDDK.....",  //
    ".....KDDMJQQQQOOQQQQJMDDK.....",  //
    ".....KDDMJUTUTUTUTUTJMDDK.....",  //
    ".....KDDDJJJJJJJJJJJJDDDK.....",  //
    ".....KKDDDDDDDDDDDDDDDDKK.....",  //
    "......KKKKKKKKKKKKKKKKKK......",  //
};
constexpr double s_pi = 3.14159265358979323846;
constexpr int s_cols = 30;
constexpr int s_rows = 26;
// rows 0..7 hold the horns, which sway around their base
constexpr int s_hornRows = 8;
const QPointF s_leftHornBase(8.5, 8.0);
const QPointF s_rightHornBase(21.5, 8.0);

QColor colorOf(char c)
{
    switch (c) {
        case 'K':
            return QColor(0x03, 0x12, 0x1a);
        case 'D':
            return QColor(0x0c, 0x36, 0x40);
        case 'M':
            return QColor(0x14, 0x51, 0x60);
        case 'L':
            return QColor(0x1d, 0x6a, 0x78);
        case 'H':
            return QColor(0x2a, 0x85, 0x93);
        case 'N':
            return QColor(0x0a, 0x2d, 0x35);
        case 'S':
            return QColor(0x01, 0x08, 0x0a);
        case 'X':
            return QColor(0x0f, 0x3f, 0x4a);
        case 'J':
            return QColor(0x06, 0x1e, 0x24);
        case 'Q':
            return QColor(0x03, 0x14, 0x1a);
        case 'O':
            return QColor(0x0f, 0x6f, 0x78);
        case 'T':
            return QColor(0xd9, 0xec, 0xe6);
        case 'U':
            return QColor(0x9f, 0xb8, 0xb1);
        case 'V':
            return QColor(0x1f, 0xb3, 0xbd);
        case 'E':
            return QColor(0x1a, 0x8c, 0x96);
        case 'F':
            return QColor(0x0d, 0x55, 0x5c);
        case 'B':
        case 'W':
            return QColor(0xb8, 0xfb, 0xff);
        default:
            return QColor(0x29, 0xdf, 0xeb);
    }
}

bool isHornCell(int x, int y)
{
    const char c = s_head[y][x];
    return y < s_hornRows && (c == 'E' || c == 'F' || c == 'G' || c == 'W');
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
        m_particles << Particle{ QPointF(6.0 + random->generateDouble() * 18.0, s_rows - 1.0 + random->generateDouble()),
                                 1.5 + random->generateDouble() * 2.0, now, 2.5 + random->generateDouble() * 2.0 };
    }
    update();
}

void WardenWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // fit the 30x26 head (plus room for the swaying horns and the speech bubble) into the widget
    // with the bubble the head keeps to the left three quarters and sits at the bottom, the bubble takes the top right corner
    const double headWidth = m_bubble ? width() * 0.74 : width();
    const double cell = std::floor(std::min(headWidth / (s_cols + 2.0), height() / (s_rows + 2.0)) * 4.0) / 4.0;
    const QPointF origin((headWidth - s_cols * cell) / 2.0,
                         m_bubble ? height() - (s_rows + 0.75) * cell : (height() - s_rows * cell) / 2.0 + cell * 0.5);
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
            if (c == 'C' || c == 'O') {
                // the souls in the cheeks and the throat light up with every heartbeat
                drawCell(x, y, mix(c == 'O' ? colorOf('O') : glowDim, glowBright, 0.25 + 0.75 * beat));
            } else if (c == 'V') {
                drawCell(x, y, mix(colorOf('M'), glowDim, 0.45 + 0.4 * beat));
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
                drawCell(x, y, c == 'G' ? mix(glowDim, glowBright, glow) : c == 'W' ? mix(glowBright, QColor(Qt::white), glow) : colorOf(c));
            }
        }
        for (int y = 0; y < s_hornRows; y++) {
            for (int x = side == 0 ? 0 : s_cols / 2; x < (side == 0 ? s_cols / 2 : s_cols); x++) {
                if (s_head[y][x] == 'W') {
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
            if (s_head[y][x] == 'B' || s_head[y][x] == 'O') {
                glowCell(x, y, (s_head[y][x] == 'O' ? 0.1 : 0.2) + 0.6 * beat);
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
    const double w = std::min(width() * 0.36, height() * 0.75);
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
