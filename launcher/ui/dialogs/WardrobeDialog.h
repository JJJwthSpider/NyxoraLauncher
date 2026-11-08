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
#include <QStringList>
#include <functional>

class QGridLayout;
class QLabel;
class QPushButton;
class VelloraCharacter;

/// The wardrobe of Vellora, like Millida Launcher's: the player on the left (a click turns it around), the tabs on the
/// right (outfits, head, body, back, pets, emotes) and the items as tiles; the capes and skins open the account's skin
/// manager, where the real Minecraft capes are put on.
class WardrobeDialog : public QDialog {
    Q_OBJECT
   public:
    WardrobeDialog(const QImage& skin, const QImage& cape, std::function<void()> openSkins, QWidget* parent = nullptr);

   private:
    void showTab(const QString& tab);
    /// look: head, body, back, pet, emote, effect
    void addTile(const QString& title, const QString& rarity, bool worn, const QStringList& look, std::function<void()> action);

    VelloraCharacter* m_character = nullptr;
    QGridLayout* m_grid = nullptr;
    QLabel* m_section = nullptr;
    QList<QPushButton*> m_tabs;
    QString m_tab = "outfits";
    int m_count = 0;
};
