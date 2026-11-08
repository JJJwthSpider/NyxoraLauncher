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

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <algorithm>

#include "LaunchPanel.h"
#include "MoonyWidget.h"
#include "VelloraCharacter.h"
#include "ui/themes/SculkAmbience.h"

namespace {
QLabel* caption(const QString& text, int pointSize, const QString& color, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setStyleSheet(QString("color: %1; font-size: %2pt; font-weight: bold; background: transparent;").arg(color).arg(pointSize));
    return label;
}
}  // namespace

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

VelloraLobby::VelloraLobby(QWidget* parent) : QWidget(parent)
{
    setObjectName("velloraLobby");
    m_ambience = new SculkAmbience(this, 0);
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(12, 12, 12, 12);
    outer->setSpacing(10);

    auto* stage = new QHBoxLayout;
    stage->setSpacing(12);
    outer->addLayout(stage, 1);

    // left: big picture tiles, like Millida's lobby
    auto* tiles = new QVBoxLayout;
    tiles->setSpacing(10);
    auto addTile = [&](const QString& title, const QString& subtitle, const QIcon& picture, auto signal) {
        auto* tile = new LaunchTile(this);
        tile->setText(title);
        tile->setSubtitle(subtitle);
        tile->setPicture(picture);
        connect(tile, &QAbstractButton::clicked, this, signal);
        tiles->addWidget(tile, 1);
    };
    // like Millida's lobby: the wardrobe, ready packs, friends
    QPixmap outfit(128, 128);
    outfit.fill(Qt::transparent);
    {
        QPainter p(&outfit);
        VelloraCharacter::paintItems(p, QRectF(0, 0, 128, 128), "crown", "chain", "demon_wings", "cube_pet", QString(), "void_orbit", 1.0);
    }
    addTile(tr("Wardrobe"), tr("Outfits, capes, pets"), QIcon(outfit), &VelloraLobby::wardrobeRequested);
    addTile(tr("Ready-made packs"), tr("Modrinth and CurseForge"), QIcon(":/sculk_theme/tile-packs-vellora.png"), &VelloraLobby::packsRequested);
    addTile(tr("With friends"), tr("Lobby, server, invite code"), QIcon::fromTheme("accounts"), &VelloraLobby::friendsRequested);
    auto* tilesBox = new QWidget(this);
    tilesBox->setLayout(tiles);
    tilesBox->setFixedWidth(200);
    stage->addWidget(tilesBox);

    // middle: the player
    auto* middle = new QVBoxLayout;
    m_character = new VelloraCharacter(this);
    middle->addWidget(m_character, 1);
    m_playerName = caption(QString(), 16, "#fbeff1", this);
    m_playerName->setAlignment(Qt::AlignHCenter);
    middle->addWidget(m_playerName);
    auto* buttons = new QHBoxLayout;
    buttons->addStretch(1);
    auto* wardrobeButton = new QPushButton(tr("Wardrobe"), this);
    connect(wardrobeButton, &QPushButton::clicked, this, &VelloraLobby::wardrobeRequested);
    buttons->addWidget(wardrobeButton);
    auto* skinButton = new QPushButton(tr("My skin"), this);
    connect(skinButton, &QPushButton::clicked, this, &VelloraLobby::skinRequested);
    buttons->addWidget(skinButton);
    buttons->addStretch(1);
    middle->addLayout(buttons);
    stage->addLayout(middle, 1);

    // right: Arkan the wizard
    auto* arkan = new QFrame(this);
    arkan->setObjectName("arkanCard");
    arkan->setStyleSheet(
        "QFrame#arkanCard { background-color: rgba(20, 5, 9, 225); border: 2px solid #8a3442; border-bottom: 5px solid #3a0d16; "
        "border-radius: 2px; }");
    arkan->setFixedWidth(230);
    auto* arkanLayout = new QVBoxLayout(arkan);
    arkanLayout->addWidget(caption(tr("AI WIZARD"), 9, "#ff8090", arkan));
    arkanLayout->addStretch(1);
    auto* wizard = new MoonyWidget(arkan);
    wizard->setExcited(true);
    wizard->setFixedSize(200, 200);
    arkanLayout->addWidget(wizard, 0, Qt::AlignHCenter);
    arkanLayout->addWidget(caption(tr("Arkan"), 16, "#fbeff1", arkan));
    auto* text = new QLabel(tr("Describe a pack in your own words and I will build it from mods"), arkan);
    text->setWordWrap(true);
    text->setStyleSheet("color: #d1a3aa; background: transparent;");
    arkanLayout->addWidget(text);
    auto* ask = new QPushButton(tr("Build with AI"), arkan);
    ask->setObjectName("accentButton");
    connect(ask, &QPushButton::clicked, this, &VelloraLobby::agentRequested);
    arkanLayout->addWidget(ask);
    stage->addWidget(arkan);

    // bottom: what we play today
    auto* today = new QFrame(this);
    today->setObjectName("todayBar");
    today->setStyleSheet(
        "QFrame#todayBar { background-color: rgba(28, 8, 13, 235); border: 2px solid #8a3442; border-bottom: 5px solid #3a0d16; "
        "border-radius: 2px; }");
    auto* todayLayout = new QHBoxLayout(today);
    todayLayout->setContentsMargins(12, 8, 12, 8);
    m_todayIcon = new QLabel(today);
    m_todayIcon->setFixedSize(52, 52);
    todayLayout->addWidget(m_todayIcon);
    auto* texts = new QVBoxLayout;
    texts->setSpacing(0);
    texts->addWidget(caption(tr("TODAY WE PLAY"), 9, "#ff8090", today));
    m_todayName = caption(QString(), 17, "#fbeff1", today);
    texts->addWidget(m_todayName);
    m_todayInfo = new QLabel(today);
    m_todayInfo->setStyleSheet("color: #d1a3aa; background: transparent;");
    texts->addWidget(m_todayInfo);
    todayLayout->addLayout(texts, 1);
    auto* choose = new QPushButton(tr("Change"), today);
    choose->setMinimumHeight(44);
    connect(choose, &QPushButton::clicked, this, &VelloraLobby::chooseRequested);
    todayLayout->addWidget(choose);
    m_play = new QPushButton(tr("▶  Play"), today);
    m_play->setObjectName("accentButton");
    m_play->setMinimumSize(240, 56);
    m_play->setStyleSheet("font-size: 18pt;");
    connect(m_play, &QPushButton::clicked, this, &VelloraLobby::playRequested);
    todayLayout->addWidget(m_play);
    outer->addWidget(today);

    setToday(QString(), QString(), QIcon(), false);
}

void VelloraLobby::setPlayer(const QString& name, const QByteArray& skinPng, const QByteArray& capePng)
{
    m_playerName->setText(name.isEmpty() ? tr("Sign in to play") : name);
    QImage skin, cape;
    if (!skinPng.isEmpty()) {
        skin.loadFromData(skinPng, "PNG");
    }
    if (!capePng.isEmpty()) {
        cape.loadFromData(capePng, "PNG");
    }
    m_character->setSkin(skin);
    m_character->setCape(cape);
}

void VelloraLobby::refreshCharacter()
{
    m_character->update();
}

void VelloraLobby::setToday(const QString& name, const QString& info, const QIcon& icon, bool running)
{
    if (name.isEmpty()) {
        m_todayName->setText(tr("Nothing yet"));
        m_todayInfo->setText(tr("Create an instance or pick a ready-made pack"));
        m_todayIcon->setPixmap(QIcon(":/sculk_theme/tile-plus-lunar.svg").pixmap(52, 52));
        m_play->setText(tr("+  New"));
        return;
    }
    m_todayName->setText(name);
    m_todayInfo->setText(info);
    m_todayIcon->setPixmap(icon.pixmap(52, 52));
    m_play->setText(running ? tr("■  Playing") : tr("▶  Play"));
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
