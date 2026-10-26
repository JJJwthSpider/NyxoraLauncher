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
#include "AgentImportTask.h"

#include <QDir>
#include <QFile>
#include <QUuid>

#include "archive/ArchiveWriter.h"

namespace ModpackAgent {

QString AgentImportTask::makePackPath()
{
    return QDir::temp().filePath(QString("sculk-agent-%1.mrpack").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
}

AgentImportTask::AgentImportTask(const QByteArray& index, QWidget* parent)
    : InstanceImportTask(QUrl::fromLocalFile(makePackPath()), true, parent), m_index(index)
{}

AgentImportTask::~AgentImportTask()
{
    if (!m_packPath.isEmpty()) {
        QFile::remove(m_packPath);
    }
}

void AgentImportTask::executeTask()
{
    m_packPath = sourceUrl().toLocalFile();
    MMCZip::ArchiveWriter pack(m_packPath);
    if (!pack.open() || !pack.addFile("modrinth.index.json", m_index) || !pack.close()) {
        emitFailed(tr("Could not write the mod pack file %1.").arg(m_packPath));
        return;
    }
    InstanceImportTask::executeTask();
}

}  // namespace ModpackAgent
