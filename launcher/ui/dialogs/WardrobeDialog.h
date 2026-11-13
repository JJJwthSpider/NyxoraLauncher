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

#include "ui/widgets/VelloraSkinGen.h"

class QGridLayout;
class QLabel;
class QPushButton;
class VelloraCharacter;

/// The wardrobe of Nyxora, like Millida Launcher's: the player on the left (a click turns it around), the tabs on the
/// right (outfits, head, body, back, pets, emotes) and the items as tiles; the capes and skins open the account's skin
/// manager, where the real Minecraft capes are put on.
class WardrobeDialog : public QDialog {
    Q_OBJECT
   public:
    /// `wearSkin` puts a generated skin on the account (true when it worked)
    WardrobeDialog(const QImage& skin,
                   const QImage& cape,
                   std::function<void()> openSkins,
                   std::function<bool(const QImage&, bool slim)> wearSkin,
                   bool slim = false,
                   QWidget* parent = nullptr);

    /// the cape made by the generator and chosen to wear in Nyxora (a null image: none)
    static QImage generatedCape();

   private:
    QWidget* generator(bool capes);
    /// what was chosen and made in the generator, kept while the wardrobe is open
    struct GenState {
        SkinGen::Options options;
        QString prompt;
        QList<SkinGen::Skin> skins;
        QList<QImage> capes;
        int selected = 0;
        qint64 seed = 0;
        bool made = false;
    };
    GenState m_skinGen, m_capeGen;
    void showTab(const QString& tab);
    /// look: head, body, back, pet, emote, effect
    void addTile(const QString& title, const QString& rarity, bool worn, const QStringList& look, std::function<void()> action);

    VelloraCharacter* m_character = nullptr;
    QGridLayout* m_grid = nullptr;
    class QScrollArea* m_scroll = nullptr;
    QLabel* m_section = nullptr;
    QList<QPushButton*> m_tabs;
    QString m_tab = "outfits";
    QImage m_skin, m_cape;
    bool m_slim = false;
    std::function<bool(const QImage&, bool)> m_wearSkin;
    qint64 m_seed = 0;
    int m_count = 0;
    int columnCount() const;
};
