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

#include "PixelCanvas.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <cmath>

namespace {
constexpr int s_maxUndo = 100;
}

PixelCanvas::PixelCanvas(QWidget* parent) : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumSize(256, 128);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCursor(Qt::CrossCursor);
}

void PixelCanvas::setImage(const QImage& image, bool resetHistory)
{
    if (resetHistory) {
        m_undo.clear();
        m_redo.clear();
    } else if (!m_image.isNull()) {
        pushUndo();
    }
    m_image = image.convertToFormat(QImage::Format_ARGB32);
    update();
    emit changed();
}

void PixelCanvas::setBoxes(const QList<SkinArt::Box>& boxes)
{
    m_boxes = boxes;
    m_faces = SkinArt::faces(boxes);
    update();
}

void PixelCanvas::setGridVisible(bool visible)
{
    m_grid = visible;
    update();
}

void PixelCanvas::setHighlightOverlay(int mode)
{
    m_highlight = mode;
    update();
}

void PixelCanvas::pushUndo()
{
    m_undo << m_image;
    if (m_undo.size() > s_maxUndo) {
        m_undo.removeFirst();
    }
    m_redo.clear();
}

void PixelCanvas::undo()
{
    if (m_undo.isEmpty()) {
        return;
    }
    m_redo << m_image;
    m_image = m_undo.takeLast();
    update();
    emit changed();
}

void PixelCanvas::redo()
{
    if (m_redo.isEmpty()) {
        return;
    }
    m_undo << m_image;
    m_image = m_redo.takeLast();
    update();
    emit changed();
}

QRect PixelCanvas::targetRect() const
{
    if (m_image.isNull()) {
        return {};
    }
    // whole pixels only, so every texture pixel is a crisp square
    const int scale = std::max(1, std::min((width() - 8) / m_image.width(), (height() - 8) / m_image.height()));
    const QSize size(m_image.width() * scale, m_image.height() * scale);
    return QRect(QPoint((width() - size.width()) / 2, (height() - size.height()) / 2), size);
}

QPoint PixelCanvas::pixelAt(const QPointF& position) const
{
    const auto target = targetRect();
    if (target.isEmpty() || !target.contains(position.toPoint())) {
        return { -1, -1 };
    }
    const double scale = double(target.width()) / m_image.width();
    return { int((position.x() - target.x()) / scale), int((position.y() - target.y()) / scale) };
}

void PixelCanvas::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), palette().color(QPalette::Base));
    const auto target = targetRect();
    if (target.isEmpty()) {
        return;
    }
    const double scale = double(target.width()) / m_image.width();
    auto toWidget = [&](const QRect& r) {
        return QRectF(target.x() + r.x() * scale, target.y() + r.y() * scale, r.width() * scale, r.height() * scale);
    };

    // a checkerboard shows transparent pixels
    const int check = std::max(4, int(scale / 2));
    for (int y = target.top(); y < target.bottom(); y += check) {
        for (int x = target.left(); x < target.right(); x += check) {
            const bool dark = ((x - target.left()) / check + (y - target.top()) / check) % 2;
            p.fillRect(QRect(x, y, check, check).intersected(target), dark ? QColor(0x3a, 0x3f, 0x44) : QColor(0x4c, 0x52, 0x58));
        }
    }
    p.drawImage(target, m_image);

    // parts of the texture that belong to no face are not used by the game
    QPainterPath unused;
    unused.addRect(target);
    QPainterPath used;
    for (const auto& face : m_faces) {
        used.addRect(toWidget(face.rect));
    }
    p.fillPath(unused.subtracted(used), QColor(0, 0, 0, 150));

    // the layer that is not highlighted fades out
    if (m_highlight != 0) {
        for (const auto& face : m_faces) {
            if (face.overlay != (m_highlight == 2)) {
                p.fillRect(toWidget(face.rect), QColor(0, 0, 0, 140));
            }
        }
    }

    if (m_grid && scale >= 6) {
        p.setPen(QPen(QColor(0, 0, 0, 45), 1));
        for (int x = 0; x <= m_image.width(); x++) {
            p.drawLine(QPointF(target.x() + x * scale, target.top()), QPointF(target.x() + x * scale, target.bottom()));
        }
        for (int y = 0; y <= m_image.height(); y++) {
            p.drawLine(QPointF(target.left(), target.y() + y * scale), QPointF(target.right(), target.y() + y * scale));
        }
    }

    // outlines of the faces: first layer white, second layer sculk blue, the face under the mouse bright
    const auto* hovered = SkinArt::faceAt(m_faces, m_hoverPixel);
    for (const auto& face : m_faces) {
        QPen pen(face.overlay ? QColor(0x29, 0xdf, 0xeb, 110) : QColor(255, 255, 255, 90), 1);
        if (face.overlay) {
            pen.setStyle(Qt::DashLine);
        }
        p.setPen(pen);
        p.drawRect(toWidget(face.rect).adjusted(0, 0, -1, -1));
    }
    if (hovered) {
        p.setPen(QPen(QColor(0xff, 0xe0, 0x6a), 2));
        p.drawRect(toWidget(hovered->rect).adjusted(1, 1, -1, -1));
    }
    if (QRect(QPoint(), m_image.size()).contains(m_hoverPixel)) {
        p.setPen(QPen(Qt::white, 1));
        p.drawRect(toWidget(QRect(m_hoverPixel, QSize(1, 1))));
        if (m_mirror) {
            const auto mirrored = SkinArt::mirrorPixel(m_boxes, m_faces, m_hoverPixel);
            if (mirrored.x() >= 0 && mirrored != m_hoverPixel) {
                p.setPen(QPen(QColor(255, 255, 255, 120), 1, Qt::DotLine));
                p.drawRect(toWidget(QRect(mirrored, QSize(1, 1))));
            }
        }
    }
}

void PixelCanvas::paintPixel(QPoint pixel, const QColor& color)
{
    if (!QRect(QPoint(), m_image.size()).contains(pixel)) {
        return;
    }
    m_image.setPixelColor(pixel, color);
    if (m_mirror) {
        const auto mirrored = SkinArt::mirrorPixel(m_boxes, m_faces, pixel);
        if (mirrored.x() >= 0) {
            m_image.setPixelColor(mirrored, color);
        }
    }
}

void PixelCanvas::floodFill(QPoint start, const QColor& color)
{
    // the fill stays on the face it starts on, so it never runs over onto another part of the body
    const auto* face = SkinArt::faceAt(m_faces, start);
    const QRect bounds = face ? face->rect : QRect(QPoint(), m_image.size());
    const QRgb from = m_image.pixel(start);
    const QRgb to = color.rgba();
    if (from == to) {
        return;
    }
    QList<QPoint> stack{ start };
    while (!stack.isEmpty()) {
        const auto p = stack.takeLast();
        if (!bounds.contains(p) || m_image.pixel(p) != from) {
            continue;
        }
        paintPixel(p, color);
        stack << QPoint(p.x() + 1, p.y()) << QPoint(p.x() - 1, p.y()) << QPoint(p.x(), p.y() + 1) << QPoint(p.x(), p.y() - 1);
    }
}

void PixelCanvas::apply(QPoint pixel, bool secondary)
{
    if (!QRect(QPoint(), m_image.size()).contains(pixel)) {
        return;
    }
    switch (m_tool) {
        case Tool::Pencil:
            paintPixel(pixel, secondary ? QColor(Qt::transparent) : m_color);
            break;
        case Tool::Eraser:
            paintPixel(pixel, Qt::transparent);
            break;
        case Tool::Fill:
            floodFill(pixel, secondary ? QColor(Qt::transparent) : m_color);
            break;
        case Tool::Picker: {
            const auto picked = m_image.pixelColor(pixel);
            if (picked.alpha() > 0) {
                emit colorPicked(picked);
            }
            return;
        }
        case Tool::Shade: {
            // left button darkens, right button lightens: quick shadows and highlights
            auto c = m_image.pixelColor(pixel);
            if (c.alpha() == 0) {
                return;
            }
            c = secondary ? c.lighter(112) : c.darker(112);
            paintPixel(pixel, c);
            break;
        }
    }
    update();
    emit changed();
}

void PixelCanvas::mousePressEvent(QMouseEvent* event)
{
    const auto pixel = pixelAt(event->position());
    if (pixel.x() < 0 || (event->button() != Qt::LeftButton && event->button() != Qt::RightButton)) {
        return;
    }
    m_secondary = event->button() == Qt::RightButton;
    if (m_tool != Tool::Picker) {
        pushUndo();
    }
    m_drawing = m_tool != Tool::Fill && m_tool != Tool::Picker;
    m_lastPixel = pixel;
    apply(pixel, m_secondary);
}

void PixelCanvas::mouseMoveEvent(QMouseEvent* event)
{
    const auto pixel = pixelAt(event->position());
    if (pixel != m_hoverPixel) {
        m_hoverPixel = pixel;
        const auto* face = SkinArt::faceAt(m_faces, pixel);
        emit hovered(face ? SkinArt::describe(m_boxes, *face) : QString());
        update();
    }
    if (!m_drawing || pixel.x() < 0 || pixel == m_lastPixel) {
        return;
    }
    // fast strokes skip pixels: fill the line between the last two positions
    const QPoint from = m_lastPixel.x() < 0 ? pixel : m_lastPixel;
    const int steps = std::max(std::abs(pixel.x() - from.x()), std::abs(pixel.y() - from.y()));
    for (int i = 1; i <= steps; i++) {
        const QPoint p(int(std::lround(from.x() + (pixel.x() - from.x()) * double(i) / steps)),
                       int(std::lround(from.y() + (pixel.y() - from.y()) * double(i) / steps)));
        if (m_tool == Tool::Shade && p != pixel) {
            continue;  // shading every pixel of the line twice would make it too dark
        }
        apply(p, m_secondary);
    }
    m_lastPixel = pixel;
}

void PixelCanvas::mouseReleaseEvent(QMouseEvent*)
{
    m_drawing = false;
    m_lastPixel = { -1, -1 };
}

void PixelCanvas::leaveEvent(QEvent*)
{
    m_hoverPixel = { -1, -1 };
    emit hovered(QString());
    update();
}
