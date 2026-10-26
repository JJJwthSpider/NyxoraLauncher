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
#include <QNetworkRequest>
#include <QPointer>
#include <QTimer>

#include "minecraft/auth/MinecraftAccount.h"

class QLabel;
class QLineEdit;
class QNetworkReply;
class QPushButton;

/// Changes the Minecraft name (nickname) of a Microsoft account through the Minecraft services API.
/// Checks while typing whether the name is free; Mojang allows one change every 30 days.
class ChangeNameDialog : public QDialog {
    Q_OBJECT
   public:
    ChangeNameDialog(MinecraftAccountPtr account, QWidget* parent = nullptr);

   private:
    void nameEdited();
    void checkAvailability();
    void apply();
    void setStatus(const QString& html, bool canApply);
    QNetworkRequest request(const QString& url) const;

    MinecraftAccountPtr m_account;
    QLineEdit* m_name = nullptr;
    QLabel* m_status = nullptr;
    QPushButton* m_applyButton = nullptr;
    QTimer m_checkTimer;
    QPointer<QNetworkReply> m_checkReply;
    QPointer<QNetworkReply> m_changeReply;
};
