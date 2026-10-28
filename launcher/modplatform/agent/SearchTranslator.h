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

#include <QString>
#include <QStringList>

/// Mod platforms (Modrinth, CurseForge) only understand English search terms.
/// This turns a Russian query such as "зомби и оружие" into "zombie weapons",
/// leaving English words, mod names and special queries (URLs, #ids) untouched.
namespace SearchTranslator {

/// Translates every Russian word that is known; unknown words are kept as they are.
QString toEnglish(const QString& query);

/// Same as toEnglish(), but returns the separate English keywords (filler words removed).
QStringList keywords(const QString& query);

/// Russian words of the text that the dictionary does not know (fillers left out).
QStringList unknownWords(const QString& text);

/// True if the text contains Cyrillic letters.
bool hasCyrillic(const QString& text);

}  // namespace SearchTranslator
