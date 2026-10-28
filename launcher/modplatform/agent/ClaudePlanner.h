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

#include <QObject>
#include <QPointer>

#include "ModpackPlan.h"

class QNetworkReply;

namespace ModpackAgent {

/// Asks Claude to turn a free-form request into a pack plan (themes, search terms, well known mods).
/// Only used when the user entered an Anthropic API key; otherwise the local parser is used.
class ClaudePlanner : public QObject {
    Q_OBJECT
   public:
    explicit ClaudePlanner(QObject* parent = nullptr);

    /// requestedMods / excluded: what the built-in rules read from the conversation, passed on so Claude respects it
    void plan(const QString& request, const QString& apiKey, const QStringList& requestedMods = {}, const QStringList& excluded = {});
    void abort();

   signals:
    void planned(ModpackAgent::Plan plan);
    void failed(QString reason);

   private:
    void onFinished();

    QPointer<QNetworkReply> m_reply;
    QString m_request;
};

}  // namespace ModpackAgent
