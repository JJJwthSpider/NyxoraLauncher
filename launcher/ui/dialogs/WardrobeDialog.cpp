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
#include "WardrobeDialog.h"

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

#include "ui/themes/VelloraSound.h"

#include <QAbstractButton>
#include <QCoreApplication>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "ui/widgets/VelloraCharacter.h"

namespace {
/// a wardrobe tile: the item on a dark figure and its name; a crimson frame when it is worn
class ItemTile : public QAbstractButton {
   public:
    QString head, body, back, pet, emote, effect;
    QColor frame = QColor(0x8A, 0x93, 0xA0);  // the rarity
    bool worn = false;
    explicit ItemTile(QWidget* parent) : QAbstractButton(parent)
    {
        setCursor(Qt::PointingHandCursor);
        setFixedSize(136, 156);
    }

   protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        const QRectF r = rect().adjusted(2, 2, -2, -2);
        p.fillRect(r, worn ? QColor(0x4A, 0x14, 0x20) : QColor(0x2B, 0x0E, 0x15));
        p.setPen(QPen(worn ? QColor(0xFF, 0x5A, 0x6E) : (underMouse() ? frame.lighter(130) : frame), 2));
        p.drawRect(r);
        p.fillRect(QRectF(r.left(), r.bottom() - 4, r.width(), 4), worn ? QColor(0x8E, 0x14, 0x28) : QColor(0x12, 0x04, 0x0A));
        VelloraCharacter::paintItems(p, QRectF(r.left() + 6, r.top() + 6, r.width() - 12, r.height() - 58), head, body, back, pet, emote, effect, 1.3);
        p.setPen(QColor(0xFB, 0xEF, 0xF1));
        QFont f = font();
        f.setBold(true);
        f.setPointSizeF(f.pointSizeF() * 0.9);
        p.setFont(f);
        p.drawText(QRectF(r.left() + 4, r.bottom() - 50, r.width() - 8, 30), Qt::AlignCenter | Qt::TextWordWrap, text());
        if (worn) {
            p.setPen(QColor(0xFF, 0x80, 0x90));
            p.drawText(QRectF(r.left(), r.bottom() - 19, r.width(), 14), Qt::AlignCenter, QObject::tr("worn"));
        }
    }
};
}  // namespace

WardrobeDialog::WardrobeDialog(const QImage& skin, const QImage& cape, std::function<void()> openSkins, QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Wardrobe"));
    resize(1060, 640);
    auto* layout = new QHBoxLayout(this);

    auto* left = new QVBoxLayout;
    m_character = new VelloraCharacter(this);
    m_character->setSkin(skin);
    m_character->setCape(cape);
    m_character->setMinimumWidth(340);
    left->addWidget(m_character, 1);
    auto* hint = new QLabel(tr("Click the player to turn around"), this);
    hint->setAlignment(Qt::AlignCenter);
    left->addWidget(hint);
    auto* skins = new QPushButton(tr("Skins and capes..."), this);
    skins->setToolTip(tr("The real Minecraft capes of your account: everyone sees them in the game"));
    connect(skins, &QPushButton::clicked, this, [openSkins] { openSkins(); });
    left->addWidget(skins);
    auto* undress = new QPushButton(tr("Take everything off"), this);
    connect(undress, &QPushButton::clicked, this, [this] {
        for (const auto& slot : { Cosmetics::Head, Cosmetics::Body, Cosmetics::Back, Cosmetics::Pet, Cosmetics::Effect }) {
            Cosmetics::wear(slot, QString());
        }
        VelloraSound::play("close");
        showTab(m_tab);
    });
    left->addWidget(undress);
    layout->addLayout(left, 4);

    auto* right = new QVBoxLayout;
    auto* tabs = new QHBoxLayout;
    const std::pair<QString, QString> names[] = { { "outfits", tr("Outfits") }, { Cosmetics::Head, tr("Head") }, { Cosmetics::Body, tr("Body") },
                                                  { Cosmetics::Back, tr("Back") },  { Cosmetics::Pet, tr("Pets") },  { Cosmetics::Effect, tr("Effects") },
                                                  { Cosmetics::Emote, tr("Emotes") } };
    for (const auto& [id, name] : names) {
        auto* tab = new QPushButton(name, this);
        tab->setCheckable(true);
        tab->setProperty("tab", id);
        connect(tab, &QPushButton::clicked, this, [this, id] { showTab(id); });
        tabs->addWidget(tab);
        m_tabs << tab;
    }
    tabs->addStretch(1);
    right->addLayout(tabs);
    m_section = new QLabel(this);
    m_section->setStyleSheet("font-size: 14pt; font-weight: bold;");
    right->addWidget(m_section);
    auto* scroll = new QScrollArea(this);
    m_scroll = scroll;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* holder = new QWidget(scroll);
    m_grid = new QGridLayout(holder);
    m_grid->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    scroll->setWidget(holder);
    right->addWidget(scroll, 1);
    layout->addLayout(right, 6);
    showTab(m_tab);
}

void WardrobeDialog::addTile(const QString& title, const QString& rarity, bool worn, const QStringList& look, std::function<void()> action)
{
    auto* tile = new ItemTile(this);
    tile->setText(title);
    tile->worn = worn;
    tile->frame = Cosmetics::rarityColor(rarity);
    tile->head = look.value(0);
    tile->body = look.value(1);
    tile->back = look.value(2);
    tile->pet = look.value(3);
    tile->emote = look.value(4);
    tile->effect = look.value(5);
    connect(tile, &QAbstractButton::clicked, this, [action] { action(); });
    m_grid->addWidget(tile, m_count / 4, m_count % 4);
    m_count++;
}

void WardrobeDialog::showTab(const QString& tab)
{
    if (tab != m_tab && m_scroll && m_scroll->widget()) {
        // like Millida's tabs: the items slide in from the side of the new tab and fade up, with a blip
        VelloraSound::play("nav");
        int from = 0, to = 0;
        for (int i = 0; i < m_tabs.size(); i++) {
            from = m_tabs.at(i)->property("tab").toString() == m_tab ? i : from;
            to = m_tabs.at(i)->property("tab").toString() == tab ? i : to;
        }
        QWidget* holder = m_scroll->widget();
        auto* effect = new QGraphicsOpacityEffect(holder);
        holder->setGraphicsEffect(effect);
        auto* fade = new QPropertyAnimation(effect, "opacity", holder);
        fade->setDuration(240);
        fade->setStartValue(0.0);
        fade->setEndValue(1.0);
        fade->setEasingCurve(QEasingCurve::OutCubic);
        connect(fade, &QPropertyAnimation::finished, holder, [holder] { holder->setGraphicsEffect(nullptr); });
        auto* slide = new QPropertyAnimation(holder, "pos", holder);
        slide->setDuration(240);
        slide->setStartValue(QPoint(to >= from ? 30 : -30, 0));
        slide->setEndValue(QPoint(0, 0));
        slide->setEasingCurve(QEasingCurve::OutCubic);
        fade->start(QAbstractAnimation::DeleteWhenStopped);
        slide->start(QAbstractAnimation::DeleteWhenStopped);
    }
    m_tab = tab;
    for (auto* button : m_tabs) {
        button->setChecked(button->property("tab").toString() == tab);
    }
    while (auto* item = m_grid->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    m_count = 0;
    m_character->setBack(tab == Cosmetics::Back);
    if (tab == "outfits") {
        m_section->setText(tr("Outfits — all open"));
        for (const auto& outfit : Cosmetics::outfits()) {
            const bool worn = Cosmetics::worn(Cosmetics::Head) == outfit.head && Cosmetics::worn(Cosmetics::Back) == outfit.back &&
                              Cosmetics::worn(Cosmetics::Body) == outfit.body;
            addTile(Cosmetics::title(outfit), "legendary", worn, { outfit.head, outfit.body, outfit.back, outfit.pet, QString(), outfit.effect },
                    [this, outfit] {
                        Cosmetics::wearOutfit(outfit);
                        VelloraSound::play("wear");
                        showTab("outfits");
                    });
        }
        return;
    }
    const auto list = Cosmetics::inSlot(tab);
    m_section->setText(tab == Cosmetics::Emote ? tr("Emotes: click and the player plays it") : QString());
    for (auto* button : m_tabs) {
        if (button->property("tab").toString() == tab && tab != Cosmetics::Emote) {
            m_section->setText(QString("%1 · %2").arg(button->text()).arg(list.size()));
        }
    }
    for (const auto& item : list) {
        const bool worn = tab != Cosmetics::Emote && Cosmetics::worn(tab) == item.id;
        const QStringList look = { tab == Cosmetics::Head ? item.id : QString(), tab == Cosmetics::Body ? item.id : QString(),
                                   tab == Cosmetics::Back ? item.id : QString(), tab == Cosmetics::Pet ? item.id : QString(),
                                   tab == Cosmetics::Emote ? item.id : QString(), tab == Cosmetics::Effect ? item.id : QString() };
        addTile(Cosmetics::title(item), item.rarity, worn, look, [this, tab, item] {
            if (tab == Cosmetics::Emote) {
                VelloraSound::play("toggle");
                m_character->playEmote(item.id);
                return;
            }
            const bool putOn = Cosmetics::worn(tab) != item.id;
            VelloraSound::play(putOn ? "wear" : "click");
            Cosmetics::wear(tab, putOn ? item.id : QString());
            showTab(tab);
        });
    }
}
