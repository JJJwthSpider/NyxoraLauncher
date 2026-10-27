// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "InstanceDelegate.h"
#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QTextLayout>
#include <QTextOption>
#include <QtMath>

#include <QIcon>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextEdit>
#include "BaseInstance.h"
#include "InstanceList.h"
#include "InstanceView.h"
#include "FileSystem.h"
#include "ui/themes/SculkTheme.h"

// Origin: Qt
static void viewItemTextLayout(QTextLayout& textLayout, int lineWidth, qreal& height, qreal& widthUsed)
{
    height = 0;
    widthUsed = 0;
    textLayout.beginLayout();
    QString str = textLayout.text();
    while (true) {
        QTextLine line = textLayout.createLine();
        if (!line.isValid())
            break;
        if (line.textLength() == 0)
            break;
        line.setLineWidth(lineWidth);
        line.setPosition(QPointF(0, height));
        height += line.height();
        widthUsed = qMax(widthUsed, line.naturalTextWidth());
    }
    textLayout.endLayout();
}

ListViewDelegate::ListViewDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void drawSelectionRect(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect)
{
    if ((option.state & QStyle::State_Selected))
        painter->fillRect(rect, option.palette.brush(QPalette::Highlight));
    else {
        QColor backgroundColor = option.palette.color(QPalette::Window);
        backgroundColor.setAlpha(160);
        painter->fillRect(rect, QBrush(backgroundColor));
    }
}

void drawFocusRect(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect)
{
    if (!(option.state & QStyle::State_HasFocus))
        return;
    QStyleOptionFocusRect opt;
    opt.direction = option.direction;
    opt.fontMetrics = option.fontMetrics;
    opt.palette = option.palette;
    opt.rect = rect;
    // opt.state           = option.state | QStyle::State_KeyboardFocusChange |
    // QStyle::State_Item;
    auto col = option.state & QStyle::State_Selected ? QPalette::Highlight : QPalette::Base;
    opt.backgroundColor = option.palette.color(col);
    // Apparently some widget styles expect this hint to not be set
    painter->setRenderHint(QPainter::Antialiasing, false);

    QStyle* style = option.widget ? option.widget->style() : QApplication::style();

    style->drawPrimitive(QStyle::PE_FrameFocusRect, &opt, painter, option.widget);

    painter->setRenderHint(QPainter::Antialiasing);
}

// TODO this can be made a lot prettier
void drawProgressOverlay(QPainter* painter, const QStyleOptionViewItem& option, const int value, const int maximum)
{
    if (maximum == 0 || value == maximum) {
        return;
    }

    painter->save();

    qreal percent = (qreal)value / (qreal)maximum;
    QColor color = option.palette.color(QPalette::Dark);
    color.setAlphaF(0.70f);
    painter->setBrush(color);
    painter->setPen(QPen(QBrush(), 0));
    painter->drawPie(option.rect, 90 * 16, -percent * 360 * 16);

    painter->restore();
}

void drawBadges(QPainter* painter, const QStyleOptionViewItem& option, BaseInstance* instance, QIcon::Mode mode, QIcon::State state)
{
    QList<QString> pixmaps;
    if (instance->isRunning()) {
        pixmaps.append("status-running");
    } else if (instance->hasCrashed() || instance->hasVersionBroken()) {
        pixmaps.append("status-bad");
    }
    if (instance->hasUpdateAvailable()) {
        pixmaps.append("checkupdate");
    }

    static const int itemSide = 24;
    static const int spacing = 1;
    const int itemsPerRow = qMax(1, qFloor(double(option.rect.width() + spacing) / double(itemSide + spacing)));
    const int rows = qCeil((double)pixmaps.size() / (double)itemsPerRow);
    QListIterator<QString> it(pixmaps);
    painter->translate(option.rect.topLeft());
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < itemsPerRow; ++x) {
            if (!it.hasNext()) {
                return;
            }
            // FIXME: inject this.
            auto icon = QIcon::fromTheme(it.next());
            // opt.icon.paint(painter, iconbox, Qt::AlignCenter, mode, state);
            const QPixmap pixmap;
            // itemSide
            QRect badgeRect(option.rect.width() - x * itemSide + qMax(x - 1, 0) * spacing - itemSide,
                            y * itemSide + qMax(y - 1, 0) * spacing, itemSide, itemSide);
            icon.paint(painter, badgeRect, Qt::AlignCenter, mode, state);
        }
    }
    painter->translate(-option.rect.topLeft());
}

static QSize viewItemTextSize(const QStyleOptionViewItem* option)
{
    QStyle* style = option->widget ? option->widget->style() : QApplication::style();
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    QTextLayout textLayout;
    textLayout.setTextOption(textOption);
    textLayout.setFont(option->font);
    textLayout.setText(option->text);
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, option, option->widget) + 1;
    QRect bounds(0, 0, 100 - 2 * textMargin, 600);
    qreal height = 0, widthUsed = 0;
    viewItemTextLayout(textLayout, bounds.width(), height, widthUsed);
    const QSize size(qCeil(widthUsed), qCeil(height));
    return QSize(size.width() + 2 * textMargin, size.height());
}

// "1.21.1 · NeoForge" under the name of the instance. Read from mmc-pack.json and cached until the file changes:
// loading the pack profile from a paint event would start tasks while painting.
static QString instanceSubtitle(BaseInstance* instance)
{
    struct Entry {
        QDateTime modified;
        QString subtitle;
    };
    static QHash<QString, Entry> cache;

    const QFileInfo file(FS::PathCombine(instance->instanceRoot(), "mmc-pack.json"));
    const QDateTime modified = file.lastModified();
    if (auto it = cache.constFind(instance->id()); it != cache.constEnd() && it->modified == modified) {
        return it->subtitle;
    }

    QHash<QString, QString> versions;
    QFile pack(file.filePath());
    if (pack.open(QIODevice::ReadOnly)) {
        const auto components = QJsonDocument::fromJson(pack.readAll()).object().value("components").toArray();
        for (const auto& value : components) {
            const auto component = value.toObject();
            const QString version = component.value("version").toString(component.value("cachedVersion").toString());
            versions.insert(component.value("uid").toString(), version);
        }
    }

    QString subtitle = versions.value("net.minecraft");
    static const std::pair<const char*, const char*> loaders[] = {
        { "net.neoforged", "NeoForge" },
        { "net.minecraftforge", "Forge" },
        { "net.fabricmc.fabric-loader", "Fabric" },
        { "org.quiltmc.quilt-loader", "Quilt" },
    };
    for (const auto& [uid, name] : loaders) {
        if (!subtitle.isEmpty() && versions.contains(uid)) {
            subtitle += QString::fromUtf8(" \u00B7 ") + name;
            break;
        }
    }
    cache.insert(instance->id(), { modified, subtitle });
    return subtitle;
}

void ListViewDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    painter->save();
    painter->setClipRect(opt.rect);

    opt.features |= QStyleOptionViewItem::WrapText;
    opt.text = index.data().toString();
    opt.textElideMode = Qt::ElideRight;
    opt.displayAlignment = Qt::AlignTop | Qt::AlignHCenter;

    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();

    // const int iconSize =  style->pixelMetric(QStyle::PM_IconViewIconSize);
    const int iconSize = 48;
    QRect iconbox = opt.rect;
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, opt.widget) + 1;
    QRect textRect = opt.rect;
    QRect textHighlightRect = textRect;
    // clip the decoration on top, remove width padding
    textRect.adjust(textMargin, iconSize + textMargin + 5, -textMargin, 0);

    textHighlightRect.adjust(0, iconSize + 5, 0, 0);

    // the same delegate draws the icon picker; cards, versions and Play buttons belong to the instance list only
    const bool instanceCard = qobject_cast<const InstanceView*>(opt.widget) != nullptr;

    // on the textured sculk floor every instance gets its own card, so the name stays readable
    if (instanceCard && SculkTheme::isActive()) {
        QColor card = opt.palette.color(QPalette::Base);
        card.setAlphaF(0.82);
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(QPen(opt.palette.color(QPalette::Light), 1));
        painter->setBrush(card);
        painter->drawRoundedRect(QRectF(opt.rect).adjusted(1.5, 1.5, -1.5, -1.5), 8, 8);
    }

    // draw background
    {
        // FIXME: unused
        // QSize textSize = viewItemTextSize ( &opt );
        drawSelectionRect(painter, opt, textHighlightRect);
        /*
        QPalette::ColorGroup cg;
        QStyleOptionViewItem opt2(opt);

        if ((opt.widget && opt.widget->isEnabled()) || (opt.state & QStyle::State_Enabled))
        {
            if (!(opt.state & QStyle::State_Active))
                cg = QPalette::Inactive;
            else
                cg = QPalette::Normal;
        }
        else
        {
            cg = QPalette::Disabled;
        }
        */
        /*
        opt2.palette.setCurrentColorGroup(cg);

        // fill in background, if any


        if (opt.backgroundBrush.style() != Qt::NoBrush)
        {
            QPointF oldBO = painter->brushOrigin();
            painter->setBrushOrigin(opt.rect.topLeft());
            painter->fillRect(opt.rect, opt.backgroundBrush);
            painter->setBrushOrigin(oldBO);
        }

        drawSelectionRect(painter, opt2, textHighlightRect);
        */

        /*
        if (opt.showDecorationSelected)
        {
            drawSelectionRect(painter, opt2, opt.rect);
            drawFocusRect(painter, opt2, opt.rect);
            // painter->fillRect ( opt.rect, opt.palette.brush ( cg, QPalette::Highlight ) );
        }
        else
        {

            // if ( opt.state & QStyle::State_Selected )
            {
                // QRect textRect = subElementRect ( QStyle::SE_ItemViewItemText,  opt,
                // opt.widget );
                // painter->fillRect ( textHighlightRect, opt.palette.brush ( cg,
                // QPalette::Highlight ) );
                drawSelectionRect(painter, opt2, textHighlightRect);
                drawFocusRect(painter, opt2, textHighlightRect);
            }
        }
        */
    }

    // icon mode and state, also used for badges
    QIcon::Mode mode = QIcon::Normal;
    if (!(opt.state & QStyle::State_Enabled))
        mode = QIcon::Disabled;
    else if (opt.state & QStyle::State_Selected)
        mode = QIcon::Selected;
    QIcon::State state = opt.state & QStyle::State_Open ? QIcon::On : QIcon::Off;

    // draw the icon
    {
        iconbox.setHeight(iconSize);
        opt.icon.paint(painter, iconbox, Qt::AlignCenter, mode, state);
    }
    // hovering the icon shows that a click changes it: a frame around it and a pencil badge
    if (const auto* view = qobject_cast<const InstanceView*>(opt.widget); view && view->hoveredIconIndex() == index) {
        const QRect box = InstanceView::iconRect(opt.rect).adjusted(1, 1, -1, -1);
        const QColor accent = opt.palette.color(QPalette::Highlight);
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(QPen(accent.lighter(130), 1.5));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(QRectF(box), 8, 8);
        const QRectF badge(box.right() - 17, box.bottom() - 17, 20, 20);
        painter->setPen(QPen(opt.palette.color(QPalette::Base), 1.5));
        painter->setBrush(accent);
        painter->drawEllipse(badge);
        // a little pencil: the body from the bottom left to the top right, and its tip
        const QPointF c = badge.center();
        painter->setPen(QPen(opt.palette.color(QPalette::HighlightedText), 2.2, Qt::SolidLine, Qt::FlatCap));
        painter->drawLine(c + QPointF(-3.5, 3.5), c + QPointF(4, -4));
        painter->setPen(Qt::NoPen);
        painter->setBrush(opt.palette.color(QPalette::HighlightedText));
        painter->drawPolygon(QPolygonF() << c + QPointF(-5.5, 5.5) << c + QPointF(-4.6, 2.6) << c + QPointF(-2.6, 4.6));
    }
    // set the text colors
    QPalette::ColorGroup cg = opt.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(opt.state & QStyle::State_Active))
        cg = QPalette::Inactive;
    if (opt.state & QStyle::State_Selected) {
        painter->setPen(opt.palette.color(cg, QPalette::HighlightedText));
    } else {
        painter->setPen(opt.palette.color(cg, QPalette::Text));
    }

    // draw the text
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    textOption.setTextDirection(opt.direction);
    textOption.setAlignment(QStyle::visualAlignment(opt.direction, opt.displayAlignment));
    QTextLayout textLayout;
    textLayout.setTextOption(textOption);
    textLayout.setFont(opt.font);
    textLayout.setText(opt.text);

    qreal width, height;
    viewItemTextLayout(textLayout, textRect.width(), height, width);

    const int lineCount = textLayout.lineCount();

    const QRect layoutRect = QStyle::alignedRect(opt.direction, opt.displayAlignment, QSize(textRect.width(), int(height)), textRect);
    const QPointF position = layoutRect.topLeft();
    for (int i = 0; i < lineCount; ++i) {
        const QTextLine line = textLayout.lineAt(i);
        line.draw(painter, position);
    }

    // FIXME: this really has no business of being here. Make generic.
    auto instance = (BaseInstance*)index.data(InstanceList::InstancePointerRole).value<void*>();

    // the Minecraft version and the loader, in a smaller and dimmer font
    if (instance && instanceCard) {
        if (const QString subtitle = instanceSubtitle(instance); !subtitle.isEmpty()) {
            QFont small = opt.font;
            small.setPointSizeF(opt.font.pointSizeF() * 0.85);
            painter->setFont(small);
            painter->setPen(opt.palette.color(cg, opt.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::PlaceholderText));
            const QRect subtitleRect(textRect.left(), int(position.y() + height) + 1, textRect.width(), QFontMetrics(small).height());
            painter->drawText(subtitleRect, Qt::AlignHCenter | Qt::AlignTop,
                              QFontMetrics(small).elidedText(subtitle, Qt::ElideRight, subtitleRect.width()));
        }
    }
    if (instance) {
        drawBadges(painter, opt, instance, mode, state);
    }

    drawProgressOverlay(painter, opt, index.data(InstanceViewRoles::ProgressValueRole).toInt(),
                        index.data(InstanceViewRoles::ProgressMaximumRole).toInt());

    // the Play button at the bottom of the card
    if (instanceCard) {
        const auto* view = qobject_cast<const InstanceView*>(opt.widget);
        const bool hovered = view && view->hoveredPlayIndex() == index;
        const bool running = instance && instance->isRunning();
        const QRect button = InstanceView::playButtonRect(opt.rect);
        QColor fill = opt.palette.color(QPalette::Highlight);
        if (running) {
            fill = opt.palette.color(QPalette::Button);
        } else if (hovered) {
            fill = fill.lighter(125);
        }
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(QPen(running ? opt.palette.color(QPalette::Mid) : fill.lighter(140), 1));
        painter->setBrush(fill);
        painter->drawRoundedRect(QRectF(button).adjusted(0.5, 0.5, -0.5, -0.5), 6, 6);

        QFont font = opt.font;
        font.setBold(true);
        painter->setFont(font);
        painter->setPen(running ? opt.palette.color(QPalette::Text) : opt.palette.color(QPalette::HighlightedText));
        const QString label = running ? tr("Running") : QString::fromUtf8("\u25B6 ") + tr("Play");
        painter->drawText(button, Qt::AlignCenter, painter->fontMetrics().elidedText(label, Qt::ElideRight, button.width() - 6));
    }

    painter->restore();
}

QSize ListViewDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.features |= QStyleOptionViewItem::WrapText;
    opt.text = index.data().toString();
    opt.textElideMode = Qt::ElideRight;
    opt.displayAlignment = Qt::AlignTop | Qt::AlignHCenter;

    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &option, opt.widget) + 1;
    int height = 48 + textMargin * 2 + 5;  // TODO: turn constants into variables
    const bool instanceCard = qobject_cast<const InstanceView*>(option.widget) != nullptr;
    if (instanceCard) {
        height += 30 + 16;  // the Play button and the version line
    }
    QSize szz = viewItemTextSize(&opt);
    height += szz.height();
    // FIXME: maybe the icon items could scale and keep proportions?
    QSize sz(instanceCard ? 136 : 100, height);  // instance cards are wide enough for "1.21.1 · NeoForge"
    return sz;
}

class NoReturnTextEdit : public QTextEdit {
    Q_OBJECT
   public:
    explicit NoReturnTextEdit(QWidget* parent) : QTextEdit(parent)
    {
        setTextInteractionFlags(Qt::TextEditorInteraction);
        setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    }
    bool event(QEvent* event) override
    {
        auto eventType = event->type();
        if (eventType == QEvent::KeyPress || eventType == QEvent::KeyRelease) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            auto key = keyEvent->key();
            if ((key == Qt::Key_Return || key == Qt::Key_Enter) && eventType == QEvent::KeyPress) {
                emit editingDone();
                return true;
            }
            if (key == Qt::Key_Tab) {
                return true;
            }
        }
        return QTextEdit::event(event);
    }
   signals:
    void editingDone();
};

void ListViewDelegate::updateEditorGeometry(QWidget* editor,
                                            const QStyleOptionViewItem& option,
                                            [[maybe_unused]] const QModelIndex& index) const
{
    const int iconSize = 48;
    QRect textRect = option.rect;
    // QStyle *style = option.widget ? option.widget->style() : QApplication::style();
    textRect.adjust(0, iconSize + 5, 0, 0);
    editor->setGeometry(textRect);
}

void ListViewDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto text = index.data(Qt::EditRole).toString();
    QTextEdit* realEditor = qobject_cast<NoReturnTextEdit*>(editor);
    realEditor->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    realEditor->append(text);
    realEditor->selectAll();
    realEditor->document()->clearUndoRedoStacks();
}

void ListViewDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    QTextEdit* realEditor = qobject_cast<NoReturnTextEdit*>(editor);
    QString text = realEditor->toPlainText();
    text.replace(QChar('\n'), QChar(' '));
    text = text.trimmed();
    // Prevent instance names longer than 128 chars
    text.truncate(128);
    if (text.size() != 0) {
        emit textChanged(model->data(index).toString(), text);
        model->setData(index, text);
    }
}

QWidget* ListViewDelegate::createEditor(QWidget* parent,
                                        [[maybe_unused]] const QStyleOptionViewItem& option,
                                        [[maybe_unused]] const QModelIndex& index) const
{
    auto editor = new NoReturnTextEdit(parent);
    connect(editor, &NoReturnTextEdit::editingDone, this, &ListViewDelegate::editingDone);
    return editor;
}

void ListViewDelegate::editingDone()
{
    NoReturnTextEdit* editor = qobject_cast<NoReturnTextEdit*>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}

#include "InstanceDelegate.moc"
