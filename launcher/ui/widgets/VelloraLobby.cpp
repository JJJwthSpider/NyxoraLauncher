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
#include "VelloraLobby.h"

#include <QAbstractButton>
#include <QEvent>
#include <QHBoxLayout>
#include <QtMath>
#include <functional>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <algorithm>
#include <QMenu>

#include "LaunchPanel.h"
#include "MoonyWidget.h"
#include "VelloraCharacter.h"
#include "ui/themes/SculkAmbience.h"
#include "ui/themes/VelloraSound.h"

namespace {
QLabel* caption(const QString& text, int pointSize, const QString& color, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setStyleSheet(QString("color: %1; font-size: %2pt; font-weight: bold; background: transparent;").arg(color).arg(pointSize));
    return label;
}
}  // namespace

namespace {
/// a pixel speaker with sound waves, or crossed out
QIcon speakerIcon(bool on)
{
    const QStringList rows = on ? QStringList{ "....w.......", "...ww..w....", "..www...w...", "wwwww.w..w..", "wwwww..w.w..", "wwwww..w.w..",
                                               "wwwww.w..w..", "..www...w...", "...ww..w....", "....w......." }
                                : QStringList{ "....w.......", "...ww.......", "..www.r...r.", "wwwww..r.r..", "wwwww...r...", "wwwww..r.r..",
                                               "wwwww.r...r.", "..www.......", "...ww.......", "....w......." };
    const int u = 2;
    QPixmap pixmap(12 * u, 10 * u);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    for (int j = 0; j < rows.size(); j++) {
        for (int i = 0; i < rows.at(j).size(); i++) {
            const QChar c = rows.at(j).at(i);
            if (c == 'w') {
                p.fillRect(i * u, j * u, u, u, QColor(0xfb, 0xef, 0xf1));
            } else if (c == 'r') {
                p.fillRect(i * u, j * u, u, u, QColor(0xff, 0x46, 0x54));
            }
        }
    }
    p.end();
    return QIcon(pixmap);
}
}  // namespace

namespace {
/// a pixel note, or crossed out
QIcon noteIcon(bool on)
{
    const QStringList rows = { "....wwwww...", "....w...w...", "....w...w...", "....w...w...", "....w...w...", "..www.www...", ".wwww.www...", "..ww...w...." };
    const int u = 2;
    QPixmap pixmap(12 * u, 10 * u);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    for (int j = 0; j < rows.size(); j++) {
        for (int i = 0; i < rows.at(j).size(); i++) {
            if (rows.at(j).at(i) == 'w') {
                p.fillRect(i * u, (j + 1) * u, u, u, on ? QColor(0xfb, 0xef, 0xf1) : QColor(0x8a, 0x6a, 0x70));
            }
        }
    }
    if (!on) {
        for (int k = 0; k < 10; k++) {
            p.fillRect((k + 1) * u, k * u, u, u, QColor(0xff, 0x46, 0x54));
        }
    }
    p.end();
    return QIcon(pixmap);
}
}  // namespace

namespace {
QIcon pixelIcon(const QStringList& rows, const QColor& color)
{
    const int u = 2;
    int w = 0;
    for (const QString& r : rows) {
        w = std::max(w, int(r.size()));
    }
    QPixmap pixmap(w * u, int(rows.size()) * u);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    for (int j = 0; j < rows.size(); j++) {
        for (int i = 0; i < rows.at(j).size(); i++) {
            if (rows.at(j).at(i) == '#') {
                p.fillRect(i * u, j * u, u, u, color);
            }
        }
    }
    p.end();
    return QIcon(pixmap);
}
}  // namespace

QIcon VelloraLobby::gearIcon()
{
    return pixelIcon({ "....##....", ".#.####.#.", "..######..", ".###..###.", "###....###", "###....###", ".###..###.", "..######..",
                       ".#.####.#.", "....##...." },
                     QColor(0xfb, 0xef, 0xf1));
}

QIcon VelloraLobby::dotsIcon()
{
    return pixelIcon({ "..........", "..........", "..........", "..........", "##..##..##", "##..##..##", "..........", "..........",
                       "..........", ".........." },
                     QColor(0xfb, 0xef, 0xf1));
}

QIcon VelloraLobby::peopleIcon()
{
    return pixelIcon({ "...##.......", "..####..##..", "..####.####.", "...##..####.", "..####..##..", ".######.###.", ".######.####",
                       ".######.####", "............", "............" },
                     QColor(0xfb, 0xef, 0xf1));
}

QImage VelloraLobby::silhouette()
{
    // no skin yet (not signed in): a dark player-shaped shadow with a crimson rim
    QImage out(16, 32, QImage::Format_ARGB32);
    out.fill(Qt::transparent);
    QPainter p(&out);
    const QRect parts[] = { { 4, 0, 8, 8 }, { 4, 8, 8, 12 }, { 0, 8, 4, 12 }, { 12, 8, 4, 12 }, { 4, 20, 4, 12 }, { 8, 20, 4, 12 } };
    for (const auto& r : parts) {
        p.fillRect(r, QColor(0x8a, 0x34, 0x42));
        p.fillRect(r.adjusted(1, 1, -1, -1), QColor(0x2b, 0x0e, 0x15));
    }
    p.fillRect(QRect(6, 4, 1, 1), QColor(0xff, 0x46, 0x54));
    p.fillRect(QRect(9, 4, 1, 1), QColor(0xff, 0x46, 0x54));
    p.end();
    return out;
}

QImage VelloraLobby::renderBody(const QImage& source)
{
    QImage skin = source.convertToFormat(QImage::Format_ARGB32);
    QImage out(16, 32, QImage::Format_ARGB32);
    out.fill(Qt::transparent);
    if (skin.width() < 64 || skin.height() < 32) {
        return out;
    }
    const bool modern = skin.height() >= 64;
    QPainter p(&out);
    auto part = [&](int x, int y, int w, int h, int toX, int toY, bool mirror) {
        QImage piece = skin.copy(x, y, w, h);
        if (mirror) {
            piece = piece.mirrored(true, false);
        }
        p.drawImage(toX, toY, piece);
    };
    part(8, 8, 8, 8, 4, 0, false);     // head
    part(20, 20, 8, 12, 4, 8, false);  // body
    part(44, 20, 4, 12, 0, 8, false);  // right arm
    part(4, 20, 4, 12, 4, 20, false);  // right leg
    if (modern) {
        part(36, 52, 4, 12, 12, 8, false);  // left arm
        part(20, 52, 4, 12, 8, 20, false);  // left leg
        part(20, 36, 8, 12, 4, 8, false);   // jacket
        part(44, 36, 4, 12, 0, 8, false);   // right sleeve
        part(52, 52, 4, 12, 12, 8, false);  // left sleeve
        part(4, 36, 4, 12, 4, 20, false);   // right trouser
        part(4, 52, 4, 12, 8, 20, false);   // left trouser
    } else {
        part(44, 20, 4, 12, 12, 8, true);
        part(4, 20, 4, 12, 8, 20, true);
    }
    part(40, 8, 8, 8, 4, 0, false);  // hat
    p.end();
    return out;
}

/// A tile of the lobby, drawn the way Millida Launcher draws its tiles, in Vellora's burgundy: a dark outline with cut
/// corners, a plate in the colour of the section, hard rays behind the picture, the picture standing on the tile (its top
/// rises over the edge) and the name on a dense band over the bottom; a "!" or a ribbon calls for attention.
class VelloraHubTile : public QAbstractButton {
   public:
    using Painter = std::function<void(QPainter&, const QRectF&)>;

    VelloraHubTile(const QColor& plate, const QString& title, const QString& subtitle, Painter art, QWidget* parent)
        : QAbstractButton(parent), m_plate(plate), m_subtitle(subtitle), m_art(std::move(art))
    {
        setText(title);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    }
    void setRibbon(const QString& text) { m_ribbon = text; update(); }
    void setAlert(bool on) { m_alert = on; update(); }
    QSize sizeHint() const override { return { 210, 140 }; }
    QSize minimumSizeHint() const override { return { 210, 110 }; }

   protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        const int over = 18;  // the picture rises this much over the plate
        const QRectF tile(0, over, width(), height() - over);
        const int s = 3;
        auto notched = [&](const QRectF& r, qreal cut, const QColor& c) {
            p.fillRect(r.adjusted(cut, 0, -cut, 0), c);
            p.fillRect(r.adjusted(0, cut, 0, -cut), c);
        };
        const bool hot = underMouse() || isDown();
        const QColor plate = hot ? m_plate.lighter(112) : m_plate;
        notched(tile, 2 * s, m_plate.darker(330));
        notched(tile.adjusted(s, s, -s, -s), s, plate);
        p.fillRect(QRectF(tile.left() + 2 * s, tile.top() + s, tile.width() - 4 * s, s), m_plate.lighter(140));
        // hard rays from a point near the top right
        p.save();
        p.setClipRect(tile.adjusted(2 * s, 2 * s, -2 * s, -2 * s));
        p.setRenderHint(QPainter::Antialiasing);
        QColor ray = m_plate.lighter(160);
        ray.setAlpha(80);
        const QPointF c(tile.left() + tile.width() * 0.62, tile.top() + tile.height() * 0.3);
        const qreal radius = std::max(tile.width(), tile.height()) * 1.4;
        for (int i = 0; i < 16; i++) {
            const qreal a0 = qDegreesToRadians(4 + i * 22.5), a1 = qDegreesToRadians(4 + i * 22.5 + 9);
            QPolygonF wedge;
            wedge << c << c + QPointF(std::cos(a0), std::sin(a0)) * radius << c + QPointF(std::cos(a1), std::sin(a1)) * radius;
            p.setPen(Qt::NoPen);
            p.setBrush(ray);
            p.drawPolygon(wedge);
        }
        p.restore();
        // the picture, pressed to the band, its top may rise over the plate
        const int band = m_subtitle.isEmpty() ? 30 : 44;
        p.save();
        p.setClipRect(QRectF(2 * s, 0, width() - 4 * s, height() - 2 * s));
        const qreal jump = hot ? -4 : 0;
        m_art(p, QRectF(2 * s, jump, width() - 4 * s, height() - band - 2 * s - jump));
        p.restore();
        // the band with the name
        const QRectF strip(2 * s, height() - band - 2 * s, width() - 4 * s, band);
        p.fillRect(strip, m_plate.darker(220));
        p.fillRect(QRectF(strip.left(), strip.top(), strip.width(), 3), m_plate.darker(330));
        QFont f = font();
        f.setBold(true);
        f.setPointSizeF(f.pointSizeF() * 1.12);
        p.setFont(f);
        p.setPen(QColor(0xfb, 0xef, 0xf1));
        p.drawText(strip.adjusted(10, 5, -6, 0), Qt::AlignLeft | Qt::AlignTop, text());
        if (!m_subtitle.isEmpty()) {
            QFont small = font();
            small.setPointSizeF(small.pointSizeF() * 0.86);
            p.setFont(small);
            p.setPen(QColor(0xff, 0xc2, 0xca));
            p.drawText(strip.adjusted(10, 0, -6, -5), Qt::AlignLeft | Qt::AlignBottom, m_subtitle);
        }
        if (!m_ribbon.isEmpty()) {
            QFont r = font();
            r.setBold(true);
            r.setPointSizeF(r.pointSizeF() * 0.8);
            p.setFont(r);
            const int w = QFontMetrics(r).horizontalAdvance(m_ribbon) + 14;
            const QRectF box(width() - w - 8, over + 8, w, 20);
            p.fillRect(box, QColor(0xe8, 0x36, 0x4f));
            p.setPen(Qt::white);
            p.drawText(box, Qt::AlignCenter, m_ribbon);
        }
        if (m_alert) {
            const QRectF box(0, over - 6, 24, 24);
            notched(box, 3, QColor(0x1e, 0x04, 0x08));
            notched(box.adjusted(2, 2, -2, -2), 2, QColor(0xe8, 0x36, 0x4f));
            p.setPen(Qt::white);
            p.setFont(f);
            p.drawText(box, Qt::AlignCenter, "!");
        }
    }

   private:
    QColor m_plate;
    QString m_subtitle;
    QString m_ribbon;
    bool m_alert = false;
    Painter m_art;
};

/// the banner on the right, like Millida's promo card: dark, with a crimson pixel edge at the bottom
class VelloraBanner : public QFrame {
   public:
    using QFrame::QFrame;

   protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        const int s = 3;
        p.fillRect(rect().adjusted(2 * s, 0, -2 * s, 0), QColor(0x06, 0x01, 0x03));
        p.fillRect(rect().adjusted(0, 2 * s, 0, -2 * s), QColor(0x06, 0x01, 0x03));
        p.fillRect(rect().adjusted(s, s, -s, -s), QColor(0x14, 0x06, 0x0a));
        const QColor reds[] = { QColor(0xe8, 0x36, 0x4f), QColor(0xb3, 0x26, 0x3a), QColor(0xff, 0x6b, 0x7d), QColor(0x8e, 0x14, 0x28),
                                QColor(0xc8, 0x1f, 0x3a) };
        const int step = 8;
        for (int i = 0; i * step < width() - 2 * s; i++) {
            const int h = 2 + ((i * 7 + 3) % 3);
            for (int j = 0; j < h; j++) {
                p.fillRect(QRect(s + i * step, height() - s - (j + 1) * step, step, step), reds[(i * 3 + j * 5) % 5]);
            }
        }
    }
};

VelloraLobby::VelloraLobby(QWidget* parent) : QWidget(parent)
{
    setObjectName("velloraLobby");
    m_ambience = new SculkAmbience(this, 0);
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(14, 10, 14, 12);
    outer->setSpacing(8);

    auto* stage = new QHBoxLayout;
    stage->setSpacing(12);
    outer->addLayout(stage, 1);

    // left: the tiles of Millida's lobby, in burgundy: the wardrobe, my skin, playing with friends
    auto* tiles = new QVBoxLayout;
    tiles->setSpacing(6);
    auto* wardrobe = new VelloraHubTile(QColor(0xb3, 0x26, 0x3a), tr("Wardrobe"),
                                        tr("%n item(s) — all open", nullptr, int(Cosmetics::items().size())),
                                        [](QPainter& p, const QRectF& r) {
                                            VelloraCharacter::paintItems(p, r, "crown", "chain", "demon_wings", "cube_pet", QString(),
                                                                         QString(), 1.0);
                                        },
                                        this);
    wardrobe->setAlert(true);
    connect(wardrobe, &QAbstractButton::clicked, this, [this, wardrobe] {
        wardrobe->setAlert(false);
        emit wardrobeRequested();
    });
    tiles->addWidget(wardrobe, 1);
    m_skinTile = new VelloraHubTile(QColor(0x7a, 0x1e, 0x4e), tr("My skin"), tr("Upload your own"),
                                    [this](QPainter& p, const QRectF& r) {
                                        // the player's head and shoulders, big, from the skin
                                        const QImage body = m_skin.isNull() ? silhouette() : renderBody(m_skin);
                                        const qreal u = std::min(r.width() / 22.0, r.height() / 13.0);
                                        const QRectF target(r.center().x() - 8 * u, r.top() + r.height() * 0.12, 16 * u, 32 * u);
                                        p.drawImage(target, body);
                                    },
                                    this);
    connect(m_skinTile, &QAbstractButton::clicked, this, &VelloraLobby::skinRequested);
    tiles->addWidget(m_skinTile, 1);
    auto* friends = new VelloraHubTile(QColor(0x5a, 0x1a, 0x1a), tr("Play with friends"), tr("Lobby, server, invite code"),
                                       [](QPainter& p, const QRectF& r) {
                                           const QRectF half(r.left(), r.top(), r.width() / 2, r.height());
                                           VelloraCharacter::paintItems(p, half.adjusted(0, 0, 10, 0), "cat_ears", "scarf", QString(), QString(),
                                                                        QString(), QString(), 0);
                                           VelloraCharacter::paintItems(p, half.translated(r.width() / 2 - 10, 0), "headphones", QString(),
                                                                        QString(), QString(), "wave", QString(), 0);
                                       },
                                       this);
    friends->setRibbon(tr("Free"));
    connect(friends, &QAbstractButton::clicked, this, &VelloraLobby::friendsRequested);
    tiles->addWidget(friends, 1);
    stage->addLayout(tiles);

    // middle: the player with the nametag
    auto* middle = new QVBoxLayout;
    middle->addSpacing(24);
    m_playerName = caption(QString(), 13, "#ffffff", this);
    m_playerName->setStyleSheet("color: white; font-weight: bold; font-size: 12pt; background-color: rgba(0, 0, 0, 140); padding: 2px 8px;");
    middle->addWidget(m_playerName, 0, Qt::AlignHCenter);
    m_character = new VelloraCharacter(this);
    middle->addWidget(m_character, 1);
    stage->addLayout(middle, 1);

    // right: the chips in the top right corner and the banner
    auto* right = new QVBoxLayout;
    auto* chips = new QHBoxLayout;
    chips->addStretch(1);
    // the interface sounds on and off, like the sound button of Millida's lobby
    // (a drawn speaker, not an emoji: a missing emoji font made Qt's font fallback crash under Wine)
    auto* sound = new QPushButton(this);
    sound->setMinimumSize(44, 44);
    sound->setIconSize(QSize(24, 24));
    sound->setIcon(speakerIcon(VelloraSound::enabled()));
    sound->setToolTip(tr("Interface sounds"));
    connect(sound, &QPushButton::clicked, this, [sound] {
        const bool on = !VelloraSound::enabled();
        VelloraSound::setEnabled(on);
        sound->setIcon(speakerIcon(on));
        VelloraSound::play("toggle");
    });
    chips->addWidget(sound);
    auto* music = new QPushButton(this);
    music->setMinimumSize(44, 44);
    music->setIconSize(QSize(24, 24));
    music->setIcon(noteIcon(VelloraSound::musicEnabled()));
    music->setToolTip(tr("Music"));
    connect(music, &QPushButton::clicked, this, [music] {
        const bool on = !VelloraSound::musicEnabled();
        VelloraSound::setMusicEnabled(on);
        music->setIcon(noteIcon(on));
        VelloraSound::play("toggle");
    });
    chips->addWidget(music);
    auto* friendsChip = new QPushButton(peopleIcon(), tr("Friends"), this);
    friendsChip->setIconSize(QSize(24, 20));
    friendsChip->setMinimumHeight(44);
    friendsChip->setToolTip(tr("Play with friends: a lobby, a server, an invite code"));
    connect(friendsChip, &QPushButton::clicked, this, &VelloraLobby::friendsRequested);
    chips->addWidget(friendsChip);
    m_accountChip = new QPushButton(this);
    m_accountChip->setMinimumHeight(44);
    m_accountChip->setIconSize(QSize(28, 28));
    connect(m_accountChip, &QPushButton::clicked, this, &VelloraLobby::skinRequested);
    chips->addWidget(m_accountChip);
    // the settings and "⋯" (folders, help, updates), like the gear in the corner of Millida's lobby
    auto* settings = new QPushButton(this);
    settings->setMinimumSize(44, 44);
    settings->setIconSize(QSize(20, 20));
    settings->setIcon(gearIcon());
    settings->setToolTip(tr("Settings"));
    connect(settings, &QPushButton::clicked, this, &VelloraLobby::settingsRequested);
    chips->addWidget(settings);
    m_more = new QPushButton(this);
    m_more->setMinimumSize(44, 44);
    m_more->setIconSize(QSize(20, 20));
    m_more->setIcon(dotsIcon());
    m_more->setToolTip(tr("Folders, help, updates"));
    chips->addWidget(m_more);
    right->addLayout(chips);
    right->addStretch(1);
    auto* banner = new VelloraBanner(this);
    banner->setFixedSize(320, 200);
    auto* bannerLayout = new QHBoxLayout(banner);
    bannerLayout->setContentsMargins(16, 12, 6, 30);
    auto* bannerTexts = new QVBoxLayout;
    bannerTexts->setSpacing(0);
    bannerTexts->addWidget(caption(tr("AI WIZARD × VELLORA"), 8, "#d1a3aa", banner));
    bannerTexts->addSpacing(4);
    bannerTexts->addWidget(caption(tr("BUILD"), 16, "#fbeff1", banner));
    bannerTexts->addWidget(caption(tr("A PACK"), 16, "#fbeff1", banner));
    bannerTexts->addWidget(caption(tr("WITH WORDS"), 16, "#ff4654", banner));
    bannerTexts->addStretch(1);
    auto* ask = new QPushButton(tr("▶ Build"), banner);
    ask->setObjectName("accentButton");
    connect(ask, &QPushButton::clicked, this, &VelloraLobby::agentRequested);
    bannerTexts->addWidget(ask, 0, Qt::AlignLeft);
    bannerLayout->addLayout(bannerTexts, 1);
    auto* wizard = new MoonyWidget(banner);
    wizard->setExcited(true);
    wizard->setFixedSize(116, 136);
    bannerLayout->addWidget(wizard, 0, Qt::AlignTop);
    right->addWidget(banner, 0, Qt::AlignRight);
    right->addStretch(1);
    stage->addLayout(right);

    // bottom, in the middle: what we play today and the big Play button
    auto* bottom = new QHBoxLayout;
    bottom->setSpacing(4);
    bottom->addStretch(1);
    auto* today = new QFrame(this);
    today->setObjectName("todayBar");
    today->setStyleSheet(
        "QFrame#todayBar { background-color: rgba(42, 10, 18, 240); border: 2px solid #8a3442; border-bottom: 5px solid #3a0d16; "
        "border-radius: 2px; }");
    today->setCursor(Qt::PointingHandCursor);
    today->installEventFilter(this);
    m_today = today;
    auto* todayLayout = new QHBoxLayout(today);
    todayLayout->setContentsMargins(10, 8, 16, 8);
    m_todayIcon = new QLabel(today);
    m_todayIcon->setFixedSize(60, 60);
    m_todayIcon->setStyleSheet("background-color: #3a0d16; border: 2px solid #0b0305;");
    m_todayIcon->setAlignment(Qt::AlignCenter);
    todayLayout->addWidget(m_todayIcon);
    auto* texts = new QVBoxLayout;
    texts->setSpacing(0);
    texts->addWidget(caption(tr("TODAY WE PLAY"), 9, "#ff8090", today));
    m_todayName = caption(QString(), 17, "#fbeff1", today);
    texts->addWidget(m_todayName);
    m_todayInfo = new QLabel(today);
    m_todayInfo->setStyleSheet("color: #d1a3aa; background: transparent; border: none;");
    texts->addWidget(m_todayInfo);
    // like Millida's card: it says it opens the packs
    m_change = new QLabel(tr("Change the pack ›"), today);
    m_change->setStyleSheet("color: #ff8090; background: transparent; border: none; font-weight: bold;");
    texts->addWidget(m_change);
    todayLayout->addLayout(texts, 1);
    today->setMinimumWidth(420);
    bottom->addWidget(today, 3);
    m_play = new QPushButton(tr("▶  Play"), this);
    m_play->setObjectName("accentButton");
    m_play->setMinimumSize(240, 84);
    m_play->setStyleSheet("font-size: 22pt;");
    connect(m_play, &QPushButton::clicked, this, &VelloraLobby::playRequested);
    // the hours played, small at the bottom of the button
    auto* playLayout = new QVBoxLayout(m_play);
    playLayout->setContentsMargins(0, 0, 0, 6);
    playLayout->addStretch(1);
    m_playHours = new QLabel(m_play);
    m_playHours->setAlignment(Qt::AlignCenter);
    m_playHours->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_playHours->setStyleSheet("color: rgba(255, 255, 255, 200); font-size: 9pt; font-weight: bold; background: transparent;");
    m_playHours->hide();
    playLayout->addWidget(m_playHours);
    bottom->addWidget(m_play, 1);
    bottom->addStretch(1);
    outer->addLayout(bottom);

    setToday(QString(), QString(), QIcon(), false);
}

bool VelloraLobby::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_today && event->type() == QEvent::MouseButtonRelease) {
        emit chooseRequested();  // like Millida: the card opens the packs
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void VelloraLobby::setPlayer(const QString& name, const QByteArray& skinPng, const QByteArray& capePng, bool slim, bool guest)
{
    m_character->setSlim(slim);
    m_playerName->setText(name.isEmpty() ? tr("Sign in to play") : name);
    QImage skin, cape;
    if (!skinPng.isEmpty()) {
        skin.loadFromData(skinPng, "PNG");
    }
    if (!capePng.isEmpty()) {
        cape.loadFromData(capePng, "PNG");
    }
    m_skin = skin;
    m_character->setSkin(skin);
    m_character->setCape(cape);
    m_skinTile->update();
    // the account chip: the head from the skin and the name, like Millida's
    QImage head(8, 8, QImage::Format_ARGB32);
    head.fill(QColor(0x2b, 0x0e, 0x15));
    if (skin.width() >= 64) {
        QPainter p(&head);
        p.drawImage(0, 0, skin.copy(8, 8, 8, 8));
        p.drawImage(0, 0, skin.copy(40, 8, 8, 8));
    }
    m_accountChip->setIcon(QIcon(QPixmap::fromImage(head.scaled(28, 28))));
    m_accountChip->setText(name.isEmpty() ? tr("Sign in") : guest ? name + "  ˅\n" + tr("Guest") : name + "  ˅");
    m_accountChip->setToolTip(guest ? tr("A guest: instances, mods and the wardrobe; the AI and the game itself need a Microsoft account")
                                    : QString());
}

void VelloraLobby::refreshCharacter()
{
    m_character->update();
}

void VelloraLobby::setToday(const QString& name, const QString& info, const QIcon& icon, bool running)
{
    m_hasToday = !name.isEmpty();
    m_running = running;
    if (name.isEmpty()) {
        m_todayName->setText(tr("Nothing yet"));
        m_todayInfo->setText(tr("Create an instance or pick a ready-made pack"));
        m_todayIcon->setPixmap(QIcon(":/sculk_theme/tile-plus-lunar.svg").pixmap(48, 48));
        m_change->setText(tr("Choose a pack ›"));
    } else {
        m_todayName->setText(name);
        m_todayInfo->setText(info);
        m_todayIcon->setPixmap(icon.pixmap(48, 48));
        m_change->setText(tr("Change the pack ›"));
    }
    updatePlayButton();
}

void VelloraLobby::setPlayed(qint64 seconds)
{
    m_played = seconds;
    updatePlayButton();
}

void VelloraLobby::setMoreMenu(QMenu* menu)
{
    connect(m_more, &QPushButton::clicked, this, [this, menu] {
        // under the button, its right edge on the button's: it is in the corner of the window
        menu->exec(m_more->mapToGlobal(QPoint(m_more->width() - menu->sizeHint().width(), m_more->height())));
    });
}

void VelloraLobby::updatePlayButton()
{
    m_playHours->hide();
    if (!m_hasToday) {
        m_play->setText(tr("+  New"));
        return;
    }
    if (m_running) {
        m_play->setText(tr("■  Playing"));
        return;
    }
    // the hours played under "Play", like Millida's button: a reason to come back
    const qint64 hours = m_played / 3600, minutes = (m_played % 3600) / 60;
    const QString played = m_played <= 0 ? QString() : hours > 0 ? tr("%1 h played").arg(hours) : tr("%1 min played").arg(std::max<qint64>(1, minutes));
    m_play->setText(tr("▶  Play"));
    m_playHours->setText(played);
    m_playHours->setVisible(!played.isEmpty());
}

void VelloraLobby::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    const qreal dpr = devicePixelRatioF();
    if (m_scene.size() != size() * dpr) {
        m_scene = SculkAmbience::moonScene(size(), dpr);
    }
    painter.drawPixmap(0, 0, m_scene);
    m_ambience->paint(&painter, rect(), QPoint(), QColor(232, 54, 79));
}
