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
#include "SearchTranslator.h"

#include <QRegularExpression>
#include <QSet>

namespace SearchTranslator {

namespace {

struct Entry {
    const char* stem;  // beginning of a Russian word, lower case
    const char* english;
};

// Ordered by nothing in particular; the longest matching stem always wins.
const Entry s_dictionary[] = {
    // monsters & creatures
    { "зомб", "zombie" },
    { "нежит", "undead" },
    { "скелет", "skeleton" },
    { "паук", "spider" },
    { "вампир", "vampire" },
    { "оборотн", "werewolf" },
    { "моб", "mobs" },
    { "монстр", "monsters" },
    { "существ", "creatures" },
    { "живот", "animals" },
    { "питом", "pets" },
    { "дракон", "dragons" },
    { "босс", "bosses" },
    { "пират", "pirates" },
    { "рыцар", "knights" },
    { "викинг", "vikings" },
    { "ведьм", "witch" },
    { "призрак", "ghosts" },
    { "демон", "demons" },
    { "пчел", "bees" },
    { "пчёл", "bees" },
    { "лошад", "horses" },
    { "рыб", "fishing" },
    { "рыбалк", "fishing" },
    // weapons & combat
    { "оруж", "weapons" },
    { "пушк", "guns" },
    { "пушек", "guns" },
    { "огнестрел", "guns" },
    { "ружь", "guns" },
    { "автомат", "guns" },
    { "винтовк", "rifle" },
    { "пистолет", "pistol" },
    { "меч", "swords" },
    { "лук", "bow" },
    { "арбалет", "crossbow" },
    { "брон", "armor" },
    { "доспех", "armor" },
    { "щит", "shield" },
    { "бой", "combat" },
    { "боев", "combat" },
    { "сражен", "combat" },
    { "пвп", "pvp" },
    // magic
    { "маги", "magic" },
    { "магич", "magic" },
    { "волшеб", "magic" },
    { "заклин", "spells" },
    { "зачар", "enchantments" },
    { "чар", "enchantments" },
    { "зель", "potions" },
    { "алхим", "alchemy" },
    { "фэнтез", "fantasy" },
    { "фентез", "fantasy" },
    // technology
    { "техн", "technology" },
    { "технолог", "technology" },
    { "механ", "mechanical" },
    { "автоматиз", "automation" },
    { "электр", "electricity" },
    { "энерг", "energy" },
    { "труб", "pipes" },
    { "завод", "factory" },
    { "индустр", "industrial" },
    { "космос", "space" },
    { "космич", "space" },
    { "ракет", "rockets" },
    { "планет", "planets" },
    // performance & graphics
    { "оптимиз", "optimization" },
    { "производит", "performance" },
    { "фпс", "fps" },
    { "лаг", "lag" },
    { "шейдер", "shaders" },
    { "граф", "graphics" },
    { "текстур", "textures" },
    { "освещ", "lighting" },
    { "свет", "lighting" },
    { "анимац", "animations" },
    { "физик", "physics" },
    // world
    { "приключ", "adventure" },
    { "подземел", "dungeons" },
    { "данж", "dungeons" },
    { "структур", "structures" },
    { "здани", "buildings" },
    { "деревн", "villages" },
    { "город", "city" },
    { "замк", "castles" },
    { "замок", "castles" },
    { "биом", "biomes" },
    { "мир", "world" },
    { "генерац", "generation" },
    { "ландшафт", "terrain" },
    { "пещер", "caves" },
    { "океан", "ocean" },
    { "мор", "ocean" },
    { "лес", "forest" },
    { "незер", "nether" },
    { "ад", "nether" },
    { "гор", "mountains" },
    { "руд", "ores" },
    { "шахт", "mining" },
    { "добыч", "mining" },
    { "погод", "weather" },
    { "сезон", "seasons" },
    // survival & atmosphere
    { "хоррор", "horror" },
    { "ужас", "horror" },
    { "страш", "horror" },
    { "выжив", "survival" },
    { "реализм", "realism" },
    { "реалист", "realistic" },
    { "хардкор", "hardcore" },
    { "апокал", "apocalypse" },
    { "постапок", "post-apocalyptic" },
    { "радиац", "radiation" },
    { "инфекц", "infection" },
    { "вирус", "infection" },
    { "средневек", "medieval" },
    { "кровь", "blood" },
    { "кровав", "blood" },
    // food & farming
    { "еда", "food" },
    { "еды", "food" },
    { "еду", "food" },
    { "кулинар", "cooking" },
    { "готовк", "cooking" },
    { "кухн", "cooking" },
    { "ферм", "farming" },
    { "урож", "crops" },
    { "растен", "plants" },
    // building & decoration
    { "декор", "decoration" },
    { "мебел", "furniture" },
    { "строит", "building" },
    { "постро", "building" },
    { "блок", "blocks" },
    { "дом", "house" },
    { "кроват", "bed" },
    { "крафт", "crafting" },
    { "верстак", "crafting" },
    { "оборон", "defense" },
    { "турел", "turrets" },
    { "ловуш", "traps" },
    { "лут", "loot" },
    { "сокровищ", "treasure" },
    // transport
    { "транспорт", "vehicles" },
    { "машин", "vehicles" },
    { "самолет", "planes" },
    { "самолёт", "planes" },
    { "поезд", "trains" },
    { "лодк", "boats" },
    { "корабл", "ships" },
    // utilities
    { "карт", "map" },
    { "миникарт", "minimap" },
    { "хранен", "storage" },
    { "сундук", "chests" },
    { "рюкзак", "backpacks" },
    { "инвентар", "inventory" },
    { "рецепт", "recipes" },
    { "интерфейс", "ui" },
    { "меню", "menu" },
    { "звук", "sounds" },
    { "музык", "music" },
    { "голос", "voice chat" },
    { "чат", "chat" },
    { "квест", "quests" },
    { "навык", "skills" },
    { "скилл", "skills" },
    { "уров", "levels" },
    { "рпг", "rpg" },
    { "инструмент", "tools" },
    { "кирк", "pickaxe" },
    { "скин", "skins" },
    { "косметик", "cosmetics" },
    { "эмоц", "emotes" },
    { "библиот", "library" },
    { "сервер", "server" },
    // loaders
    { "неофордж", "neoforge" },
    { "фордж", "forge" },
    { "фабрик", "fabric" },
    { "квилт", "quilt" },
};

// Words that carry no meaning for a search.
const QSet<QString>& fillerWords()
{
    static const QSet<QString> s_fillers = {
        "с",     "со",    "и",     "или",    "для",    "на",        "в",      "во",        "про",     "по",    "а",
        "мод",   "моды",  "модов", "модами", "модом",  "сборка",    "сборку", "сборки",    "сборкой", "хочу",  "нужен",
        "нужна", "нужны", "нужно", "много",  "лучшие", "лучший",    "самые",  "самый",     "какой",   "какие", "чтобы",
        "был",   "была",  "были",  "есть",   "мне",    "майнкрафт", "майн",   "minecraft", "mod",     "mods",  "with",
        "and",   "for",   "the",   "a",      "pack",   "modpack",   "сделай", "собери",    "найди",
        // chat words that are not part of the theme
        "добавь", "добавить", "добавьте", "поставь", "поставить", "закинь", "засунь", "вставь", "положи", "включи", "пожалуйста",
        "плиз", "пж", "должны", "должен", "должна", "должно", "быть", "ещё", "еще", "тоже", "также", "можно", "давай", "сделать", "собрать", "побольше", "больше", "поменьше",
        "меньше", "немного", "чуть", "кучу", "куча", "типа", "всякие", "всякое", "разные", "разных", "крутые", "крутых",
        "интересные", "новые", "версия", "версию", "версии", "лоадер", "загрузчик", "обратно", "пусть", "будет", "будут", "тогда",
        "вообще", "только", "очень", "вот", "это", "этот", "эти", "там", "тут", "чтоб", "хочется", "хотел", "хотела", "бы", "ли",
        "его", "её", "их", "мод", "модик", "модики", "модпак", "модпака", "сборке", "сборочку", "игра", "игру", "игры", "из",
        "от", "до", "к", "ко", "у", "о", "об", "же", "уже", "все", "всё", "весь", "всю", "вся", "свой", "свою", "нормальный",
        "нормальные", "хороший", "хорошие", "какую-нибудь", "какие-нибудь", "что-нибудь", "нибудь", "add", "please", "also",
        "some", "more", "lots", "plus", "want", "need", "i", "me", "my", "of", "to", "in", "on",
    };
    return s_fillers;
}

QString translateWord(const QString& word)
{
    const Entry* best = nullptr;
    qsizetype bestLength = 0;
    for (const auto& entry : s_dictionary) {
        const auto stem = QString::fromUtf8(entry.stem);
        // very short stems ("бой", "лук") must match nearly the whole word, otherwise they would catch unrelated words
        if (stem.size() <= 3 && word.size() > stem.size() + 2) {
            continue;
        }
        if (word.startsWith(stem) && stem.size() > bestLength) {
            best = &entry;
            bestLength = stem.size();
        }
    }
    return best ? QString::fromUtf8(best->english) : word;
}

}  // namespace

bool hasCyrillic(const QString& text)
{
    static const QRegularExpression s_cyrillic(QStringLiteral("[\\x{0400}-\\x{04FF}]"));
    return text.contains(s_cyrillic);
}

QStringList keywords(const QString& query)
{
    static const QRegularExpression s_separators(QStringLiteral("[^\\w\\.\\-]+"), QRegularExpression::UseUnicodePropertiesOption);
    QStringList result;
    QStringList unknown;
    const auto words = query.toLower().split(s_separators, Qt::SkipEmptyParts);
    for (const auto& word : words) {
        if (fillerWords().contains(word)) {
            continue;
        }
        const auto translated = hasCyrillic(word) ? translateWord(word) : word;
        if (hasCyrillic(translated)) {
            unknown << translated;
        } else if (!result.contains(translated)) {
            result << translated;
        }
    }
    // Russian words without a translation would only spoil an English search;
    // keep them only when nothing else is left (it may be the name of a Russian mod)
    return result.isEmpty() ? unknown : result;
}

QStringList unknownWords(const QString& text)
{
    static const QRegularExpression s_separators(QStringLiteral("[^\\w\\-]+"), QRegularExpression::UseUnicodePropertiesOption);
    QStringList result;
    for (const auto& word : text.toLower().split(s_separators, Qt::SkipEmptyParts)) {
        // short words are mostly prepositions and particles
        if (word.size() < 4 || !hasCyrillic(word) || fillerWords().contains(word) || result.contains(word)) {
            continue;
        }
        if (hasCyrillic(translateWord(word))) {
            result << word;
        }
    }
    return result;
}

QString toEnglish(const QString& query)
{
    const auto trimmed = query.trimmed();
    // leave project ids, URLs and plain English untouched
    if (trimmed.isEmpty() || trimmed.startsWith('#') || trimmed.contains("://") || !hasCyrillic(trimmed)) {
        return query;
    }
    return keywords(trimmed).join(' ');
}

}  // namespace SearchTranslator
