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
#include <cmath>

#include "MoonyWidget.h"
#include "ui/themes/SculkAmbience.h"
#include "ui/themes/SculkTheme.h"

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
    m_ambience = new SculkAmbience(this, 5);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setAttribute(Qt::WA_Hover);

    m_hoverAnimation.setDuration(180);
    m_hoverAnimation.setEasingCurve(QEasingCurve::OutCubic);
    connect(&m_hoverAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        m_hover = value.toDouble();
        update();
    });

    // the catalyst gem on top pulses slowly
    m_gemTimer.setInterval(60);
    connect(&m_gemTimer, &QTimer::timeout, this, [this] {
        m_gemPhase += 0.06;
        update(QRect(width() / 2 - 12, 0, 24, 14));
    });
    m_gemTimer.start();
}

void LaunchTile::animateHover(bool hovered)
{
    m_hoverAnimation.stop();
    m_hoverAnimation.setStartValue(m_hover);
    m_hoverAnimation.setEndValue(hovered ? 1.0 : 0.0);
    m_hoverAnimation.start();
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
    widget->setGeometry(widgetRect());
    widget->show();
}

QRect LaunchTile::pictureRect() const
{
    const int side = std::max(32, std::min(width() - 40, height() - s_captionHeight - 22));
    return { (width() - side) / 2, 14, side, side };
}

QRect LaunchTile::widgetRect() const
{
    // an animated picture (Moony with its speech bubble) gets the whole width between the corner brackets
    const auto picture = pictureRect();
    return { 16, picture.top(), width() - 32, picture.height() };
}

void LaunchTile::resizeEvent(QResizeEvent* event)
{
    QAbstractButton::resizeEvent(event);
    if (m_pictureWidget) {
        m_pictureWidget->setGeometry(widgetRect());
    }
}

void LaunchTile::enterEvent(QEnterEvent* event)
{
    QAbstractButton::enterEvent(event);
    m_hovered = true;
    emit hoverChanged(true);
    animateHover(true);
}

void LaunchTile::leaveEvent(QEvent* event)
{
    QAbstractButton::leaveEvent(event);
    m_hovered = false;
    emit hoverChanged(false);
    animateHover(false);
}

void LaunchTile::paintEvent(QPaintEvent*)
{
    // the frame, the texture, the picture and the caption only change with the hover state, the palette or the size;
    // they are rendered into two layers once, so the animations (Moony, gem, the animated background) only blit pixmaps
    const auto& pal = palette();
    const qreal dpr = devicePixelRatioF();
    const QString key = QString("%1x%2@%3|%4|%5%6%7%8|%9|")
                            .arg(width())
                            .arg(height())
                            .arg(dpr)
                            .arg(qRound(m_hover * 40))
                            .arg(isDown())
                            .arg(hasFocus())
                            .arg(m_hovered)
                            .arg(SculkTheme::isActive())
                            .arg(font().key()) +
                        pal.color(QPalette::Highlight).name() + pal.color(QPalette::Button).name() + pal.color(QPalette::Base).name() +
                        pal.color(QPalette::Light).name() + pal.color(QPalette::WindowText).name() + text() + m_subtitle +
                        QString::number(m_picture.cacheKey());
    if (key != m_layerKey) {
        m_layerKey = key;
        for (int i = 0; i < 2; i++) {
            QPixmap layer((QSizeF(size()) * dpr).toSize());
            layer.setDevicePixelRatio(dpr);
            layer.fill(Qt::transparent);
            QPainter layerPainter(&layer);
            paintLayer(layerPainter, i);
            m_layers[i] = layer;
        }
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.drawPixmap(0, 0, m_layers[0]);

    const QColor accent = pal.color(QPalette::Highlight).lighter(isDown() ? 100 : 135);
    const QRectF r = QRectF(rect()).adjusted(3.5, 3.5, -3.5, -3.5);
    // the veins of the sculk body pulse and particles rise, between the body and the decoration
    if (SculkTheme::isActive()) {
        painter.save();
        painter.setClipRect(r.adjusted(s_chamfer, 2, -s_chamfer, -2));
        m_ambience->paint(&painter, r.toRect(), QPoint(), pal.color(QPalette::Highlight), 3);
        painter.restore();
    }
    painter.drawPixmap(0, 0, m_layers[1]);

    // the catalyst gem in the middle of the top edge
    QPolygonF gem;
    const QPointF top(r.center().x(), r.top());
    gem << QPointF(top.x(), top.y() - 4) << QPointF(top.x() + 5, top.y()) << QPointF(top.x(), top.y() + 4) << QPointF(top.x() - 5, top.y());
    QColor gemGlow = accent;
    gemGlow.setAlphaF(0.25 + 0.25 * std::sin(m_gemPhase));
    painter.setPen(Qt::NoPen);
    painter.setBrush(gemGlow);
    painter.drawEllipse(top, 9, 6);
    painter.setPen(QPen(pal.color(QPalette::Base), 1));
    painter.setBrush(accent);
    painter.drawPolygon(gem);
}

void LaunchTile::paintLayer(QPainter& painter, int layer)
{
    painter.setRenderHint(QPainter::Antialiasing, true);

    const auto& pal = palette();
    const QColor accent = pal.color(QPalette::Highlight).lighter(isDown() ? 100 : 135);
    const QColor idleBorder = hasFocus() ? accent : pal.color(QPalette::Light);
    const QColor border = QColor::fromRgbF(idleBorder.redF() + (accent.redF() - idleBorder.redF()) * m_hover,
                                           idleBorder.greenF() + (accent.greenF() - idleBorder.greenF()) * m_hover,
                                           idleBorder.blueF() + (accent.blueF() - idleBorder.blueF()) * m_hover);
    const QRectF r = QRectF(rect()).adjusted(3.5, 3.5, -3.5, -3.5);
    const auto path = framePath(r);

    if (layer == 0) {
        // outer glow while hovered, fading in and out
        if (m_hover > 0.01) {
            QColor glow = accent;
            glow.setAlphaF(0.12 * m_hover);
            painter.setPen(QPen(glow, 10));
            painter.drawPath(path);
            glow.setAlphaF(0.25 * m_hover);
            painter.setPen(QPen(glow, 5));
            painter.drawPath(path);
        }

        // body
        QLinearGradient body(r.topLeft(), r.bottomLeft());
        body.setColorAt(0, isDown() ? pal.color(QPalette::Highlight).darker(220) : pal.color(QPalette::Button));
        body.setColorAt(1, pal.color(QPalette::Base));
        painter.setPen(QPen(border, 1.5));
        painter.setBrush(body);
        painter.drawPath(path);
        // the body is a block of sculk
        if (SculkTheme::isActive()) {
            painter.save();
            painter.setClipPath(path);
            painter.setOpacity(0.55);
            painter.drawTiledPixmap(r, SculkTheme::texture(pal.color(QPalette::Button), pal.color(QPalette::Highlight), 3));
            painter.restore();
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(path);
        }
        return;
    }

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

ProfileButton::ProfileButton(QWidget* parent) : QAbstractButton(parent)
{
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover);
    setFixedHeight(52);
}

void ProfileButton::setProfile(const QString& name, const QPixmap& face)
{
    setText(name);
    m_face = face;
    update();
}

void ProfileButton::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const auto& pal = palette();
    const bool hovered = underMouse();
    const QRectF r = QRectF(rect()).adjusted(1.5, 1.5, -1.5, -1.5);
    painter.setPen(QPen(hovered ? pal.color(QPalette::Highlight).lighter(135) : pal.color(QPalette::Light), 1.5));
    painter.setBrush(isDown() ? pal.color(QPalette::Highlight).darker(220) : pal.color(QPalette::Button));
    painter.drawRoundedRect(r, 10, 10);

    // the face is pixel art: scale without smoothing
    const QRect faceRect(10, (height() - 32) / 2, 32, 32);
    if (!m_face.isNull()) {
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.drawPixmap(faceRect, m_face);
    } else {
        painter.fillRect(faceRect, pal.color(QPalette::Base));
    }

    const QRect textRect(faceRect.right() + 10, 6, width() - faceRect.right() - 30, height() - 12);
    auto nameFont = font();
    nameFont.setBold(true);
    painter.setFont(nameFont);
    painter.setPen(hovered ? pal.color(QPalette::Highlight).lighter(135) : pal.color(QPalette::WindowText));
    painter.drawText(textRect.adjusted(0, 0, 0, -textRect.height() / 2), Qt::AlignLeft | Qt::AlignBottom,
                     fontMetrics().elidedText(text(), Qt::ElideRight, textRect.width()));
    auto subFont = font();
    subFont.setPointSizeF(subFont.pointSizeF() * 0.85);
    painter.setFont(subFont);
    painter.setPen(pal.color(QPalette::PlaceholderText));
    painter.drawText(textRect.adjusted(0, textRect.height() / 2 + 1, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("Skins and nickname"));

    // a small arrow: there is a menu behind the button
    painter.setPen(QPen(pal.color(QPalette::Highlight).lighter(135), 1.8));
    const QPointF c(width() - 14, height() / 2.0);
    painter.drawLine(c + QPointF(-4, -2), c + QPointF(0, 2));
    painter.drawLine(c + QPointF(0, 2), c + QPointF(4, -2));
}

LaunchPanel::LaunchPanel(QWidget* parent) : QWidget(parent)
{
    setObjectName("launchPanel");
    setFixedWidth(200);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 10, 8, 10);
    layout->setSpacing(10);

    m_agentTile = new LaunchTile(this);
    m_moony = new MoonyWidget;
    m_agentTile->setPictureWidget(m_moony);
    m_moony->setBubbleVisible(true);
    connect(m_agentTile, &LaunchTile::hoverChanged, m_moony, &MoonyWidget::setExcited);
    connect(m_agentTile, &LaunchTile::hoverChanged, m_moony, &MoonyWidget::setTalking);
    connect(m_agentTile, &QAbstractButton::clicked, this, &LaunchPanel::agentRequested);
    layout->addWidget(m_agentTile, 1);

    m_packsTile = new LaunchTile(this);
    connect(m_packsTile, &QAbstractButton::clicked, this, &LaunchPanel::packsRequested);
    layout->addWidget(m_packsTile, 1);

    m_newTile = new LaunchTile(this);
    connect(m_newTile, &QAbstractButton::clicked, this, &LaunchPanel::newInstanceRequested);
    layout->addWidget(m_newTile, 1);
    updatePictures();

    m_profile = new ProfileButton(this);
    connect(m_profile, &QAbstractButton::clicked, this, &LaunchPanel::profileRequested);
    layout->addWidget(m_profile);

    retranslate();
}

void LaunchPanel::retranslate()
{
    m_agentTile->setText(tr("AI agent"));
    m_agentTile->setSubtitle(tr("A pack from your description"));
    m_agentTile->setToolTip(tr("Describe a pack in your own words and Moony assembles it with mods from Modrinth and CurseForge"));
    m_packsTile->setText(tr("Ready packs"));
    m_packsTile->setSubtitle(tr("From Modrinth and CurseForge"));
    m_packsTile->setToolTip(tr("Install a ready-made mod pack from the internet"));
    m_newTile->setText(tr("New instance"));
    m_newTile->setSubtitle(tr("Version, loader and mods"));
    m_newTile->setToolTip(tr("Create an instance yourself: choose the version and the loader, then add mods"));
    m_profile->setToolTip(tr("Change your skin, cape or nickname"));
    setProfile(m_profileName, m_profileFace);
}

void LaunchPanel::setProfile(const QString& name, const QPixmap& face)
{
    m_profileName = name;
    m_profileFace = face;
    m_profile->setProfile(name.isEmpty() ? tr("Sign in") : name, face);
}

void LaunchPanel::updatePictures()
{
    const bool lunar = SculkTheme::isLunar();
    m_packsTile->setPicture(QIcon(lunar ? ":/sculk_theme/tile-wrenches-lunar.svg" : ":/sculk_theme/tile-wrenches.svg"));
    m_newTile->setPicture(QIcon(lunar ? ":/sculk_theme/tile-plus-lunar.svg" : ":/sculk_theme/tile-plus.svg"));
}

void LaunchPanel::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) {
        updatePictures();
    }
}

void LaunchPanel::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    if (SculkTheme::isActive()) {
        // tinted glass over the sculk of the main window, like the tool bars
        QColor glass = palette().color(QPalette::Window);
        glass.setAlphaF(0.55);
        painter.fillRect(rect(), glass);
    } else {
        painter.fillRect(rect(), palette().color(QPalette::Window));
    }
    painter.setPen(palette().color(QPalette::Light));
    painter.drawLine(rect().topRight(), rect().bottomRight());
}
