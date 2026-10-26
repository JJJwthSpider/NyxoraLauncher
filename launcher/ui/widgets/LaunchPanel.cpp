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
#include "LaunchPanel.h"

#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>

#include "WardenWidget.h"

namespace {

// the frame takes its colors from the palette, so it also fits other themes
constexpr int s_chamfer = 14;
constexpr int s_captionHeight = 58;

QPainterPath framePath(const QRectF& r)
{
    const double c = s_chamfer;
    QPainterPath path;
    path.moveTo(r.left() + c, r.top());
    path.lineTo(r.right() - c, r.top());
    path.lineTo(r.right(), r.top() + c);
    path.lineTo(r.right(), r.bottom() - c);
    path.lineTo(r.right() - c, r.bottom());
    path.lineTo(r.left() + c, r.bottom());
    path.lineTo(r.left(), r.bottom() - c);
    path.lineTo(r.left(), r.top() + c);
    path.closeSubpath();
    return path;
}

}  // namespace

LaunchTile::LaunchTile(QWidget* parent) : QAbstractButton(parent)
{
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setAttribute(Qt::WA_Hover);
}

void LaunchTile::setSubtitle(const QString& subtitle)
{
    m_subtitle = subtitle;
    update();
}

void LaunchTile::setPicture(const QIcon& icon)
{
    m_picture = icon;
    update();
}

void LaunchTile::setPictureWidget(QWidget* widget)
{
    m_pictureWidget = widget;
    widget->setParent(this);
    widget->setGeometry(pictureRect());
    widget->show();
}

QRect LaunchTile::pictureRect() const
{
    const int side = std::max(32, std::min(width() - 40, height() - s_captionHeight - 22));
    return { (width() - side) / 2, 14, side, side };
}

void LaunchTile::resizeEvent(QResizeEvent* event)
{
    QAbstractButton::resizeEvent(event);
    if (m_pictureWidget) {
        m_pictureWidget->setGeometry(pictureRect());
    }
}

void LaunchTile::enterEvent(QEnterEvent* event)
{
    QAbstractButton::enterEvent(event);
    m_hovered = true;
    emit hoverChanged(true);
    update();
}

void LaunchTile::leaveEvent(QEvent* event)
{
    QAbstractButton::leaveEvent(event);
    m_hovered = false;
    emit hoverChanged(false);
    update();
}

void LaunchTile::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const auto& pal = palette();
    const QColor accent = pal.color(QPalette::Highlight).lighter(isDown() ? 100 : 135);
    const QColor border = m_hovered || hasFocus() ? accent : pal.color(QPalette::Light);
    const QRectF r = QRectF(rect()).adjusted(3.5, 3.5, -3.5, -3.5);
    const auto path = framePath(r);

    // outer glow while hovered
    if (m_hovered) {
        QColor glow = accent;
        glow.setAlphaF(0.25);
        painter.setPen(QPen(glow, 6));
        painter.drawPath(path);
    }

    // body
    QLinearGradient body(r.topLeft(), r.bottomLeft());
    body.setColorAt(0, isDown() ? pal.color(QPalette::Highlight).darker(220) : pal.color(QPalette::Button));
    body.setColorAt(1, pal.color(QPalette::Base));
    painter.setPen(QPen(border, 1.5));
    painter.setBrush(body);
    painter.drawPath(path);

    // glowing brackets on the four chamfered corners
    painter.setPen(QPen(accent, 2.2, Qt::SolidLine, Qt::SquareCap));
    const double c = s_chamfer;
    const double arm = 10;
    auto bracket = [&painter, c, arm](QPointF corner, double dx, double dy) {
        const QPointF a(corner.x() + dx * c, corner.y());
        const QPointF b(corner.x(), corner.y() + dy * c);
        painter.drawLine(a, QPointF(a.x() + dx * arm, a.y()));
        painter.drawLine(a, b);
        painter.drawLine(b, QPointF(b.x(), b.y() + dy * arm));
    };
    bracket(r.topLeft(), 1, 1);
    bracket(r.topRight(), -1, 1);
    bracket(r.bottomLeft(), 1, -1);
    bracket(r.bottomRight(), -1, -1);

    // sculk veins: little pixel clusters creeping in from the corners
    QColor vein = pal.color(QPalette::Highlight);
    vein.setAlphaF(0.55);
    const double p = 3;
    auto veins = [&painter, vein, p](QPointF start, double dx, double dy) {
        const QPointF cells[] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 3, 1 }, { 1, 3 } };
        for (const auto& cell : cells) {
            painter.fillRect(
                QRectF(start.x() + dx * cell.x() * p - (dx < 0 ? p : 0), start.y() + dy * cell.y() * p - (dy < 0 ? p : 0), p, p), vein);
        }
    };
    veins(r.topLeft() + QPointF(6, c + 6), 1, 1);
    veins(r.topRight() + QPointF(-6, c + 6), -1, 1);
    veins(r.bottomLeft() + QPointF(6, -c - 6), 1, -1);
    veins(r.bottomRight() + QPointF(-6, -c - 6), -1, -1);

    // the catalyst gem in the middle of the top edge
    QPolygonF gem;
    const QPointF top(r.center().x(), r.top());
    gem << QPointF(top.x(), top.y() - 4) << QPointF(top.x() + 5, top.y()) << QPointF(top.x(), top.y() + 4) << QPointF(top.x() - 5, top.y());
    painter.setPen(QPen(pal.color(QPalette::Base), 1));
    painter.setBrush(accent);
    painter.drawPolygon(gem);

    // picture
    if (!m_pictureWidget && !m_picture.isNull()) {
        const auto pic = pictureRect().adjusted(8, 8, -8, -8);
        m_picture.paint(&painter, pic);
    }

    // caption
    QRectF caption(r.left() + 8, r.bottom() - s_captionHeight - 6, r.width() - 16, s_captionHeight);
    auto titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.1);
    painter.setFont(titleFont);
    painter.setPen(m_hovered ? accent : pal.color(QPalette::WindowText));
    const QRectF titleRect(caption.left(), caption.top(), caption.width(), 22);
    painter.drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter, text());
    auto subFont = font();
    subFont.setPointSizeF(subFont.pointSizeF() * 0.9);
    painter.setFont(subFont);
    painter.setPen(pal.color(QPalette::PlaceholderText));
    painter.drawText(caption.adjusted(0, 24, 0, 0), Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, m_subtitle);
}

LaunchPanel::LaunchPanel(QWidget* parent) : QWidget(parent)
{
    setObjectName("launchPanel");
    setFixedWidth(200);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 10, 8, 10);
    layout->setSpacing(10);

    m_agentTile = new LaunchTile(this);
    m_warden = new WardenWidget;
    m_agentTile->setPictureWidget(m_warden);
    connect(m_agentTile, &LaunchTile::hoverChanged, m_warden, &WardenWidget::setExcited);
    connect(m_agentTile, &QAbstractButton::clicked, this, &LaunchPanel::agentRequested);
    layout->addWidget(m_agentTile, 1);

    m_packsTile = new LaunchTile(this);
    m_packsTile->setPicture(QIcon(":/sculk_theme/tile-wrenches.svg"));
    connect(m_packsTile, &QAbstractButton::clicked, this, &LaunchPanel::packsRequested);
    layout->addWidget(m_packsTile, 1);

    m_newTile = new LaunchTile(this);
    m_newTile->setPicture(QIcon(":/sculk_theme/tile-plus.svg"));
    connect(m_newTile, &QAbstractButton::clicked, this, &LaunchPanel::newInstanceRequested);
    layout->addWidget(m_newTile, 1);

    retranslate();
}

void LaunchPanel::retranslate()
{
    m_agentTile->setText(tr("AI agent"));
    m_agentTile->setSubtitle(tr("A pack from your description"));
    m_agentTile->setToolTip(tr("Describe a pack in your own words and the Warden assembles it with mods from Modrinth and CurseForge"));
    m_packsTile->setText(tr("Ready packs"));
    m_packsTile->setSubtitle(tr("From Modrinth and CurseForge"));
    m_packsTile->setToolTip(tr("Install a ready-made mod pack from the internet"));
    m_newTile->setText(tr("New instance"));
    m_newTile->setSubtitle(tr("Version, loader and mods"));
    m_newTile->setToolTip(tr("Create an instance yourself: choose the version and the loader, then add mods"));
}

void LaunchPanel::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), palette().color(QPalette::Window));
    painter.setPen(palette().color(QPalette::Light));
    painter.drawLine(rect().topRight(), rect().bottomRight());
}
