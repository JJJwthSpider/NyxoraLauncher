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

#include <QIcon>
#include <QPixmap>
#include <QWidget>

class QLabel;
class QPushButton;
class LaunchTile;
class MoonyWidget;
class SculkAmbience;

/// The lobby of Vellora Client, made like Millida Launcher's: picture tiles on the left, the player drawn full-length
/// from the skin in the middle, Arkan the wizard on the right and "Сегодня играем" with a big Play button at the bottom,
/// all over the red rain.
class VelloraLobby : public QWidget {
    Q_OBJECT
   public:
    explicit VelloraLobby(QWidget* parent = nullptr);

    /// the player: the nickname and the skin texture (PNG); empty: no account yet
    void setPlayer(const QString& name, const QByteArray& skinPng);
    /// the installation the Play button starts (empty name: none yet)
    void setToday(const QString& name, const QString& info, const QIcon& icon, bool running);

    /// the front of a Minecraft skin as a 16x32 figure (head, body, arms, legs with their outer layers)
    static QImage renderBody(const QImage& skin);
    /// a dark figure for when there is no skin yet
    static QImage silhouette();

   signals:
    void playRequested();
    void chooseRequested();
    void agentRequested();
    void packsRequested();
    void newInstanceRequested();
    void friendsRequested();
    void skinRequested();

   protected:
    void paintEvent(QPaintEvent* event) override;

   private:
    SculkAmbience* m_ambience = nullptr;
    QPixmap m_scene;
    QWidget* m_body = nullptr;  // draws the player as sharp pixels
    QLabel* m_playerName = nullptr;
    QLabel* m_todayIcon = nullptr;
    QLabel* m_todayName = nullptr;
    QLabel* m_todayInfo = nullptr;
    QPushButton* m_play = nullptr;
};
