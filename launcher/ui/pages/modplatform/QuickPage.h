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

#include "meta/Version.h"
#include "meta/VersionList.h"
#include "tasks/Task.h"
#include "ui/pages/BasePage.h"

class NewInstanceDialog;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;

/// The simplest way to make an instance: pick a Minecraft version and a loader, everything else is automatic.
class QuickPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit QuickPage(NewInstanceDialog* dialog, QWidget* parent = nullptr);
    ~QuickPage() override;

    QString displayName() const override { return tr("Quick start"); }
    QIcon icon() const override { return QIcon::fromTheme("new"); }
    QString id() const override { return "quick"; }
    QString helpPage() const override { return "Vanilla-platform"; }
    bool shouldDisplay() const override { return true; }
    void openedImpl() override;
    void retranslate() override;

   private:
    struct LoaderInfo {
        QString key;  // "", "neoforge", "fabric", "forge", "quilt"
        QString uid;
        QString name;
    };

    void loadMinecraftVersions();
    void fillMinecraftVersions();
    void loadLoaderVersions();
    void fillLoaderVersions();
    void updateLoaderDescription();
    void suggestCurrent();
    const LoaderInfo& currentLoader() const;

    NewInstanceDialog* m_dialog = nullptr;

    QList<LoaderInfo> m_loaders;
    Meta::VersionList::Ptr m_minecraftList;
    Meta::VersionList::Ptr m_loaderList;
    Task::Ptr m_minecraftTask;
    Task::Ptr m_loaderTask;
    QList<Meta::Version::Ptr> m_minecraftVersions;
    QList<Meta::Version::Ptr> m_loaderVersions;

    QLabel* m_title = nullptr;
    QLabel* m_versionLabel = nullptr;
    QComboBox* m_versionBox = nullptr;
    QCheckBox* m_snapshots = nullptr;
    QLabel* m_loaderLabel = nullptr;
    QButtonGroup* m_loaderGroup = nullptr;
    QList<QPushButton*> m_loaderButtons;
    QLabel* m_loaderDescription = nullptr;
    QLabel* m_loaderVersionLabel = nullptr;
    QComboBox* m_loaderVersionBox = nullptr;
    QLabel* m_status = nullptr;
    QLabel* m_tip = nullptr;
};
