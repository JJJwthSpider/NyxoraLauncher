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
#include <iterator>
#include <cmath>

namespace {

// The Warden head of the AI agent, pixel for pixel like the Warden in Minecraft: a 16x16 head with the antlers
// spreading to the sides (program_info/warden_icon.py draws the same grid for agent.svg).
// A light antler tip   B, C antler   D..H dark skin   I, J, M darkest skin   K teal in the mouth   L the mouth
const char* s_head[] = {
    ".A.B............................B.A.",  //
    ".BBB............................BBB.",  //
    "BBBC......DDDDDDDDDDDDDDDD......CBBB",  //
    "BAC.......DEDFFDGFDFFFFFFD.......CAB",  //
    "BBCE......EDFFFFFFFFFFFFFF......ECBB",  //
    "ABBCBD.E..FFFHGHFHFFHHHHHH..E.DBCBBA",  //
    "A.BBDDDEG.IHGIGIHIIIHIHIII.GEDDDBB.A",  //
    "...A..FFGGHGIIIHIHHIHIIIIIGGFF..A...",  //
    "...A..AFEGIGIIHFFFFFFHIIIIGEFA..A...",  //
    "........EGJIIHFFFKKFFFIIIIGE........",  //
    "..........JIIFGLLLLLLGFIII..........",  //
    "..........JIFLLLLLLLLLLFII..........",  //
    "..........IIHLLMMMMMMLLHII..........",  //
    "..........IHLMMMHHHHMMMLHI..........",  //
    "..........IGFMMKKKEKFMMFGI..........",  //
    "..........IIHFFEIIIIEFFHII..........",  //
    "..........IIHHHIIIIIIHHHII..........",  //
    "..........IIIHIIIIIIIIHIIH..........",  //
};
constexpr double s_pi = 3.14159265358979323846;
constexpr int s_cols = 36;
constexpr int s_rows = 18;
// the head itself is columns 10..25, everything left and right of it belongs to the antlers
constexpr int s_headLeft = 10;
constexpr int s_headRight = 25;
const QPointF s_leftHornBase(10.0, 8.5);
const QPointF s_rightHornBase(26.0, 8.5);

QColor colorOf(char c)
{
    switch (c) {
        case 'A':
            return QColor(0xa2, 0xb1, 0xb8);
        case 'B':
            return QColor(0x3d, 0x5c, 0x6a);
        case 'C':
            return QColor(0x34, 0x48, 0x52);
        case 'D':
            return QColor(0x26, 0x31, 0x37);
        case 'E':
            return QColor(0x17, 0x21, 0x23);
        case 'F':
            return QColor(0x1a, 0x23, 0x28);
        case 'G':
            return QColor(0x16, 0x15, 0x1a);
        case 'H':
            return QColor(0x1b, 0x1a, 0x20);
        case 'I':
            return QColor(0x09, 0x09, 0x11);
        case 'J':
            return QColor(0x0e, 0x0d, 0x13);
        case 'K':
            return QColor(0x2e, 0x3f, 0x46);
        case 'L':
            return QColor(0x00, 0x00, 0x00);
        case 'M':
            return QColor(0x06, 0x05, 0x0b);
        default:
            return QColor(0, 0, 0);
    }
}

const QPointF s_mouthCenter(17.5, 11.5);
// glowing dots along the left antler (the right one is mirrored), each twinkling in its own rhythm
const QPoint s_antlerDots[] = { { 2, 1 }, { 1, 4 }, { 4, 5 }, { 3, 6 }, { 7, 5 }, { 8, 7 } };
// sparkles around the head, in cell coordinates, kept clear of the head and the speech bubble
const QPointF s_sparkles[] = { { 3.5, 10.0 }, { 32.5, 10.0 }, { 6.5, 14.0 }, { 29.5, 14.0 }, { 8.5, 11.5 },
                               { 27.5, 11.5 }, { 5.0, 1.5 },  { 31.0, 1.5 }, { 2.0, 16.0 }, { 34.0, 16.0 } };

bool isHornCell(int x, int)
{
    return x < s_headLeft || x > s_headRight;
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
        m_particles << Particle{ QPointF(s_headLeft + random->generateDouble() * (s_headRight - s_headLeft), s_rows - 1.0 + random->generateDouble()),
                                 1.5 + random->generateDouble() * 2.0, now, 2.5 + random->generateDouble() * 2.0 };
    }
    update();
}

void WardenWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // the head with its antlers fills the width; the speech bubble, if shown, sits above the head between the antlers
    constexpr double bubbleRows = 6.0;
    const double rowsNeeded = s_rows + 1.0 + (m_bubble ? bubbleRows : 0.0);
    const double cell = std::floor(std::min(width() / (s_cols + 1.0), height() / rowsNeeded) * 4.0) / 4.0;
    const QPointF origin((width() - s_cols * cell) / 2.0, height() - (s_rows + 0.5) * cell);
    const double bob = std::sin(m_time * 1.3) * cell * 0.25;
    painter.translate(origin.x(), origin.y() + bob);

    const double beat = heartbeat(m_beatPhase);
    const double glow = 0.5 + 0.5 * std::sin(m_time * 2.4);

    // halo behind the head, beating with the heart
    {
        const QPointF center((s_headLeft + s_headRight + 1) * cell / 2.0, s_rows * cell * 0.55);
        const double radius = 12.0 * cell * (1.0 + 0.08 * beat);
        QRadialGradient gradient(center, radius);
        QColor inner(0x29, 0xdf, 0xeb);
        inner.setAlphaF(std::min(1.0, (m_excited ? 0.34 : 0.2) + 0.22 * beat));
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
        QRadialGradient gradient(center, cell * 2.0);
        QColor c(0x8f, 0xf6, 0xfb);
        c.setAlphaF(std::clamp(strength, 0.0, 1.0));
        QColor clear = c;
        clear.setAlphaF(0);
        gradient.setColorAt(0, c);
        gradient.setColorAt(1, clear);
        painter.setPen(Qt::NoPen);
        painter.setBrush(gradient);
        painter.drawEllipse(center, cell * 2.0, cell * 2.0);
    };
    // three souls circling the head; the ones behind it are drawn before the head, the others after it
    auto drawSouls = [&painter, cell, this](bool front) {
        for (int i = 0; i < 3; i++) {
            const double angle = m_time * 0.9 + i * 2.0 * s_pi / 3.0;
            const double depth = std::sin(angle);  // in front of the head when positive
            if ((depth >= 0) != front) {
                continue;
            }
            const QPointF center((s_headLeft + s_headRight + 1) / 2.0 * cell + std::cos(angle) * 11.0 * cell,
                                 (s_rows * 0.5 + depth * 2.5) * cell);
            const double size = cell * (0.9 + 0.35 * depth);
            QRadialGradient gradient(center, size * 2.6);
            QColor c(0x8f, 0xf6, 0xfb);
            c.setAlphaF(0.35 + 0.25 * depth);
            QColor clear = c;
            clear.setAlphaF(0);
            gradient.setColorAt(0, c);
            gradient.setColorAt(1, clear);
            painter.setPen(Qt::NoPen);
            painter.setBrush(gradient);
            painter.drawEllipse(center, size * 2.6, size * 2.6);
            painter.fillRect(QRectF(center.x() - size / 2, center.y() - size / 2, size, size), QColor(0xd8, 0xfd, 0xff));
        }
    };
    drawSouls(false);
    const QColor soul(0x3f, 0xc8, 0xd2);
    const QColor tipBright(0xe2, 0xf4, 0xff);

    // the head; the teal in the mouth lights up a little with every heartbeat
    painter.setRenderHint(QPainter::Antialiasing, false);
    for (int y = 0; y < s_rows; y++) {
        for (int x = 0; x < s_cols; x++) {
            const char c = s_head[y][x];
            if (c == '.' || isHornCell(x, y)) {
                continue;
            }
            drawCell(x, y, c == 'K' ? mix(colorOf('K'), soul, 0.15 + 0.55 * beat) : colorOf(c));
        }
    }

    // a soul glowing deep in the throat, brighter with every heartbeat
    painter.setRenderHint(QPainter::Antialiasing, true);
    {
        const QPointF center((s_mouthCenter.x() + 0.5) * cell, (s_mouthCenter.y() + 0.5) * cell);
        QRadialGradient gradient(center, cell * 4.0);
        QColor inner(0x5f, 0xf3, 0xfb);
        inner.setAlphaF(std::clamp(0.25 + 0.55 * beat + (m_excited ? 0.15 : 0.0), 0.0, 1.0));
        QColor outer = inner;
        outer.setAlphaF(0.0);
        gradient.setColorAt(0.0, inner);
        gradient.setColorAt(1.0, outer);
        painter.setPen(Qt::NoPen);
        painter.setBrush(gradient);
        painter.drawEllipse(center, cell * 4.0, cell * 2.6);
    }

    // the antlers sway around where they grow out of the head, in opposite directions; the light tips shimmer
    const double sway = std::sin(m_time * (m_excited ? 3.2 : 1.8)) * (m_excited ? 6.0 : 3.5);
    for (int side = 0; side < 2; side++) {
        const QPointF base = side == 0 ? s_leftHornBase : s_rightHornBase;
        painter.save();
        painter.translate(base.x() * cell, base.y() * cell);
        painter.rotate(side == 0 ? sway : -sway);
        painter.translate(-base.x() * cell, -base.y() * cell);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const int from = side == 0 ? 0 : s_headRight + 1;
        const int to = side == 0 ? s_headLeft : s_cols;
        for (int y = 0; y < s_rows; y++) {
            for (int x = from; x < to; x++) {
                const char c = s_head[y][x];
                if (c == '.') {
                    continue;
                }
                // pulses of energy run from the head out to the tips of the antlers
                const double along = std::abs(x + 0.5 - base.x());
                const double pulse = std::pow(std::max(0.0, std::sin(m_time * (m_excited ? 6.0 : 3.5) - along * 0.55)), 6.0);
                QColor color = colorOf(c);
                if (c == 'A') {
                    color = mix(mix(colorOf('A'), QColor(0x8f, 0xf6, 0xfb), 0.55), tipBright, glow * 0.6 + pulse * 0.4);
                } else if (c == 'B' || c == 'C') {
                    color = mix(color, QColor(0x29, 0xdf, 0xeb), 0.15 + 0.6 * pulse);
                }
                drawCell(x, y, color);
                if (c == 'A') {
                    glowCell(x, y, 0.25 + 0.3 * glow + 0.3 * pulse + (m_excited ? 0.15 : 0.0));
                } else if (pulse > 0.3 && (c == 'B' || c == 'C')) {
                    glowCell(x, y, 0.25 * pulse);
                }
            }
        }
        // the glowing dots on this antler move with it
        for (int i = 0; i < int(std::size(s_antlerDots)); i++) {
            const int x = side == 0 ? s_antlerDots[i].x() : s_cols - 1 - s_antlerDots[i].x();
            const int y = s_antlerDots[i].y();
            const double twinkle = 0.5 + 0.5 * std::sin(m_time * (2.0 + 0.37 * i) + i * 1.7 + side * 0.9);
            painter.setRenderHint(QPainter::Antialiasing, false);
            drawCell(x, y, mix(QColor(0x29, 0xdf, 0xeb), QColor(0xe6, 0xfe, 0xff), twinkle));
            painter.setRenderHint(QPainter::Antialiasing, true);
            glowCell(x, y, 0.3 + 0.45 * twinkle + (m_excited ? 0.15 : 0.0));
        }
        painter.restore();
    }
    painter.setRenderHint(QPainter::Antialiasing, true);

    // sparkles around the head: little four-pointed stars that light up and fade one after another
    for (int i = 0; i < int(std::size(s_sparkles)); i++) {
        const double phase = std::fmod(m_time * 0.45 + i * 0.37, 1.0);
        const double light = std::pow(std::max(0.0, std::sin(phase * s_pi)), 3.0);
        if (light < 0.02) {
            continue;
        }
        const QPointF center((s_sparkles[i].x() + 0.5) * cell, (s_sparkles[i].y() + 0.5) * cell);
        const double arm = cell * (0.6 + 0.9 * light);
        QColor color(0xc8, 0xfc, 0xff);
        color.setAlphaF(light);
        painter.setPen(QPen(color, std::max(1.0, cell * 0.35), Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(center - QPointF(arm, 0), center + QPointF(arm, 0));
        painter.drawLine(center - QPointF(0, arm), center + QPointF(0, arm));
        glowCell(s_sparkles[i].x(), s_sparkles[i].y(), 0.35 * light);
    }

    // the souls in front of the head
    drawSouls(true);

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
    // above the head, between the antlers, the tail pointing down at the Warden
    const double h = std::min(height() * 0.24, width() * 0.16);
    const double w = h * 2.0;
    const QRectF box(width() / 2.0 - w / 2.0 + w * 0.15, 1, w, h);
    QPainterPath path;
    path.addRoundedRect(box, h * 0.3, h * 0.3);
    QPolygonF tail;
    tail << QPointF(box.left() + w * 0.18, box.bottom() - 1) << QPointF(box.left() + w * 0.05, box.bottom() + h * 0.35)
         << QPointF(box.left() + w * 0.38, box.bottom() - 1);
    path.addPolygon(tail);
    path = path.simplified();
    painter.setPen(QPen(QColor(0x29, 0xdf, 0xeb), std::max(1.5, w * 0.05)));
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
