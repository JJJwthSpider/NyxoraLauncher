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

#include "InstanceImportTask.h"

namespace ModpackAgent {

/// Creates an instance from the pack the agent assembled.
/// The pack is written as a Modrinth .mrpack and then imported like any downloaded modpack,
/// which downloads every mod, checks its hash and installs Minecraft with the loader.
class AgentImportTask : public InstanceImportTask {
    Q_OBJECT
   public:
    AgentImportTask(const QByteArray& index, QWidget* parent);
    ~AgentImportTask() override;

   protected:
    void executeTask() override;

   private:
    static QString makePackPath();

    QByteArray m_index;
    QString m_packPath;
};

}  // namespace ModpackAgent
