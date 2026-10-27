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

#include <QDialog>

#include "modplatform/server/ServerManager.h"

class BaseInstance;
class QCheckBox;
class QComboBox;
class QFrame;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;

/// "My server": the settings of a server made from an instance (PvP, game mode, difficulty, ...), start and stop,
/// the invite code for friends, the players online and the server console.
class ServerWindow : public QDialog {
    Q_OBJECT
   public:
    explicit ServerWindow(BaseInstance* preselected, QWidget* parent = nullptr);

   private:
    QWidget* buildSettings();
    QWidget* buildStatus();
    void loadSettings();
    ServerSettings collectSettings() const;
    void startOrStop();
    void updateState();
    void updateAddresses();
    QString invitation() const;

    ServerManager* m_manager;

    // settings
    QComboBox* m_instance = nullptr;
    QLabel* m_instanceInfo = nullptr;
    QComboBox* m_gamemode = nullptr;
    QComboBox* m_difficulty = nullptr;
    QCheckBox* m_pvp = nullptr;
    QCheckBox* m_hardcore = nullptr;
    QCheckBox* m_allowFlight = nullptr;
    QCheckBox* m_commandBlocks = nullptr;
    QSpinBox* m_maxPlayers = nullptr;
    QSpinBox* m_viewDistance = nullptr;
    QSpinBox* m_memory = nullptr;
    QLineEdit* m_motd = nullptr;
    QLineEdit* m_seed = nullptr;
    QCheckBox* m_whitelist = nullptr;
    QLineEdit* m_whitelistNames = nullptr;
    QCheckBox* m_internet = nullptr;
    QCheckBox* m_eula = nullptr;
    QWidget* m_settingsBox = nullptr;

    // status
    QPushButton* m_startButton = nullptr;
    QLabel* m_status = nullptr;
    QFrame* m_inviteBox = nullptr;
    QLabel* m_code = nullptr;
    QLabel* m_addresses = nullptr;
    QPushButton* m_copyCode = nullptr;
    QPushButton* m_copyInvite = nullptr;
    QListWidget* m_players = nullptr;
    QPlainTextEdit* m_console = nullptr;
    QLineEdit* m_command = nullptr;
};
