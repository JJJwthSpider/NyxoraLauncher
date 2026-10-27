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

#include <QGroupBox>

class BaseInstance;
class QLabel;

/// "Icon" section of the instance settings: shows the current icon and changes it either to one of the
/// offered icons or to any picture from the computer.
class InstanceIconBox : public QGroupBox {
    Q_OBJECT
   public:
    explicit InstanceIconBox(BaseInstance* instance, QWidget* parent = nullptr);

   protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

   private:
    void updatePreview();
    void pickOffered();
    void pickOwn();

    BaseInstance* m_instance;
    QLabel* m_preview;
};
