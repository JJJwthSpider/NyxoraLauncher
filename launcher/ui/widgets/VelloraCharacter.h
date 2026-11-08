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

#include <QElapsedTimer>
#include <QImage>
#include <QTimer>
#include <QWidget>

/// The wardrobe of Vellora, like the cosmetics of Millida Launcher: pixel-art items drawn in code on the player in the
/// launcher (the same items as the Android app, android/src/.../Cosmetics.java). Only the real Minecraft capes of the
/// account are seen in the game; the rest is for the launcher.
namespace Cosmetics {
inline const QString Head = "head", Body = "body", Back = "back", Pet = "pet", Emote = "emote";

struct Item {
    QString id, slot;
    const char* title;  // translated through QCoreApplication::translate("Cosmetics", ...)
};
const QList<Item>& items();
QList<Item> inSlot(const QString& slot);
QString title(const Item& item);

struct Outfit {
    const char* title;
    QString head, back, body, pet;
};
const QList<Outfit>& outfits();

QString worn(const QString& slot);
void wear(const QString& slot, const QString& id);
}  // namespace Cosmetics

/// The player, alive: drawn from the skin with sharp pixels, breathing, wings moving, the pet hopping, emotes; a click
/// turns it around to show the back with the cape. `preview` shows one item without wearing it.
class VelloraCharacter : public QWidget {
    Q_OBJECT
   public:
    explicit VelloraCharacter(QWidget* parent = nullptr);

    void setSkin(const QImage& skin);
    void setCape(const QImage& cape);
    void setBack(bool back);
    void playEmote(const QString& id);
    /// draws a still figure (no skin) with these items into `target`, for the wardrobe tiles
    static void paintItems(QPainter& painter, const QRectF& target, const QString& head, const QString& body, const QString& back,
                           const QString& pet, const QString& emote = QString());

   protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

   private:
    QImage m_skin;
    QImage m_cape;
    bool m_back = false;
    QString m_emote;
    QElapsedTimer m_clock;
    qint64 m_emoteStart = 0;
    QTimer m_timer;
};
