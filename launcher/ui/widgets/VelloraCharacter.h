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

#include <QColor>
#include <array>
#include <vector>
#include <QElapsedTimer>
#include <QHash>
#include <QStringList>
#include <QImage>
#include <QTimer>
#include <QWidget>

/// The wardrobe of Vellora, like the cosmetics of Millida Launcher: pixel-art items drawn in code on the player in the
/// launcher (the same items as the Android app, android/src/.../Cosmetics.java). Only the real Minecraft capes of the
/// account are seen in the game; the rest is for the launcher.
namespace Cosmetics {
inline const QString Head = "head", Body = "body", Back = "back", Pet = "pet", Effect = "effect", Emote = "emote";

struct Layer {
    double x = 0, y = 0;
    QStringList rows;
    QHash<QChar, QColor> colors;
    QString view, anim;
};

struct Item {
    QString id, slot, titleRu, titleEn, rarity;
    QList<Layer> layers;
    QList<QColor> particleColors;
    int particleCount = 0;
    QString motion, shape;
};
const QList<Item>& items();
QList<Item> inSlot(const QString& slot);
const Item* find(const QString& id);
QString title(const Item& item);
/// the frame colour of a rarity, like Millida's: grey, blue, purple, gold
QColor rarityColor(const QString& rarity);

struct Outfit {
    QString titleRu, titleEn, head, back, body, pet, effect;
};
const QList<Outfit>& outfits();
QString title(const Outfit& outfit);

QString worn(const QString& slot);
void wear(const QString& slot, const QString& id);
void wearOutfit(const Outfit& outfit);
}  // namespace Cosmetics

/// The player in 3D, like the character of Millida Launcher's lobby: the Minecraft model (head, body, arms, legs with the
/// second layer of the skin and the cape) drawn from the skin by a small software renderer with a depth buffer, pixel
/// sharp. It breathes, looks around and swings its arms; dragging turns it, a click plays an animation (wave, jump, dance,
/// spin, bow, cheer). The wardrobe items are built in 3D from their pixels; effects fly around in 3D. The same renderer as
/// the Android app (CharacterView.java).
class VelloraCharacter : public QWidget {
    Q_OBJECT
   public:
    explicit VelloraCharacter(QWidget* parent = nullptr);

    void setSkin(const QImage& skin);
    void setCape(const QImage& cape);
    void setSlim(bool slim);
    void setBack(bool back);
    void playEmote(const QString& id);
    /// turns the player to this angle (degrees, 0: facing us) and keeps it there for a while
    void turnTo(double degrees);
    /// false: drawn still (the small pictures of the skin generator)
    void setAnimated(bool animated);
    /// the head and shoulders, big (a lobby tile)
    void setCloseUp(bool closeUp);
    /// false: only the skin, without the wardrobe items (the generator's variants)
    void setShowItems(bool show);
    /// draws a still figure (no skin) with these items into `target`, for the wardrobe tiles
    static void paintItems(QPainter& painter, const QRectF& target, const QString& head, const QString& body, const QString& back,
                           const QString& pet, const QString& emote = QString(), const QString& effect = QString(), double time = 0);

   protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

   private:
    using Mat = std::array<float, 12>;
    struct Box3 {
        int part;
        float x0, y0, z0, x1, y1, z1;
        QRgb color;
    };
    void drawBox(const Mat& m, float x, float y, float z, float w, float h, float d, int u, int v, const QImage& tex, float inflate, bool mirror = false);
    void face(const Mat& m, float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz, const QImage* tex, int su, int sv,
              int sw, int sh, QRgb solid, bool mirror);
    void solidBox(const Mat& m, const Box3& b);
    void drawItem(const Cosmetics::Item* item, const QString& slot, double time);
    void drawParticles(const Cosmetics::Item* item, double time);
    void dot(float sx, float sy, float z, int size, QColor color, double alpha);
    QPointF project(const Mat& m, float x, float y, float z, float* depth = nullptr) const;
    static QList<Box3> build(const Cosmetics::Item& item, const QString& slot);

    QImage m_skin;
    QImage m_cape;
    bool m_slim = false;
    bool m_back = false;
    bool m_animated = true;
    bool m_closeUp = false;
    bool m_showItems = true;
    QString m_emote;
    QElapsedTimer m_clock;
    qint64 m_emoteStart = -100000;
    QTimer m_timer;
    float m_yaw = 0.42f, m_yawSpeed = 0, m_pitch = -0.17f;
    bool m_dragging = false;
    QPoint m_down;
    int m_lastX = 0;
    qint64 m_downTime = 0, m_lastTouch = -100000;
    int m_bufW = 0, m_bufH = 0;
    std::vector<QRgb> m_color;
    std::vector<float> m_depth;
    float m_scale = 1, m_cx = 0, m_cy = 0;
    std::array<Mat, 10> m_mats;
};
