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

#include <QAbstractButton>
#include <QIcon>
#include <QPixmap>
#include <QTimer>
#include <QVariantAnimation>
#include <QWidget>

class QPainter;
class WardenWidget;
class SculkAmbience;

/// A big button in the sculk frame that all launch tiles share:
/// chamfered corners, glowing corner brackets, little sculk veins and a catalyst gem on top.
class LaunchTile : public QAbstractButton {
    Q_OBJECT
   public:
    explicit LaunchTile(QWidget* parent = nullptr);

    void setSubtitle(const QString& subtitle);
    /// a static picture, e.g. the crossed wrenches
    void setPicture(const QIcon& icon);
    /// an animated picture (the Warden); the tile owns and places it
    void setPictureWidget(QWidget* widget);

    QSize sizeHint() const override { return { 184, 176 }; }
    QSize minimumSizeHint() const override { return { 150, 150 }; }

   signals:
    void hoverChanged(bool hovered);

   protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

   private:
    QRect pictureRect() const;
    QRect widgetRect() const;
    void animateHover(bool hovered);

    QString m_subtitle;
    QIcon m_picture;
    QWidget* m_pictureWidget = nullptr;
    bool m_hovered = false;
    double m_hover = 0;  // 0..1, animated so the glow fades in and out smoothly
    double m_gemPhase = 0;
    QVariantAnimation m_hoverAnimation;
    QTimer m_gemTimer;
    SculkAmbience* m_ambience = nullptr;

    /// 0: glow, body and texture; 1: brackets, veins, picture and caption
    void paintLayer(QPainter& painter, int layer);
    QString m_layerKey;
    QPixmap m_layers[2];
};

/// The player's face and nickname at the bottom of the launch panel; opens skins and nickname settings.
class ProfileButton : public QAbstractButton {
    Q_OBJECT
   public:
    explicit ProfileButton(QWidget* parent = nullptr);
    void setProfile(const QString& name, const QPixmap& face);
    QSize sizeHint() const override { return { 184, 52 }; }

   protected:
    void paintEvent(QPaintEvent* event) override;

   private:
    QPixmap m_face;
};

/// The column on the left of the main window: the mod pack agent (animated Warden),
/// ready-made packs from the internet (crossed wrenches) and a new instance (white plus).
class LaunchPanel : public QWidget {
    Q_OBJECT
   public:
    explicit LaunchPanel(QWidget* parent = nullptr);

    void retranslate();
    /// shows the face and nickname of the active account (empty name: no account yet)
    void setProfile(const QString& name, const QPixmap& face);
    ProfileButton* profileButton() const { return m_profile; }

   signals:
    void agentRequested();
    void profileRequested();
    void packsRequested();
    void newInstanceRequested();

   protected:
    void paintEvent(QPaintEvent* event) override;

   private:
    LaunchTile* m_agentTile = nullptr;
    LaunchTile* m_packsTile = nullptr;
    LaunchTile* m_newTile = nullptr;
    WardenWidget* m_warden = nullptr;
    ProfileButton* m_profile = nullptr;
    QString m_profileName;
    QPixmap m_profileFace;
};
