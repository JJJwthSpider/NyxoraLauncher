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

#include <QWidget>

#include "modplatform/agent/ClaudePlanner.h"
#include "modplatform/agent/ModpackBuilder.h"
#include "ui/pages/BasePage.h"

class NewInstanceDialog;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QGroupBox;

/// "AI pack" page of the new instance dialog: the user describes a pack in their own words
/// ("сборка с зомби neoforge 1.21.1") and the agent assembles an instance with fitting mods.
class AgentPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit AgentPage(NewInstanceDialog* dialog, QWidget* parent = nullptr);
    ~AgentPage() override;

    QString displayName() const override { return tr("AI pack"); }
    QIcon icon() const override { return QIcon::fromTheme("star"); }
    QString id() const override { return "agent"; }
    QString helpPage() const override { return QString(); }
    bool shouldDisplay() const override { return true; }
    void openedImpl() override;
    void retranslate() override;

   private:
    void startBuild();
    void stopBuild();
    void startBuilder(const ModpackAgent::Plan& plan);
    void builderFinished();
    void builderFailed(const QString& reason);
    void appendLog(const QString& line);
    void setBusy(bool busy);
    void fillModTree();
    void modItemChanged(QTreeWidgetItem* item, int column);
    void suggestCurrent();

    NewInstanceDialog* m_dialog = nullptr;
    ModpackAgent::ClaudePlanner* m_planner = nullptr;
    ModpackAgent::ModpackBuilder* m_builder = nullptr;
    bool m_hasResult = false;
    bool m_fillingTree = false;

    QLabel* m_title = nullptr;
    QLabel* m_hint = nullptr;
    QLineEdit* m_request = nullptr;
    QList<QPushButton*> m_examples;
    QPushButton* m_buildButton = nullptr;
    QPushButton* m_stopButton = nullptr;
    QProgressBar* m_progress = nullptr;
    QLabel* m_summary = nullptr;
    QTreeWidget* m_modTree = nullptr;
    QPlainTextEdit* m_log = nullptr;
    QGroupBox* m_aiBox = nullptr;
    QLabel* m_aiHint = nullptr;
    QLineEdit* m_apiKey = nullptr;
};
