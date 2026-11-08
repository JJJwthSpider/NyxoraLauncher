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

#include <QJsonObject>
#include <QWidget>

#include "meta/Version.h"
#include "meta/VersionList.h"
#include "tasks/Task.h"

class InstanceTask;
class QButtonGroup;
class QCheckBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QToolButton;
class QVBoxLayout;

/// The "Create" tab of Vellora Client, made like Millida Launcher's "Новая сборка": the name, the loader as tiles with
/// a sign (a loader that does not exist for the version says why), the Minecraft version as a grid with a search and a
/// snapshots switch, "Build with AI" and "Create"; next to it the ready packs: ideas Arkan assembles and the most popular
/// modpacks on Modrinth, installed with one click.
class VelloraCreatePage : public QWidget {
    Q_OBJECT
   public:
    explicit VelloraCreatePage(QWidget* parent = nullptr);

   signals:
    /// an instance to create; `openMods`: open the mod browser right after
    void taskReady(InstanceTask* task, bool openMods);
    /// Arkan, with these words to start from (empty: just the chat)
    void agentRequested(const QString& text);
    void catalogRequested();

   protected:
    void showEvent(QShowEvent* event) override;

   private:
    struct Loader {
        QString key, uid, name, note;
    };
    void loadVersions();
    void fillVersions();
    void updateLoaders();
    void updateName();
    QString selectedVersion() const;
    QString autoName() const;
    QString whyNot(const QString& loader, const QString& version) const;
    void create();
    void createWith(const Meta::Version::Ptr& minecraft, const Loader& loader);
    void loadPacks();
    QWidget* packRow(const QJsonObject& hit);
    void installPack(const QString& projectId, const QString& title);
    static QIcon loaderSign(const QString& key);

    QList<Loader> m_loaders;
    QString m_loader = "fabric";
    Meta::VersionList::Ptr m_minecraftList;
    Task::Ptr m_minecraftTask;
    Meta::VersionList::Ptr m_loaderList;
    Task::Ptr m_loaderTask;

    QLineEdit* m_name = nullptr;
    QButtonGroup* m_loaderGroup = nullptr;
    QList<QToolButton*> m_loaderButtons;
    QLineEdit* m_versionSearch = nullptr;
    QCheckBox* m_snapshots = nullptr;
    QListWidget* m_versions = nullptr;
    QCheckBox* m_openMods = nullptr;
    QLabel* m_status = nullptr;
    QPushButton* m_create = nullptr;
    QVBoxLayout* m_packs = nullptr;
    QLabel* m_packsStatus = nullptr;
    bool m_packsLoaded = false;
};
