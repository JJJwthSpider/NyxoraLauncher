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

#include <QPixmap>

#include "FusionTheme.h"

/// @brief Modern themes of the launcher: the sculk variants in the colors of Minecraft's sculk blocks
/// (deep blue-black backgrounds with glowing cyan accents), and the lunar Lullaby theme: night black and
/// moon white, a starry sky behind everything and a see-through rainbow on selections and main buttons.
class SculkTheme : public FusionTheme {
   public:
    /// the same sculk hues at three brightness levels; Default is the lighter, everyday look
    enum class Variant { Default, Dark, Light, Lunar };

    explicit SculkTheme(Variant variant = Variant::Default);
    virtual ~SculkTheme() {}

    QString id() override;
    QString name() override;
    QString tooltip() override;
    bool hasStyleSheet() override;
    QString appStyleSheet() override;
    QPalette colorScheme() override;
    double fadeAmount() override;
    QColor fadeColor() override;
    LogColors logColorScheme() override;

    /// true while one of the sculk themes is the application theme (set by the ThemeManager)
    static bool isActive();
    static void setActive(bool active);
    /// true while the lunar Lullaby theme is the application theme: textures become a starry night sky
    static bool isLunar();
    static void setLunar(bool lunar);
    /// a seamless tile of night sky with stars of different sizes
    static QPixmap starTexture(const QColor& background, int size = 256);
    /// a seamless pixel texture of a sculk block in the given colors: a 32x32 grid of patches with glowing veins
    static QPixmap texture(const QColor& background, const QColor& glow, int pixelSize = 4);
    /// only the veins of the texture, on a transparent background: drawn on top to make them pulse
    static QPixmap veinTexture(const QColor& glow, int pixelSize = 4);
    /// the texture saved as a PNG file, for style sheets
    static QString textureFile(const QColor& background, const QColor& glow);

   private:
    struct Colors {
        QString text, disabled, window, border, surface, bar, hover, glow, accent, onAccent, panel, disabledIcon, deep, base, muted,
            itemHover, accentHover, chipText;
    };
    const Colors& colors() const;

    Variant m_variant;
};
