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
#include <QImage>
#include <QTimer>
#include <QWidget>
#include <array>
#include <vector>

class QPainter;

/// The face of the mod pack agent: Arkan, the old hooded wizard, as a 3D mob in the style of a Minecraft mod, a model
/// of boxes with a pixel texture (resources/sculk_theme/arkan_model.json and arkan.png, made by android/art/arkan3d.py)
/// drawn by a small software renderer, the same as the phone's WizardView: the face lost in the shadow of the hood with
/// only the violet eyes glowing, the violet rim stitched with gold, the silver beard, the ruby brooch; he floats over a
/// turning magic circle, looks around and blinks, and the red tesseract turns in four dimensions over his raised palm
/// and lights the cloth near it. His eyes glow brighter when the mouse comes near; while the agent works the tesseract
/// spins faster and the sparks fly. In a small place only his head and shoulders are shown, looking at you. (The class
/// keeps its old name, Moony, so nothing else has to change.)
class MoonyWidget : public QWidget {
    Q_OBJECT
   public:
    explicit MoonyWidget(QWidget* parent = nullptr);

    /// the mouse is over the tile: Arkan wakes up and his eyes glow brighter
    void setExcited(bool excited);
    /// a speech bubble next to him: the agent is a chat
    void setBubbleVisible(bool visible);
    /// the dots in the bubble jump and the tesseract spins faster while the agent is thinking or answering
    void setTalking(bool talking);
    QSize sizeHint() const override { return { 96, 96 }; }

   protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

   private:
    void tick();
    void paintArkan(QPainter& painter, const QRect& area);
    void paintBubble(QPainter& painter, const QRectF& box);

    // the software renderer: a small frame with a depth buffer, scaled up with sharp pixels
    using Matrix = std::array<float, 12>;
    QPointF project(const Matrix& m, float x, float y, float z, float* depth = nullptr) const;
    void runeDot(const Matrix& m, float x, float y, float z, QRgb color);
    void drawBox(const Matrix& m, const float* box, bool head);
    void face(const Matrix& m, const float* a, const float* b, const float* c, int su, int sv, int sw, int sh, bool head);

    QTimer m_timer;
    QElapsedTimer m_clock;
    double m_time = 0;
    double m_lastFrame = 0;
    double m_phase = 0;  // the time of the movements that go faster while he works
    double m_wake = 0;   // 0 calm .. 1 eyes bright, eases towards the target
    double m_busy = 0;   // 0 calm .. 1 working, eases towards the target
    double m_talk = 0;
    bool m_excited = false;
    bool m_bubble = false;
    bool m_talking = false;

    QImage m_frame;
    std::vector<float> m_depth;
    float m_scale = 1, m_centerX = 0, m_centerY = 0;
    bool m_blink = false;
    std::array<float, 3> m_tesseract{};  // where the tesseract is, before the projection: it lights the cloth
    float m_tesseractLight = 0;
};
