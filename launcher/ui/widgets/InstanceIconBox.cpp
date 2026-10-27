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
#include "InstanceIconBox.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "Application.h"
#include "BaseInstance.h"
#include "icons/IconList.h"
#include "ui/dialogs/IconPickerDialog.h"

InstanceIconBox::InstanceIconBox(BaseInstance* instance, QWidget* parent) : QGroupBox(tr("Icon"), parent), m_instance(instance)
{
    auto* layout = new QHBoxLayout(this);
    layout->setSpacing(14);

    m_preview = new QLabel(this);
    m_preview->setFixedSize(72, 72);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setToolTip(tr("Click to change the icon"));
    m_preview->setCursor(Qt::PointingHandCursor);
    m_preview->installEventFilter(this);
    layout->addWidget(m_preview);

    auto* column = new QVBoxLayout;
    column->setSpacing(6);
    auto* hint = new QLabel(tr("Pick one of the offered icons or use any picture from your computer."), this);
    hint->setWordWrap(true);
    column->addWidget(hint);

    auto* buttons = new QHBoxLayout;
    auto* offered = new QPushButton(tr("Choose from the offered..."), this);
    auto* own = new QPushButton(tr("Your own picture..."), this);
    own->setObjectName("accentButton");
    buttons->addWidget(offered);
    buttons->addWidget(own);
    buttons->addStretch(1);
    column->addLayout(buttons);
    layout->addLayout(column, 1);

    connect(offered, &QPushButton::clicked, this, &InstanceIconBox::pickOffered);
    connect(own, &QPushButton::clicked, this, &InstanceIconBox::pickOwn);
    // an own picture appears in the icon list a moment after it was copied, and other windows may change the icon too
    connect(APPLICATION->icons(), &IconList::iconUpdated, this, [this](const QString& key) {
        if (key == m_instance->iconKey()) {
            updatePreview();
        }
    });
    updatePreview();
}

bool InstanceIconBox::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_preview && event->type() == QEvent::MouseButtonRelease) {
        pickOffered();
        return true;
    }
    return QGroupBox::eventFilter(watched, event);
}

void InstanceIconBox::updatePreview()
{
    m_preview->setPixmap(APPLICATION->icons()->getIcon(m_instance->iconKey()).pixmap(64, 64));
}

void InstanceIconBox::pickOffered()
{
    IconPickerDialog dialog(this);
    if (dialog.execWithSelection(m_instance->iconKey()) == QDialog::Accepted) {
        m_instance->setIconKey(dialog.selectedIconKey);
        updatePreview();
    }
}

void InstanceIconBox::pickOwn()
{
    if (const auto key = IconPickerDialog::installOwnPicture(this); !key.isEmpty()) {
        m_instance->setIconKey(key);
        updatePreview();
    }
}
