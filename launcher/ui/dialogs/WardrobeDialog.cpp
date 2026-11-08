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
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFrame>
#include <QLineEdit>
#include <QButtonGroup>
#include <QIcon>
#include <QPixmap>
#include <memory>
#include <optional>
#include <QPushButton>
#include <QSettings>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>

#include "Application.h"
#include "ui/widgets/VelloraCharacter.h"
#include "ui/widgets/VelloraSkinGen.h"

namespace {
/// a wardrobe tile: the item on a dark figure and its name; a crimson frame when it is worn
class ItemTile : public QAbstractButton {
   public:
    QString head, body, back, pet, emote, effect, hand;
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
        const QRectF picture(r.left() + 6, r.top() + 6, r.width() - 12, r.height() - 58);
        if (!hand.isEmpty() && head.isEmpty() && body.isEmpty() && back.isEmpty() && pet.isEmpty() && emote.isEmpty() && effect.isEmpty()) {
            VelloraCharacter::paintIcon(p, picture.adjusted(14, 4, -14, -4), hand, 1.3);  // a thing in the hand, large
        } else {
            VelloraCharacter::paintItems(p, picture, head, body, back, pet, emote, effect, 1.3, hand);
        }
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
/// a square button of the generator: a colour, an emblem, "auto" (A) or "none" (a crossed square)
class ChipButton : public QAbstractButton {
   public:
    QRgb color = 0;
    QStringList rows;
    bool autoChip = false, none = false;
    explicit ChipButton(QWidget* parent) : QAbstractButton(parent)
    {
        setCheckable(true);
        setCursor(Qt::PointingHandCursor);
        setFixedSize(28, 28);
    }

   protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        const int w = width(), h = height();
        p.fillRect(rect(), isChecked() ? QColor(0xFF, 0x5A, 0x6E) : underMouse() ? QColor(0x8A, 0x34, 0x42) : QColor(0x5A, 0x1E, 0x2A));
        const int in = isChecked() ? 3 : 1;
        p.fillRect(QRect(in, in, w - 2 * in, h - 2 * in), QColor(0x12, 0x05, 0x08));
        const int pad = in + 2;
        if (color) {
            p.fillRect(QRect(pad, pad, w - 2 * pad, h - 2 * pad), QColor::fromRgba(color));
            p.fillRect(QRect(pad, pad, w - 2 * pad, 2), QColor(255, 255, 255, 50));
        } else if (!rows.isEmpty()) {
            const int cell = std::max(1, (std::min(w, h) - 2 * pad) / 5);
            const int ox = (w - 5 * cell) / 2, oy = (h - 5 * cell) / 2;
            for (int y = 0; y < 5 && y < rows.size(); y++) {
                for (int x = 0; x < 5; x++) {
                    if (rows[y].at(x) == '#') {
                        p.fillRect(QRect(ox + x * cell, oy + y * cell, cell, cell), QColor(0xFB, 0xEF, 0xF1));
                    }
                }
            }
        } else if (none) {
            p.setPen(QPen(QColor(0x8A, 0x34, 0x42), 2));
            p.drawLine(pad, h - pad, w - pad, pad);
        } else if (autoChip) {
            p.setPen(QColor(0xFB, 0xEF, 0xF1));
            QFont f = font();
            f.setBold(true);
            p.setFont(f);
            p.drawText(rect(), Qt::AlignCenter, "A");
        }
    }
};

/// a die drawn in pixels (the "surprise me" button): emoji on buttons are not safe on every Windows font setup
QIcon diceIcon()
{
    QPixmap pm(16, 16);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.fillRect(1, 1, 14, 14, QColor(0xFB, 0xEF, 0xF1));
    p.fillRect(1, 13, 14, 2, QColor(0xC9, 0xB8, 0xBC));
    const QColor dot(0xB3, 0x26, 0x3A);
    for (const QPoint& d : { QPoint(4, 4), QPoint(10, 4), QPoint(7, 7), QPoint(4, 10), QPoint(10, 10) }) {
        p.fillRect(d.x() - 1, d.y() - 1, 3, 3, dot);
    }
    return QIcon(pm);
}

/// a variant of the generator: the picture, the name, a crimson frame when it is the one tried on
class VariantTile : public QAbstractButton {
   public:
    explicit VariantTile(QWidget* parent) : QAbstractButton(parent)
    {
        setCheckable(true);
        setCursor(Qt::PointingHandCursor);
    }

   protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        const QRectF r = rect().adjusted(1, 1, -1, -1);
        p.fillRect(r, isChecked() ? QColor(0x3A, 0x10, 0x19) : QColor(0x1C, 0x08, 0x0D));
        p.setPen(QPen(isChecked() ? QColor(0xFF, 0x5A, 0x6E) : underMouse() ? QColor(0xB3, 0x26, 0x3A) : QColor(0x8A, 0x34, 0x42), isChecked() ? 3 : 2));
        p.drawRect(r);
        if (!text().isEmpty()) {
            p.setPen(QColor(0xFB, 0xEF, 0xF1));
            QFont f = font();
            f.setBold(true);
            p.setFont(f);
            p.drawText(QRectF(r.left(), r.bottom() - 26, r.width(), 22), Qt::AlignCenter, text());
        }
    }
};
}  // namespace

QImage WardrobeDialog::generatedCape()
{
    const QString path = QDir(APPLICATION->dataRoot()).filePath("vellora-cape.png");
    QImage cape;
    if (QSettings(QDir(APPLICATION->dataRoot()).filePath("vellora-ui.ini"), QSettings::IniFormat).value("generatedCape", false).toBool()) {
        cape.load(path);
    }
    return cape;
}

WardrobeDialog::WardrobeDialog(const QImage& skin,
                               const QImage& cape,
                               std::function<void()> openSkins,
                               std::function<bool(const QImage&, bool)> wearSkin,
                               bool slim,
                               QWidget* parent)
    : QDialog(parent), m_skin(skin), m_cape(cape), m_slim(slim), m_wearSkin(std::move(wearSkin)), m_seed(QDateTime::currentMSecsSinceEpoch())
{
    m_skinGen.seed = m_seed;
    m_capeGen.seed = m_seed + 7;
    setWindowTitle(tr("Wardrobe"));
    resize(1060, 640);
    auto* layout = new QHBoxLayout(this);

    auto* left = new QVBoxLayout;
    m_character = new VelloraCharacter(this);
    m_character->setSkin(skin);
    m_character->setCape(generatedCape().isNull() ? cape : generatedCape());
    m_character->setSlim(slim);
    m_character->setMinimumWidth(340);
    left->addWidget(m_character, 1);
    auto* hint = new QLabel(tr("Drag to turn · click for an animation"), this);
    hint->setAlignment(Qt::AlignCenter);
    left->addWidget(hint);
    auto* skins = new QPushButton(tr("Skins and capes..."), this);
    skins->setToolTip(tr("The real Minecraft capes of your account: everyone sees them in the game"));
    connect(skins, &QPushButton::clicked, this, [openSkins] { openSkins(); });
    left->addWidget(skins);
    auto* undress = new QPushButton(tr("Take everything off"), this);
    connect(undress, &QPushButton::clicked, this, [this] {
        for (const auto& slot : { Cosmetics::Head, Cosmetics::Body, Cosmetics::Back, Cosmetics::Pet, Cosmetics::Hand, Cosmetics::Effect }) {
            Cosmetics::wear(slot, QString());
        }
        VelloraSound::play("close");
        showTab(m_tab);
    });
    left->addWidget(undress);
    layout->addLayout(left, 4);

    auto* right = new QVBoxLayout;
    auto* tabs = new QHBoxLayout;
    const std::pair<QString, QString> names[] = { { "outfits", tr("Outfits") }, { "skins", tr("Skins") }, { "capes", tr("Capes") },
                                                  { Cosmetics::Head, tr("Head") }, { Cosmetics::Body, tr("Body") },
                                                  { Cosmetics::Back, tr("Back") },  { Cosmetics::Pet, tr("Pets") },  { Cosmetics::Hand, tr("In hand") }, { Cosmetics::Effect, tr("Effects") },
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
    tile->hand = look.value(6);
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
    m_character->setBack(tab == Cosmetics::Back || tab == "capes");
    if (tab == "skins" || tab == "capes") {
        m_section->setText(tab == "skins" ? tr("Arkan's AI skin generator") : tr("Arkan's AI cape generator"));
        m_grid->addWidget(generator(tab == "capes"), 0, 0, 1, 4);
        return;
    }
    if (tab == "outfits") {
        m_section->setText(tr("Outfits — all open"));
        for (const auto& outfit : Cosmetics::outfits()) {
            const bool worn = Cosmetics::worn(Cosmetics::Head) == outfit.head && Cosmetics::worn(Cosmetics::Back) == outfit.back &&
                              Cosmetics::worn(Cosmetics::Body) == outfit.body;
            addTile(Cosmetics::title(outfit), "legendary", worn, { outfit.head, outfit.body, outfit.back, outfit.pet, QString(), outfit.effect, outfit.hand },
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
                                   tab == Cosmetics::Emote ? item.id : QString(), tab == Cosmetics::Effect ? item.id : QString(),
                                   tab == Cosmetics::Hand ? item.id : QString() };
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

/// Arkan's generator: a description (it draws while it is typed; Enter or "Create" for new variants), the recent descriptions
/// and the ideas in one row (a click draws at once), "Surprise me", buttons for colours and details that repaint the same four
/// variants, the variants with the first tried on the 3D player at once, and "More variants".
QWidget* WardrobeDialog::generator(bool capes)
{
    GenState& st = capes ? m_capeGen : m_skinGen;
    QSettings ui(QDir(APPLICATION->dataRoot()).filePath("vellora-ui.ini"), QSettings::IniFormat);
    auto* panel = new QFrame(this);
    panel->setObjectName("generatorPanel");
    panel->setStyleSheet("QFrame#generatorPanel { background-color: #241018; border: 2px solid #b3263a; border-bottom: 5px solid #3a0d16; }");
    panel->setMinimumWidth(560);
    auto* layout = new QVBoxLayout(panel);
    auto* subtitle = new QLabel(capes ? tr("Describe a cape or build it with the buttons: Arkan draws at once")
                                      : tr("Describe a character or build it with the buttons: Arkan draws at once"),
                                panel);
    subtitle->setStyleSheet("color: #d1a3aa;");
    layout->addWidget(subtitle);

    auto* inputRow = new QHBoxLayout;
    auto* prompt = new QLineEdit(panel);
    prompt->setPlaceholderText(capes ? tr("For example: a purple galaxy with stars") : tr("For example: a red-haired girl wizard in a blue robe"));
    prompt->setMinimumHeight(34);
    prompt->setText(st.prompt);
    inputRow->addWidget(prompt, 1);
    auto* dice = new QPushButton(diceIcon(), tr("Surprise me"), panel);
    dice->setMinimumHeight(34);
    inputRow->addWidget(dice);
    auto* go = new QPushButton(tr("Create"), panel);
    go->setObjectName("accentButton");
    go->setMinimumHeight(34);
    inputRow->addWidget(go);
    layout->addLayout(inputRow);

    // the recent descriptions and the ideas, one row that scrolls
    auto* ideasArea = new QScrollArea(panel);
    ideasArea->setFrameShape(QFrame::NoFrame);
    ideasArea->setWidgetResizable(true);
    ideasArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ideasArea->setFixedHeight(54);
    auto* ideasHolder = new QWidget(ideasArea);
    auto* ideas = new QHBoxLayout(ideasHolder);
    ideas->setContentsMargins(0, 0, 0, 0);
    ideasArea->setWidget(ideasHolder);
    layout->addWidget(ideasArea);

    // the variants right under the description
    auto* variantsHolder = new QWidget(panel);
    auto* variants = new QHBoxLayout(variantsHolder);
    variants->setContentsMargins(0, 4, 0, 4);
    layout->addWidget(variantsHolder);
    auto* actions = new QHBoxLayout;
    layout->addLayout(actions);
    auto* status = new QLabel(panel);
    status->setWordWrap(true);
    status->setStyleSheet("color: #d1a3aa;");
    layout->addWidget(status);

    // colours and details, which fold away
    auto* customBar = new QHBoxLayout;
    auto* fold = new QPushButton(panel);
    customBar->addWidget(fold);
    customBar->addStretch(1);
    auto* reset = new QPushButton(tr("Reset"), panel);
    customBar->addWidget(reset);
    layout->addLayout(customBar);
    auto* custom = new QWidget(panel);
    auto* rows = new QGridLayout(custom);
    rows->setContentsMargins(0, 0, 0, 0);
    rows->setHorizontalSpacing(8);
    layout->addWidget(custom);
    layout->addStretch(1);
    const bool open = ui.value("generatorCustomOpen", true).toBool();
    custom->setVisible(open);
    fold->setText(open ? tr("Colours and details ▴") : tr("Colours and details ▾"));
    connect(fold, &QPushButton::clicked, this, [custom, fold] {
        const bool now = !custom->isVisible();
        custom->setVisible(now);
        fold->setText(now ? tr("Colours and details ▴") : tr("Colours and details ▾"));
        QSettings(QDir(APPLICATION->dataRoot()).filePath("vellora-ui.ini"), QSettings::IniFormat).setValue("generatorCustomOpen", now);
        VelloraSound::play("click");
    });

    auto clear = [](QLayout* l) {
        while (auto* item = l->takeAt(0)) {
            delete item->widget();
            delete item;
        }
    };

    // trying a variant on: the 3D player, the frame, the buttons under it
    auto tryOn = std::make_shared<std::function<void(int)>>();
    *tryOn = [this, capes, variants, actions, status, clear](int index) {
        GenState& st = capes ? m_capeGen : m_skinGen;
        st.selected = index;
        for (int i = 0; i < variants->count(); i++) {
            if (auto* tile = qobject_cast<QAbstractButton*>(variants->itemAt(i)->widget()); tile && tile->isCheckable()) {
                tile->setChecked(i == index);
            }
        }
        clear(actions);
        status->clear();
        if (capes) {
            if (index >= st.capes.size()) {
                return;
            }
            const QImage cape = st.capes.at(index);
            m_character->setCape(cape);
            m_character->setBack(true);
            auto* wear = new QPushButton(tr("Wear in Vellora"));
            wear->setObjectName("accentButton");
            connect(wear, &QPushButton::clicked, this, [cape, status] {
                cape.save(QDir(APPLICATION->dataRoot()).filePath("vellora-cape.png"), "PNG");
                QSettings(QDir(APPLICATION->dataRoot()).filePath("vellora-ui.ini"), QSettings::IniFormat).setValue("generatedCape", true);
                VelloraSound::play("wear");
                status->setText(tr("The cape is on in Vellora: you see it in the lobby and the wardrobe. In the game itself Minecraft shows "
                                   "only official capes."));
            });
            actions->addWidget(wear);
            auto* save = new QPushButton(tr("Save PNG..."));
            connect(save, &QPushButton::clicked, this, [this, cape] {
                const QString file = QFileDialog::getSaveFileName(this, tr("Save the cape"), "vellora-cape.png", "PNG (*.png)");
                if (!file.isEmpty()) {
                    cape.save(file, "PNG");
                }
            });
            actions->addWidget(save);
            auto* off = new QPushButton(tr("Take it off"));
            connect(off, &QPushButton::clicked, this, [this, status] {
                QSettings(QDir(APPLICATION->dataRoot()).filePath("vellora-ui.ini"), QSettings::IniFormat).setValue("generatedCape", false);
                m_character->setCape(m_cape);
                VelloraSound::play("close");
                status->setText(tr("The generated cape is off."));
            });
            actions->addWidget(off);
        } else {
            if (index >= st.skins.size()) {
                return;
            }
            const SkinGen::Skin skin = st.skins.at(index);
            m_character->setSlim(skin.slim);
            m_character->setSkin(skin.image);
            m_character->setBack(false);
            auto* wear = new QPushButton(tr("Put on the account"));
            wear->setObjectName("accentButton");
            connect(wear, &QPushButton::clicked, this, [this, skin, status] {
                if (m_wearSkin && m_wearSkin(skin.image, skin.slim)) {
                    m_skin = skin.image;
                    m_slim = skin.slim;
                    VelloraSound::play("wear");
                    status->setText(tr("The skin is on: everyone sees it in the game."));
                } else {
                    status->setText(tr("The skin was not put on (an account with Minecraft: Java Edition is needed). Save the PNG to use it later."));
                }
            });
            actions->addWidget(wear);
            auto* save = new QPushButton(tr("Save PNG..."));
            connect(save, &QPushButton::clicked, this, [this, skin] {
                const QString file = QFileDialog::getSaveFileName(this, tr("Save the skin"), "vellora-skin.png", "PNG (*.png)");
                if (!file.isEmpty()) {
                    skin.image.save(file, "PNG");
                }
            });
            actions->addWidget(save);
            auto* mine = new QPushButton(tr("My skin"));
            connect(mine, &QPushButton::clicked, this, [this] {
                m_character->setSlim(m_slim);
                m_character->setSkin(m_skin);
                VelloraSound::play("close");
            });
            actions->addWidget(mine);
        }
        actions->addStretch(1);
    };

    // drawing: `fresh` takes a new seed (new variants), otherwise the same four are repainted
    auto draw = std::make_shared<std::function<void(bool)>>();
    std::weak_ptr<std::function<void(bool)>> weakDraw = draw;  // the function must not own itself
    *draw = [this, capes, prompt, variants, clear, tryOn, weakDraw](bool fresh) {
        GenState& st = capes ? m_capeGen : m_skinGen;
        if (fresh) {
            st.seed++;
            st.selected = 0;
        }
        st.prompt = prompt->text();
        st.made = true;
        if (capes) {
            st.capes = SkinGen::capes(st.prompt, 4, st.seed, st.options);
        } else {
            st.skins = SkinGen::skins(st.prompt, 4, st.seed, st.options);
        }
        clear(variants);
        for (int i = 0; i < 4; i++) {
            auto* tile = new VariantTile(nullptr);
            auto* tileLayout = new QVBoxLayout(tile);
            tileLayout->setContentsMargins(6, 6, 6, 28);
            if (capes) {
                tile->setFixedSize(104, 160);
                auto* face = new QLabel(tile);
                face->setPixmap(QPixmap::fromImage(st.capes.at(i).copy(1, 1, 10, 16).scaled(70, 112)));
                face->setAlignment(Qt::AlignCenter);
                face->setAttribute(Qt::WA_TransparentForMouseEvents);
                tileLayout->addWidget(face);
                tile->setText(QString("#%1").arg(i + 1));
            } else {
                tile->setFixedSize(132, 196);
                auto* picture = new VelloraCharacter(tile);
                picture->setAnimated(false);
                picture->setShowItems(false);
                picture->setSlim(st.skins.at(i).slim);
                picture->setSkin(st.skins.at(i).image);
                picture->turnTo(28);
                picture->setAttribute(Qt::WA_TransparentForMouseEvents);
                tileLayout->addWidget(picture);
                tile->setText(st.skins.at(i).title);
            }
            connect(tile, &QAbstractButton::clicked, this, [tryOn, i] {
                VelloraSound::play("wear");
                (*tryOn)(i);
            });
            variants->addWidget(tile);
        }
        auto* more = new QPushButton(tr("More\nvariants"));
        more->setFixedSize(capes ? 96 : 104, capes ? 160 : 196);
        connect(more, &QPushButton::clicked, this, [weakDraw] {
            VelloraSound::play("open");
            if (auto d = weakDraw.lock()) {
                (*d)(true);
            }
        });
        variants->addWidget(more);
        variants->addStretch(1);
        (*tryOn)(std::min(st.selected, 3));
    };

    // the rows of buttons; a click repaints the same variants
    auto addRow = [this, rows, custom](const QString& title) {
        const int row = rows->rowCount();
        auto* label = new QLabel(title, custom);
        label->setStyleSheet("color: #d1a3aa;");
        rows->addWidget(label, row, 0);
        auto* line = new QHBoxLayout;
        line->setSpacing(4);
        rows->addLayout(line, row, 1);
        auto* group = new QButtonGroup(custom);
        group->setExclusive(true);
        return std::make_pair(line, group);
    };
    auto colorRow = [this, addRow, custom, draw](const QString& title, const QList<QRgb>& colors, QRgb* value) {
        auto [line, group] = addRow(title);
        QList<QRgb> all = colors;
        all.prepend(0);
        for (const QRgb c : all) {
            auto* chip = new ChipButton(custom);
            chip->color = c;
            chip->autoChip = c == 0;
            chip->setChecked(c == *value);
            chip->setToolTip(c == 0 ? tr("Auto: as the words say") : QColor::fromRgba(c).name());
            group->addButton(chip);
            connect(chip, &QAbstractButton::clicked, this, [value, c, draw] {
                *value = c;
                VelloraSound::play("click");
                (*draw)(false);
            });
            line->addWidget(chip);
        }
        line->addStretch(1);
    };
    auto textRow = [this, addRow, custom, draw](const QString& title, const QList<std::pair<QString, QString>>& choices, const QString& chosen,
                                                std::function<void(const QString&)> set) {
        auto [line, group] = addRow(title);
        QList<std::pair<QString, QString>> all = choices;
        all.prepend({ QString(), tr("Auto") });
        for (const auto& [id, name] : all) {
            auto* b = new QPushButton(name, custom);
            b->setCheckable(true);
            b->setChecked(id == chosen);
            group->addButton(b);
            connect(b, &QPushButton::clicked, this, [id, set, draw] {
                set(id);
                VelloraSound::play("click");
                (*draw)(false);
            });
            line->addWidget(b);
        }
        line->addStretch(1);
    };
    const QList<QRgb> cloth = { 0xFFC62828, 0xFF6E1423, 0xFFEF7C1E, 0xFFE0A526, 0xFFF2C230, 0xFF3E9B47, 0xFF8BD13B, 0xFF23B5B5,
                                0xFF5AB4F0, 0xFF2C5BD6, 0xFF7B3FC4, 0xFFEC6FA8, 0xFF1E1E24, 0xFF8A8F98, 0xFFECECEC, 0xFF6B4423 };
    SkinGen::Options& o = st.options;
    if (capes) {
        colorRow(tr("Colour"), cloth, &o.clothes);
        colorRow(tr("Second"), cloth, &o.second);
        textRow(tr("Pattern"),
                { { "gradient", tr("Gradient") }, { "stripes", tr("Stripes") }, { "checker", tr("Checker") }, { "diagonal", tr("Diagonal") },
                  { "border", tr("Border") }, { "galaxy", tr("Galaxy") }, { "flames", tr("Flames") }, { "ice", tr("Ice") }, { "leaves", tr("Leaves") },
                  { "rainbow", tr("Rainbow") } },
                o.pattern.value_or(QString()), [&o](const QString& id) { o.pattern = id.isNull() ? std::nullopt : std::optional<QString>(id); });
    } else {
        textRow(tr("Who"), { { "1", tr("Boy") }, { "2", tr("Girl") } }, o.gender == 0 ? QString() : QString::number(o.gender),
                [&o](const QString& id) { o.gender = id.isNull() ? 0 : id.toInt(); });
        colorRow(tr("Clothes"), cloth, &o.clothes);
        colorRow(tr("Hair"), { 0xFF1E1E24, 0xFF2B1B12, 0xFF6B4423, 0xFFD9632B, 0xFFE8CF7A, 0xFFE6E6E6, 0xFFEC6FA8, 0xFF5AB4F0, 0xFF3E9B47, 0xFF7B3FC4, 0xFFC62828 },
                 &o.hair);
        textRow(tr("Hair style"),
                { { "short", tr("Short") }, { "long", tr("Long") }, { "spiky", tr("Spiky") }, { "bun", tr("Bun") }, { "mohawk", tr("Mohawk") },
                  { "bald", tr("Bald") } },
                o.hairStyle.value_or(QString()), [&o](const QString& id) { o.hairStyle = id.isNull() ? std::nullopt : std::optional<QString>(id); });
        colorRow(tr("Eyes"), { 0xFF3B6FD6, 0xFF2E8B57, 0xFF6B4423, 0xFF1E1E24, 0xFF4FA3E0, 0xFFE53935, 0xFFFFD34A, 0xFFB04CFF }, &o.eyes);
        colorRow(tr("Skin"), { 0xFFFFDBB8, 0xFFF2C7A5, 0xFFE8B48C, 0xFFD39A6A, 0xFFB57A4A, 0xFF8D5524, 0xFFE9E4E0, 0xFF5E9E52, 0xFFB3262F, 0xFF5A8FD6 },
                 &o.skin);
    }
    {
        auto [line, group] = addRow(tr("Emblem"));
        QStringList ids = SkinGen::emblemIds();
        ids.prepend(QString(""));
        ids.prepend(QString());
        for (const QString& id : ids) {
            auto* chip = new ChipButton(custom);
            chip->autoChip = id.isNull();
            chip->none = !id.isNull() && id.isEmpty();
            if (!id.isEmpty()) {
                chip->rows = SkinGen::emblemRows(id);
            }
            chip->setChecked(id.isNull() ? !o.emblem.has_value() : (o.emblem.has_value() && *o.emblem == id));
            chip->setToolTip(id.isNull() ? tr("Auto: as the words say") : id.isEmpty() ? tr("No emblem") : id);
            group->addButton(chip);
            connect(chip, &QAbstractButton::clicked, this, [id, draw, &o] {
                o.emblem = id.isNull() ? std::nullopt : std::optional<QString>(id);
                VelloraSound::play("click");
                (*draw)(false);
            });
            line->addWidget(chip);
        }
        line->addStretch(1);
    }
    connect(reset, &QPushButton::clicked, this, [this, capes] {
        GenState& st = capes ? m_capeGen : m_skinGen;
        st.options = SkinGen::Options();
        VelloraSound::play("click");
        // the panel is made again (the buttons show "auto", the variants are repainted); not while its button is still clicking
        QTimer::singleShot(0, this, [this] { showTab(m_tab); });
    });

    // the recent descriptions first, then the ideas
    const QStringList skinIdeas = { tr("Knight in golden armour"), tr("Ninja with a red headband"), tr("Wizard in a purple robe"), tr("Pirate"),
                                    tr("Princess"),                tr("Astronaut"),                 tr("Demon"),                   tr("Cyber neon"),
                                    tr("Zombie"),                  tr("Skeleton"),                  tr("Elf"),                     tr("Viking") };
    const QStringList capeIdeas = { tr("Galaxy with stars"), tr("Fire"),       tr("Ice"),          tr("Gold with a crown"), tr("Pink with a heart"),
                                    tr("Rainbow"),           tr("Burgundy Vellora"), tr("Forest"), tr("Blue with lightning"), tr("Black and red stripes") };
    const QString key = capes ? "generatorRecentCapes" : "generatorRecentSkins";
    const QStringList recent = ui.value(key).toStringList();
    auto addIdea = [this, ideas, ideasHolder, prompt, draw](const QString& text, bool old) {
        auto* idea = new QPushButton(text, ideasHolder);
        if (old) {
            idea->setStyleSheet("font-style: italic;");
            idea->setToolTip(tr("You asked for it before"));
        }
        connect(idea, &QPushButton::clicked, this, [prompt, text, draw] {
            prompt->setText(text);
            VelloraSound::play("open");
            (*draw)(true);
        });
        ideas->addWidget(idea);
    };
    if (!recent.isEmpty()) {
        auto* label = new QLabel(tr("Recent:"), ideasHolder);
        label->setStyleSheet("color: #d1a3aa;");
        ideas->addWidget(label);
        for (const QString& r : recent) {
            addIdea(r, true);
        }
        auto* label2 = new QLabel(tr("Ideas:"), ideasHolder);
        label2->setStyleSheet("color: #d1a3aa;");
        ideas->addWidget(label2);
    }
    for (const QString& idea : capes ? capeIdeas : skinIdeas) {
        if (!recent.contains(idea)) {
            addIdea(idea, false);
        }
    }
    ideas->addStretch(1);

    // Enter or "Create": new variants, and the description is remembered
    auto now = [this, prompt, draw, key] {
        const QString text = prompt->text().trimmed();
        if (!text.isEmpty()) {
            QSettings settings(QDir(APPLICATION->dataRoot()).filePath("vellora-ui.ini"), QSettings::IniFormat);
            QStringList list = settings.value(key).toStringList();
            list.removeAll(text);
            list.prepend(text);
            settings.setValue(key, list.mid(0, 6));
        }
        VelloraSound::play("open");
        (*draw)(true);
    };
    connect(go, &QPushButton::clicked, this, now);
    connect(prompt, &QLineEdit::returnPressed, this, now);
    connect(dice, &QPushButton::clicked, this, [prompt, now, capes] {
        prompt->setText(SkinGen::surprise(capes));
        now();
    });
    // it draws while it is typed: a moment after the last letter
    auto* typing = new QTimer(panel);
    typing->setSingleShot(true);
    typing->setInterval(600);
    connect(typing, &QTimer::timeout, this, [prompt, draw] {
        if (prompt->text().trimmed().size() >= 3) {
            (*draw)(false);
        }
    });
    connect(prompt, &QLineEdit::textEdited, typing, qOverload<>(&QTimer::start));

    if (st.made) {
        (*draw)(false);  // back on the tab: what was made is still there
    } else {
        auto* hint = new QLabel(tr("Write who to draw, click an idea or the die: the variants appear here"), variantsHolder);
        hint->setStyleSheet("color: #d1a3aa;");
        variants->addWidget(hint);
    }
    return panel;
}
