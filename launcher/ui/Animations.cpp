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
#include "Animations.h"

#include <QGraphicsOpacityEffect>
#include <QPointer>
#include <QPropertyAnimation>
#include <QVariantAnimation>
#include <QWidget>
#include <algorithm>

namespace Animations {

void fadeIn(QWidget* widget, int durationMs)
{
    if (!widget) {
        return;
    }
    // a widget can only have one graphics effect; never replace one that someone else set
    if (widget->graphicsEffect() && !widget->graphicsEffect()->property("sculkFade").toBool()) {
        return;
    }
    auto* effect = new QGraphicsOpacityEffect(widget);
    effect->setProperty("sculkFade", true);
    effect->setOpacity(0.0);
    widget->setGraphicsEffect(effect);

    auto* animation = new QPropertyAnimation(effect, "opacity", widget);
    animation->setDuration(durationMs);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    QPointer<QWidget> guard(widget);
    QObject::connect(animation, &QAbstractAnimation::finished, widget, [guard, effect] {
        // the effect renders the widget into a pixmap; drop it so the widget is drawn normally again
        if (guard && guard->graphicsEffect() == effect) {
            guard->setGraphicsEffect(nullptr);
        }
    });
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void growIn(QWidget* widget, int durationMs)
{
    if (!widget) {
        return;
    }
    const int target = widget->sizeHint().height();
    widget->setMaximumHeight(0);
    auto* animation = new QVariantAnimation(widget);
    animation->setDuration(durationMs);
    animation->setStartValue(0);
    animation->setEndValue(std::max(target, 1));
    animation->setEasingCurve(QEasingCurve::OutCubic);
    QObject::connect(animation, &QVariantAnimation::valueChanged, widget,
                     [widget](const QVariant& value) { widget->setMaximumHeight(value.toInt()); });
    QObject::connect(animation, &QAbstractAnimation::finished, widget, [widget] { widget->setMaximumHeight(QWIDGETSIZE_MAX); });
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    fadeIn(widget, durationMs);
}

}  // namespace Animations
