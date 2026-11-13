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

#include <QColor>
#include <QElapsedTimer>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QRect>
#include <QTimer>

class QPainter;
class QWidget;

/// The living sculk background: the veins of the texture glow with the heartbeat of the Warden and
/// sculk charge particles float up through the picture.
///
/// The owner calls paint() from its paintEvent; the ambience repaints the target widget about 30 times
/// a second while a sculk theme is active, the animation is switched on and the widget is visible.
class SculkAmbience : public QObject {
    Q_OBJECT
   public:
    SculkAmbience(QWidget* target, int particles);

    /// true when a sculk theme is active and the "SculkBackgroundAnimation" setting is on
    static bool enabled();

    /// draws the pulse of the veins (aligned with the texture through textureOffset) and the particles;
    /// in the Nyxora theme (id "lunar"): red rain with splashes and faint red flashes
    void paint(QPainter* painter, const QRect& rect, const QPoint& textureOffset, const QColor& glow, int pixelSize = 4);

    /// the still part of the Nyxora background: a dark burgundy night with a red glow from below
    static QPixmap moonScene(const QSize& size, qreal devicePixelRatio);

   private:
    struct Particle {
        double x;      // 0..1 of the width
        double y;      // 0..1 of the height, the particle rises towards 0
        double speed;  // heights per second
        double sway;   // phase of the side to side motion
        int size;      // in screen pixels
    };
    void tick();
    void tickLunar(double t);
    void paintLunar(QPainter* painter, double t);
    Particle spawn(bool anywhere);
    static double pulse(double t);
    QRectF particleRect(const Particle& p, double t) const;

    QPointer<QWidget> m_target;
    QTimer m_timer;
    QElapsedTimer m_clock;
    QList<Particle> m_particles;
    qint64 m_lastTick = 0;
    QRect m_lastRect;  // where paint() drew last time, to repaint only around the particles
    double m_lastPulse = -1;
    quint32 m_seed = 1;
};
