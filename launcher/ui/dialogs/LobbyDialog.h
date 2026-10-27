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
#include <QPointer>

class BaseInstance;
class LaunchTask;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QFrame;
namespace Lobby {
class TunnelModInstaller;
}

/// Playing with friends. "Create a lobby" prepares an instance (installs e4mc when it has a mod loader), starts the
/// game and picks the address out of the game log as soon as the world is opened to LAN; "Join" connects to a friend's
/// address straight from the launcher.
class LobbyDialog : public QDialog {
    Q_OBJECT
   public:
    explicit LobbyDialog(BaseInstance* preselected, QWidget* parent = nullptr);

   signals:
    /// the host wants to give the pack to friends as .mrpack
    void exportRequested(BaseInstance* instance);
    /// the guest wants to import the pack a friend sent
    void importRequested();

   private:
    QWidget* buildHostTab(BaseInstance* preselected);
    QWidget* buildJoinTab(BaseInstance* preselected);
    static void fillInstances(QComboBox* combo, BaseInstance* preselected);
    static BaseInstance* instanceOf(const QComboBox* combo);

    void updateHostInfo();
    void createLobby();
    void startGame();
    void attachLog(LaunchTask* task);
    void scanLine(const QString& line);
    void setStatus(const QString& text);
    QString invitation() const;
    void join();

    // host tab
    QComboBox* m_hostInstance = nullptr;
    QLabel* m_hostInfo = nullptr;
    QPushButton* m_createButton = nullptr;
    QLabel* m_hostStatus = nullptr;
    QFrame* m_addressBox = nullptr;
    QLabel* m_addressLabel = nullptr;
    QLabel* m_lanLabel = nullptr;
    QPushButton* m_copyAddress = nullptr;
    QPushButton* m_copyInvite = nullptr;
    Lobby::TunnelModInstaller* m_installer = nullptr;
    QPointer<BaseInstance> m_running;
    QMetaObject::Connection m_logConnection;
    QMetaObject::Connection m_runningConnection;
    QString m_internetAddress;
    QString m_lanAddress;

    // join tab
    QLineEdit* m_joinAddress = nullptr;
    QComboBox* m_joinInstance = nullptr;
};
