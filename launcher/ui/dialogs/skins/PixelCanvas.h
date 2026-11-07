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

#include <QImage>
#include <QList>
#include <QWidget>

#include "minecraft/skins/SkinArt.h"

/// A zoomed pixel editor for a skin or cape texture: shows where each part of the body lies,
/// paints with the chosen tool (optionally mirrored to the other side of the body) and keeps an
/// undo history.
class PixelCanvas : public QWidget {
    Q_OBJECT
   public:
    enum class Tool { Pencil, Eraser, Fill, Picker, Shade };

    explicit PixelCanvas(QWidget* parent = nullptr);

    /// a new texture; with resetHistory the undo history starts over, otherwise the change can be undone
    void setImage(const QImage& image, bool resetHistory);
    QImage image() const { return m_image; }

    void setBoxes(const QList<SkinArt::Box>& boxes);
    void setTool(Tool tool) { m_tool = tool; }
    void setColor(const QColor& color) { m_color = color; }
    void setMirror(bool mirror) { m_mirror = mirror; }
    void setGridVisible(bool visible);
    /// dims everything but the second layer (or the first) to make it easier to see what is painted where
    void setHighlightOverlay(int mode);  // 0 none, 1 first layer, 2 second layer

    bool canUndo() const { return !m_undo.isEmpty(); }
    bool canRedo() const { return !m_redo.isEmpty(); }
    void undo();
    void redo();

    QSize sizeHint() const override { return { 512, 512 }; }

   signals:
    void changed();
    void colorPicked(QColor color);
    void hovered(QString description);

   protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

   private:
    QRect targetRect() const;
    QPoint pixelAt(const QPointF& position) const;
    void apply(QPoint pixel, bool secondary);
    void paintPixel(QPoint pixel, const QColor& color);
    void floodFill(QPoint pixel, const QColor& color);
    void pushUndo();

    QImage m_image;
    QList<SkinArt::Box> m_boxes;
    QList<SkinArt::Face> m_faces;
    Tool m_tool = Tool::Pencil;
    QColor m_color = Qt::black;
    bool m_mirror = false;
    bool m_grid = true;
    int m_highlight = 0;
    bool m_drawing = false;
    bool m_secondary = false;
    QPoint m_lastPixel{ -1, -1 };
    QPoint m_hoverPixel{ -1, -1 };
    QList<QImage> m_undo;
    QList<QImage> m_redo;
};
