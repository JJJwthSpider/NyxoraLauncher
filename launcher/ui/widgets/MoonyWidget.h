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
#include <QTimer>
#include <QWidget>

class QPainter;

/// The face of the mod pack agent: Moony, a little moon whose one half is night dark and the other
/// half moon white. It floats gently, its glow breathes, stars twinkle around it; it opens its sleepy
/// eyes when the mouse comes near and talks while the agent is working.
class MoonyWidget : public QWidget {
    Q_OBJECT
   public:
    explicit MoonyWidget(QWidget* parent = nullptr);

    /// the mouse is over the tile: Moony wakes up and glows brighter
    void setExcited(bool excited);
    /// a speech bubble next to the moon: the agent is a chat
    void setBubbleVisible(bool visible);
    /// the dots in the bubble jump and the mouth moves while the agent is thinking or answering
    void setTalking(bool talking);
    QSize sizeHint() const override { return { 96, 96 }; }

    /// paints Moony into a square (also used for the static agent icon)
    static void paintMoon(QPainter& painter, const QRectF& square, double time, double wake, double talk);

   protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

   private:
    void tick();
    void paintBubble(QPainter& painter, const QRectF& box);

    QTimer m_timer;
    QElapsedTimer m_clock;
    double m_time = 0;
    double m_lastFrame = 0;
    double m_wake = 0;  // 0 asleep .. 1 eyes open, eases towards the target
    double m_talk = 0;
    bool m_excited = false;
    bool m_bubble = false;
    bool m_talking = false;
};
