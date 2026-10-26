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
#pragma once

#include <QElapsedTimer>
#include <QList>
#include <QPointF>
#include <QTimer>
#include <QWidget>

class QPainter;

/// The face of the mod pack agent: an animated pixel art Warden head.
/// Its sculk horns sway and glow, the halo beats like the Warden's heart and soul particles drift up.
/// When "excited" (mouse over the tile) the heart beats faster and the glow gets brighter.
class WardenWidget : public QWidget {
    Q_OBJECT
   public:
    explicit WardenWidget(QWidget* parent = nullptr);

    void setExcited(bool excited);
    /// a speech bubble next to the head: the agent is a chat
    void setBubbleVisible(bool visible);
    /// the dots in the bubble jump while the agent is thinking or answering
    void setTalking(bool talking);
    QSize sizeHint() const override { return { 96, 96 }; }

   protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

   private:
    struct Particle {
        QPointF pos;  // in head pixels
        double speed;
        double born;
        double life;
    };

    void tick();
    void paintBubble(QPainter& painter);
    double heartbeat(double t) const;

    QTimer m_timer;
    QElapsedTimer m_clock;
    double m_time = 0;
    double m_beatPhase = 0;
    double m_lastFrame = 0;
    bool m_excited = false;
    bool m_bubble = false;
    bool m_talking = false;
    QList<Particle> m_particles;
};
