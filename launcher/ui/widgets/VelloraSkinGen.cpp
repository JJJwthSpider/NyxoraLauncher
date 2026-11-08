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
#include "VelloraSkinGen.h"

#include <QColor>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStringList>
#include <algorithm>
#include <cmath>
#include <functional>

namespace SkinGen {
namespace {
const std::pair<const char*, const char*> COLORS[] = {
    { "бордов", "6E1423" },
    { "burgundy", "6E1423" },
    { "maroon", "6E1423" },
    { "красн", "C62828" },
    { "red", "C62828" },
    { "алый", "E53935" },
    { "розов", "EC6FA8" },
    { "pink", "EC6FA8" },
    { "оранж", "EF7C1E" },
    { "orange", "EF7C1E" },
    { "рыж", "D9632B" },
    { "жёлт", "F2C230" },
    { "желт", "F2C230" },
    { "yellow", "F2C230" },
    { "золот", "E0A526" },
    { "gold", "E0A526" },
    { "зелён", "3E9B47" },
    { "зелен", "3E9B47" },
    { "green", "3E9B47" },
    { "изумруд", "1E8E5A" },
    { "салат", "8BD13B" },
    { "lime", "8BD13B" },
    { "бирюз", "23B5B5" },
    { "cyan", "23B5B5" },
    { "teal", "23B5B5" },
    { "голуб", "5AB4F0" },
    { "син", "2C5BD6" },
    { "blue", "2C5BD6" },
    { "фиолет", "7B3FC4" },
    { "purple", "7B3FC4" },
    { "violet", "7B3FC4" },
    { "сирен", "B48CE6" },
    { "чёрн", "1E1E24" },
    { "черн", "1E1E24" },
    { "black", "1E1E24" },
    { "тёмн", "26232B" },
    { "темн", "26232B" },
    { "dark", "26232B" },
    { "бел", "ECECEC" },
    { "white", "ECECEC" },
    { "серебр", "C0C6CE" },
    { "silver", "C0C6CE" },
    { "серый", "8A8F98" },
    { "серая", "8A8F98" },
    { "серые", "8A8F98" },
    { "серого", "8A8F98" },
    { "grey", "8A8F98" },
    { "gray", "8A8F98" },
    { "коричнев", "6B4423" },
    { "brown", "6B4423" },
    { "бежев", "D8C29D" },
    { "beige", "D8C29D" },
    { "неон", "39FF88" },
    { "neon", "39FF88" } };
const std::pair<const char*, const char*> EMBLEMS[] = {
    { "сердц", "heart" },
    { "сердеч", "heart" },
    { "heart", "heart" },
    { "любов", "heart" },
    { "звезд", "star" },
    { "звёзд", "star" },
    { "star", "star" },
    { "череп", "skull" },
    { "skull", "skull" },
    { "крипер", "creeper" },
    { "creeper", "creeper" },
    { "молни", "bolt" },
    { "lightning", "bolt" },
    { "flash", "bolt" },
    { "лун", "moon" },
    { "moon", "moon" },
    { "огон", "flame" },
    { "огн", "flame" },
    { "пламя", "flame" },
    { "fire", "flame" },
    { "алмаз", "diamond" },
    { "diamond", "diamond" },
    { "корон", "crown" },
    { "crown", "crown" },
    { "vellora", "v" },
    { "веллора", "v" } };
const std::pair<const char*, const char*> THEMES[] = {
    { "рыцар", "knight" },
    { "knight", "knight" },
    { "латы", "knight" },
    { "доспех", "knight" },
    { "броня", "knight" },
    { "armor", "knight" },
    { "паладин", "knight" },
    { "воин", "knight" },
    { "warrior", "knight" },
    { "ниндзя", "ninja" },
    { "ninja", "ninja" },
    { "шиноби", "ninja" },
    { "ассасин", "ninja" },
    { "assassin", "ninja" },
    { "маг", "wizard" },
    { "волшеб", "wizard" },
    { "wizard", "wizard" },
    { "mage", "wizard" },
    { "колдун", "wizard" },
    { "чародей", "wizard" },
    { "некромант", "wizard" },
    { "ведьм", "witch" },
    { "witch", "witch" },
    { "пират", "pirate" },
    { "pirate", "pirate" },
    { "капитан", "pirate" },
    { "зомби", "zombie" },
    { "zombie", "zombie" },
    { "скелет", "skeleton" },
    { "skeleton", "skeleton" },
    { "демон", "demon" },
    { "demon", "demon" },
    { "дьявол", "demon" },
    { "devil", "demon" },
    { "ангел", "angel" },
    { "angel", "angel" },
    { "кибер", "cyber" },
    { "cyber", "cyber" },
    { "робот", "cyber" },
    { "robot", "cyber" },
    { "андроид", "cyber" },
    { "космонавт", "astronaut" },
    { "астронавт", "astronaut" },
    { "astronaut", "astronaut" },
    { "космос", "astronaut" },
    { "принцесс", "princess" },
    { "princess", "princess" },
    { "королев", "princess" },
    { "queen", "princess" },
    { "фея", "princess" },
    { "fairy", "princess" },
    { "король", "king" },
    { "короля", "king" },
    { "king", "king" },
    { "принц", "king" },
    { "prince", "king" },
    { "царь", "king" },
    { "император", "king" },
    { "вампир", "vampire" },
    { "vampire", "vampire" },
    { "дракул", "vampire" },
    { "эльф", "elf" },
    { "elf", "elf" },
    { "лучник", "elf" },
    { "archer", "elf" },
    { "охотни", "elf" },
    { "викинг", "viking" },
    { "viking", "viking" },
    { "варвар", "viking" },
    { "barbarian", "viking" },
    { "солдат", "soldier" },
    { "soldier", "soldier" },
    { "военн", "soldier" },
    { "камуфляж", "soldier" },
    { "camo", "soldier" },
    { "спецназ", "soldier" },
    { "худи", "hoodie" },
    { "hoodie", "hoodie" },
    { "толстовк", "hoodie" },
    { "хакер", "hoodie" },
    { "hacker", "hoodie" },
    { "геймер", "hoodie" },
    { "gamer", "hoodie" },
    { "стример", "hoodie" },
    { "костюм", "suit" },
    { "suit", "suit" },
    { "бизнес", "suit" },
    { "агент", "suit" },
    { "agent", "suit" },
    { "мафи", "suit" },
    { "детектив", "suit" },
    { "повар", "chef" },
    { "chef", "chef" },
    { "доктор", "doctor" },
    { "врач", "doctor" },
    { "doctor", "doctor" },
    { "учён", "doctor" },
    { "scientist", "doctor" },
    { "футбол", "sport" },
    { "спорт", "sport" },
    { "football", "sport" },
    { "баскетбол", "sport" } };
const char* const GIRL[] = { "девушк", "девочк", "girl", "woman", "женщин", "леди", "lady", "she", "аниме", "anime", "кавай", "kawaii" };


QStringList words(const QString& prompt)
{
    return prompt.toLower().replace(QChar(0x0451), QChar(0x0435)).split(QRegularExpression("[^\\p{L}\\p{N}]+"), Qt::SkipEmptyParts);
}

QString norm(const char* s)
{
    return QString::fromUtf8(s).replace(QChar(0x0451), QChar(0x0435));
}

bool has(const QStringList& w, const QStringList& stems)
{
    for (const auto& word : w) {
        for (const auto& s : stems) {
            const QString stem = QString(s).replace(QChar(0x0451), QChar(0x0435));
            if (!stem.isEmpty() && word.startsWith(stem)) {
                return true;
            }
        }
    }
    return false;
}

QRgb colorOf(const QString& word)
{
    for (const auto& c : COLORS) {
        const QString stem = norm(c.first);
        if (word.startsWith(stem) && !(stem == norm("бел") && word.startsWith(norm("белк")))) {
            return 0xFF000000u | QString::fromLatin1(c.second).toUInt(nullptr, 16);
        }
    }
    return 0;
}

QRgb colorNear(const QStringList& w, const QStringList& nouns)
{
    for (int i = 0; i < w.size(); i++) {
        bool noun = false;
        for (const auto& n : nouns) {
            noun |= w[i].startsWith(n);
        }
        if (!noun) {
            continue;
        }
        for (int d = 1; d <= 2; d++) {
            if (i - d >= 0 && colorOf(w[i - d])) {
                return colorOf(w[i - d]);
            }
            if (i + d < w.size() && colorOf(w[i + d])) {
                return colorOf(w[i + d]);
            }
        }
    }
    return 0;
}

QList<QRgb> clothesColors(const QStringList& w)
{
    QList<QRgb> out;
    for (int i = 0; i < w.size(); i++) {
        const QRgb c = colorOf(w[i]);
        if (!c) {
            continue;
        }
        bool other = w[i].startsWith(norm("рыж"));
        for (int d = -2; d <= 2; d++) {
            if (d != 0 && i + d >= 0 && i + d < w.size()) {
                const QString& n = w[i + d];
                other |= n.startsWith(norm("волос")) || n.startsWith("hair") || n.startsWith(norm("глаз")) || n.startsWith("eye") || n.startsWith(norm("кож")) ||
                         n.startsWith("skin");
            }
        }
        if (!other) {
            out << c;
        }
    }
    return out;
}

QRgb shade(QRgb c, float f)
{
    auto ch = [f](int v) { return std::clamp(int(std::lround(v * f)), 0, 255); };
    return qRgb(ch(qRed(c)), ch(qGreen(c)), ch(qBlue(c)));
}

QRgb mix(QRgb a, QRgb b, float t)
{
    auto m = [t](int x, int y) { return int(std::lround(x * (1 - t) + y * t)); };
    return qRgb(m(qRed(a), qRed(b)), m(qGreen(a), qGreen(b)), m(qBlue(a), qBlue(b)));
}

QRgb hueShift(QRgb c, float degrees)
{
    QColor color(c);
    float h, s, v;
    color.getHsvF(&h, &s, &v);
    if (h < 0) {
        return c;
    }
    h = std::fmod(h + degrees / 360.f + 1.f, 1.f);
    return QColor::fromHsvF(h, s, v).rgb();
}

float luma(QRgb c)
{
    return qRed(c) * 0.3f + qGreen(c) * 0.59f + qBlue(c) * 0.11f;
}

QRgb contrast(QRgb top, QRgb accent)
{
    if (std::abs(luma(top) - luma(accent)) > 60) {
        return accent;
    }
    return luma(top) > 128 ? 0xFF1E1E24 : 0xFFF4F4F4;
}

const QRgb SKIN_TONES[] = { 0xFFF2C7A5, 0xFFE8B48C, 0xFFD39A6A, 0xFFB57A4A, 0xFF8D5524, 0xFFFFDBB8 };
const QRgb HAIR_COLORS[] = { 0xFF2B1B12, 0xFF4A2C1A, 0xFF6B4423, 0xFFB5834A, 0xFFE3C16F, 0xFF1E1E24, 0xFFA33A1E };
const QRgb HUE[] = { 0xFFC62828, 0xFF2C5BD6, 0xFF3E9B47, 0xFF7B3FC4, 0xFFEF7C1E, 0xFF23B5B5, 0xFF6E1423, 0xFF1E1E24, 0xFFEC6FA8, 0xFFE0A526 };
const char* const EMBLEM_CHOICES[] = { "heart", "star", "bolt", "moon", "flame", "diamond", "v", "" };
const char* const HAIR_STYLES[] = { "short", "spiky", "long", "short", "bun", "mohawk" };

enum Face { TOP = 0, BOTTOM, RIGHT, FRONT, LEFT, BACK };
using Painter = std::function<QRgb(int face, int x, int y, int w, int h)>;

struct Tex {
    QImage img;
    QRandomGenerator& rnd;
    Tex(int w, int h, QRandomGenerator& r) : img(w, h, QImage::Format_ARGB32), rnd(r) { img.fill(Qt::transparent); }
    void set(int x, int y, QRgb c)
    {
        if (x >= 0 && y >= 0 && x < img.width() && y < img.height()) {
            img.setPixel(x, y, c);
        }
    }
    void box(int u, int v, int bw, int bh, int bd, const Painter& p, float noise)
    {
        for (int face = 0; face < 6; face++) {
            int fx, fy, fw, fh;
            switch (face) {
                case TOP: fx = u + bd; fy = v; fw = bw; fh = bd; break;
                case BOTTOM: fx = u + bd + bw; fy = v; fw = bw; fh = bd; break;
                case RIGHT: fx = u; fy = v + bd; fw = bd; fh = bh; break;
                case FRONT: fx = u + bd; fy = v + bd; fw = bw; fh = bh; break;
                case LEFT: fx = u + bd + bw; fy = v + bd; fw = bd; fh = bh; break;
                default: fx = u + 2 * bd + bw; fy = v + bd; fw = bw; fh = bh; break;
            }
            for (int y = 0; y < fh; y++) {
                for (int x = 0; x < fw; x++) {
                    const QRgb c = p(face, x, y, fw, fh);
                    if (!c) {
                        continue;
                    }
                    float f = 1 + (float(rnd.generateDouble()) - 0.5f) * noise;
                    if (face == TOP) {
                        f *= 1.06f;
                    } else if (face == BOTTOM) {
                        f *= 0.72f;
                    } else if (face == RIGHT || face == LEFT) {
                        f *= 0.9f;
                    } else if (face == BACK) {
                        f *= 0.94f;
                    }
                    if (face != TOP && face != BOTTOM && fh > 4) {
                        f *= 1.04f - 0.1f * y / fh;
                    }
                    set(fx + x, fy + y, qAlpha(c) == 255 ? shade(c, f) : c);
                }
            }
        }
    }
};

struct Look {
    QString theme = "casual", title, hairStyle = "short", head, outfit = "tshirt", emblem;
    bool girl = false, beard = false, glowEyes = false, eyepatch = false, gloves = false, longSleeves = true, belt = false;
    QRgb skin = 0, hair = 0, eyes = 0, top = 0, accent = 0, pants = 0, shoes = 0, metal = 0xFFB9C0CA;
};

QStringList L(std::initializer_list<const char*> list)
{
    QStringList out;
    for (const char* s : list) {
        out << QString::fromUtf8(s);
    }
    return out;
}

template <size_t N>
QRgb pick(const QRgb (&a)[N], int i)
{
    return a[size_t(i) % N];
}

void applyOptions(Look& l, const Options& o, int variant);

Look look(const QString& prompt, QRandomGenerator& rnd, int variant, const Options& o)
{
    const QStringList w = words(prompt);
    Look l;
    QStringList girl;
    for (const char* g : GIRL) {
        girl << QString::fromUtf8(g);
    }
    l.girl = has(w, girl) || has(w, L({ "принцесс", "princess", "ведьм", "witch", "фея", "fairy", "королев", "queen" }));
    if (o.gender != 0) {
        l.girl = o.gender == 2;
    }
    for (const auto& t : THEMES) {
        if (has(w, { QString::fromUtf8(t.first) })) {
            l.theme = t.second;
            break;
        }
    }
    l.skin = SKIN_TONES[rnd.bounded(6)];
    l.hair = HAIR_COLORS[rnd.bounded(7)];
    const QRgb eyes[] = { 0xFF3B6FD6, 0xFF2E8B57, 0xFF6B4423, 0xFF1E1E24, 0xFF4FA3E0 };
    l.eyes = eyes[rnd.bounded(5)];
    l.hairStyle = l.girl ? (rnd.bounded(2) ? "long" : "bun") : HAIR_STYLES[rnd.bounded(6)];
    l.top = HUE[rnd.bounded(10)];
    l.accent = hueShift(l.top, 150 + rnd.bounded(60));
    const QRgb pants[] = { 0xFF2C3E73, 0xFF26232B, 0xFF4A4F5A, 0xFF5B3A26, 0xFF1E3A5F };
    l.pants = pants[rnd.bounded(5)];
    const QRgb shoes[] = { 0xFF1E1E24, 0xFFECECEC, 0xFF5B3A26, 0xFF8A3442 };
    l.shoes = shoes[rnd.bounded(4)];
    l.emblem = EMBLEM_CHOICES[(rnd.bounded(8) + variant) % 8];
    l.longSleeves = rnd.bounded(2);
    const char* outfits[] = { "tshirt", "hoodie", "jacket", "tshirt" };
    l.outfit = outfits[(variant + rnd.bounded(2)) % 4];

    const QString& th = l.theme;
    if (th == "knight") {
        l.outfit = "armor";
        l.metal = has(w, L({ "золот", "gold" })) ? 0xFFE0A526 : has(w, L({ "черн", "black", "темн", "dark" })) ? 0xFF3A3A44 : 0xFFB9C0CA;
        l.head = variant % 2 == 0 ? "helmet" : "";
        l.accent = 0xFFC62828;
        l.title = QString::fromUtf8("Рыцарь");
        l.emblem = variant % 3 == 0 ? "crown" : variant % 3 == 1 ? "" : "diamond";
    } else if (th == "ninja") {
        l.outfit = "ninja";
        l.top = 0xFF1E1E24;
        l.accent = 0xFFC62828;
        l.pants = 0xFF1E1E24;
        l.shoes = 0xFF26232B;
        l.head = "mask";
        l.emblem = "";
        l.title = QString::fromUtf8("Ниндзя");
    } else if (th == "wizard" || th == "witch") {
        l.outfit = "robe";
        const QRgb robes[] = { 0xFF4B2A8C, 0xFF2C3E9E, 0xFF6E1423, 0xFF1E5B4A };
        l.top = pick(robes, variant);
        l.accent = 0xFFE0A526;
        l.head = variant % 2 == 0 ? "hood" : "";
        l.beard = !l.girl && variant % 2 == 0;
        if (l.beard) {
            l.hair = 0xFFE6E6E6;
        }
        l.emblem = variant % 2 == 0 ? "star" : "moon";
        l.title = th == "witch" ? QString::fromUtf8("Ведьма") : QString::fromUtf8("Маг");
    } else if (th == "pirate") {
        l.outfit = "pirate";
        const QRgb coats[] = { 0xFF4A2C1A, 0xFF26232B, 0xFF6E1423 };
        l.top = pick(coats, variant);
        l.accent = 0xFFE0A526;
        l.head = "bandana";
        l.eyepatch = variant % 2 == 0;
        l.beard = !l.girl && variant % 3 != 1;
        l.pants = 0xFF3B2A1E;
        l.shoes = 0xFF1E1E24;
        l.emblem = "";
        l.title = QString::fromUtf8("Пират");
    } else if (th == "zombie") {
        l.outfit = "zombie";
        l.skin = 0xFF5E9E52;
        l.top = 0xFF2CA6A4;
        l.pants = 0xFF4B3A8C;
        l.eyes = 0xFF1E1E24;
        l.hair = 0xFF2E4A2A;
        l.hairStyle = "short";
        l.emblem = "";
        l.title = QString::fromUtf8("Зомби");
    } else if (th == "skeleton") {
        l.outfit = "skeleton";
        l.skin = 0xFFE8E4D8;
        l.top = 0xFF1A1A1F;
        l.emblem = "";
        l.hairStyle = "bald";
        l.title = QString::fromUtf8("Скелет");
    } else if (th == "demon") {
        l.outfit = variant % 2 == 0 ? "suit" : "robe";
        l.skin = 0xFFB3262F;
        l.top = 0xFF1E1E24;
        l.accent = 0xFFFF6B1E;
        l.hair = 0xFF1E1E24;
        l.eyes = 0xFFFFD34A;
        l.glowEyes = true;
        l.head = "horns";
        l.emblem = "flame";
        l.title = QString::fromUtf8("Демон");
    } else if (th == "angel") {
        l.outfit = "robe";
        l.top = 0xFFF1EEE6;
        l.accent = 0xFFE0A526;
        l.hair = 0xFFF0D68A;
        l.eyes = 0xFF4FA3E0;
        l.head = "halo";
        l.emblem = "";
        l.title = QString::fromUtf8("Ангел");
    } else if (th == "cyber") {
        l.outfit = "suit";
        l.top = 0xFF16161D;
        const QRgb neon[] = { 0xFF23E0E0, 0xFFFF3D9A, 0xFF39FF88, 0xFFB04CFF };
        l.accent = pick(neon, variant);
        l.pants = 0xFF16161D;
        l.shoes = 0xFF26232B;
        l.head = "visor";
        l.glowEyes = true;
        l.eyes = l.accent;
        l.emblem = "bolt";
        l.title = QString::fromUtf8("Кибер");
    } else if (th == "astronaut") {
        l.outfit = "space";
        l.top = 0xFFECECEC;
        const QRgb acc[] = { 0xFFEF7C1E, 0xFF2C5BD6, 0xFFC62828 };
        l.accent = pick(acc, variant);
        l.pants = 0xFFDADADA;
        l.shoes = 0xFF8A8F98;
        l.head = variant % 2 == 0 ? "space" : "";
        l.emblem = "";
        l.title = QString::fromUtf8("Космонавт");
    } else if (th == "king") {
        l.outfit = "robe";
        l.top = 0xFF8E1C2C;
        l.accent = 0xFFE0A526;
        l.head = "crown";
        l.beard = !l.girl && variant % 2 == 0;
        l.emblem = "crown";
        l.title = QString::fromUtf8("Король");
    } else if (th == "princess") {
        l.outfit = "dress";
        const QRgb dresses[] = { 0xFFEC6FA8, 0xFFB48CE6, 0xFF5AB4F0, 0xFFF1EEE6 };
        l.top = pick(dresses, variant);
        l.accent = 0xFFE0A526;
        l.head = "tiara";
        l.hairStyle = "long";
        l.girl = true;
        l.emblem = "heart";
        l.title = QString::fromUtf8("Принцесса");
    } else if (th == "vampire") {
        l.outfit = "suit";
        l.skin = 0xFFE9E4E0;
        l.top = 0xFF1E1E24;
        l.accent = 0xFFB3121F;
        l.hair = 0xFF1E1E24;
        l.eyes = 0xFFE53935;
        l.glowEyes = true;
        l.emblem = "";
        l.title = QString::fromUtf8("Вампир");
    } else if (th == "elf") {
        l.outfit = "jacket";
        l.top = 0xFF3E7B3A;
        l.accent = 0xFF6B4423;
        l.hair = 0xFFE3C16F;
        l.hairStyle = "long";
        l.pants = 0xFF5B3A26;
        l.shoes = 0xFF4A2C1A;
        l.belt = true;
        l.head = variant % 2 == 0 ? "headband" : "";
        l.emblem = "";
        l.title = QString::fromUtf8("Эльф");
    } else if (th == "viking") {
        l.outfit = "fur";
        l.top = 0xFF5B3A26;
        l.accent = 0xFF9A8F80;
        l.beard = !l.girl;
        const QRgb hairs[] = { 0xFFA33A1E, 0xFFE3C16F, 0xFF6B4423 };
        l.hair = pick(hairs, variant);
        l.head = variant % 2 == 0 ? "viking" : "";
        l.pants = 0xFF4A3B2E;
        l.shoes = 0xFF3B2A1E;
        l.emblem = "";
        l.title = QString::fromUtf8("Викинг");
    } else if (th == "soldier") {
        l.outfit = "camo";
        l.top = 0xFF4B5D3A;
        l.pants = 0xFF4B5D3A;
        l.shoes = 0xFF1E1E24;
        l.head = variant % 2 == 0 ? "cap" : "";
        l.belt = true;
        l.emblem = "";
        l.title = QString::fromUtf8("Солдат");
    } else if (th == "hoodie") {
        l.outfit = "hoodie";
        if (has(w, L({ "хакер", "hacker" }))) {
            l.top = 0xFF1E1E24;
        }
        l.title = QString::fromUtf8("Худи");
    } else if (th == "suit") {
        l.outfit = "suit";
        l.top = 0xFF1E1E24;
        const QRgb ties[] = { 0xFFC62828, 0xFF2C5BD6, 0xFF6E1423 };
        l.accent = pick(ties, variant);
        l.pants = 0xFF1E1E24;
        l.shoes = 0xFF16161D;
        l.emblem = "";
        l.title = QString::fromUtf8("Агент");
    } else if (th == "chef" || th == "doctor") {
        l.outfit = "coat";
        l.top = 0xFFF1EEE6;
        l.accent = th == "chef" ? 0xFFC62828 : 0xFF5AB4F0;
        l.pants = 0xFF26232B;
        l.emblem = "";
        l.title = th == "chef" ? QString::fromUtf8("Повар") : QString::fromUtf8("Доктор");
    } else if (th == "sport") {
        l.outfit = "jersey";
        l.emblem = "";
        l.title = QString::fromUtf8("Спортсмен");
    } else {
        l.title = l.girl ? QString::fromUtf8("Девушка") : QString::fromUtf8("Игрок");
    }

    QList<QRgb> clothes = clothesColors(w);
    if (!clothes.isEmpty() && (th == "cyber" || th == "vampire" || th == "demon")) {
        l.accent = clothes.first();
        if (th == "cyber") {
            l.eyes = l.accent;
        }
        clothes.clear();
    }
    if (!clothes.isEmpty()) {
        const QRgb c = clothes.first();
        if (l.outfit == "armor") {
            l.metal = c;
        } else {
            l.top = variant == 0 ? c : mix(c, hueShift(c, variant * 12.f - 18), 0.35f);
        }
        if (clothes.size() > 1) {
            l.accent = clothes[1];
            if (l.outfit != "suit" && l.outfit != "ninja") {
                l.pants = shade(clothes[1], 0.75f);
            }
        }
        if (th == "ninja" && clothes.size() == 1) {
            l.accent = c;
            l.top = 0xFF1E1E24;
        }
    }
    QRgb hair = colorNear(w, L({ "волос", "hair", "прическ" }));
    if (has(w, L({ "рыж", "ginger", "redhead" }))) {
        hair = 0xFFD9632B;
    } else if (has(w, L({ "блонд", "blond" }))) {
        hair = 0xFFE8CF7A;
    } else if (has(w, L({ "брюнет" }))) {
        hair = 0xFF2B1B12;
    }
    if (hair) {
        l.hair = hair;
    }
    if (const QRgb e = colorNear(w, L({ "глаз", "eye" }))) {
        l.eyes = e;
    }
    if (const QRgb s = colorNear(w, L({ "кож", "skin" }))) {
        l.skin = s;
    }
    for (const auto& e : EMBLEMS) {
        if (has(w, { QString::fromUtf8(e.first) })) {
            l.emblem = e.second;
            break;
        }
    }
    if (has(w, L({ "длинн", "long" }))) {
        l.hairStyle = "long";
    } else if (has(w, L({ "лыс", "bald" }))) {
        l.hairStyle = "bald";
    } else if (has(w, L({ "ирокез", "mohawk" }))) {
        l.hairStyle = "mohawk";
    }
    if (has(w, L({ "бород", "beard" }))) {
        l.beard = true;
    }
    if (has(w, L({ "капюшон", "hood" }))) {
        l.head = "hood";
    }
    if (has(w, L({ "шлем", "helmet" }))) {
        l.head = "helmet";
    }
    if (has(w, L({ "повязк", "headband" }))) {
        l.head = th == "ninja" ? "mask" : "headband";
    }
    if (has(w, L({ "корон", "crown" }))) {
        l.head = "crown";
    }
    if (has(w, L({ "рог", "horn" }))) {
        l.head = "horns";
    }
    if (has(w, L({ "светящ", "glow" }))) {
        l.glowEyes = true;
    }
    applyOptions(l, o, variant);
    return l;
}

/// the buttons win over the words
void applyOptions(Look& l, const Options& o, int variant)
{
    if (o.gender == 2 && l.theme != "princess" && (l.hairStyle == "bald" || l.hairStyle == "mohawk" || l.hairStyle == "spiky")) {
        l.hairStyle = variant % 2 == 0 ? "long" : "bun";
    }
    if (o.gender == 1) {
        if (l.theme == "princess") {
            l.outfit = "robe";
            l.head = "crown";
            l.title = QString::fromUtf8("Принц");
        }
        if (l.title == QString::fromUtf8("Девушка")) {
            l.title = QString::fromUtf8("Игрок");
        }
    } else if (o.gender == 2 && l.title == QString::fromUtf8("Игрок")) {
        l.title = QString::fromUtf8("Девушка");
    }
    if (o.clothes) {
        const QRgb c = o.clothes;
        if (l.outfit == "armor") {
            l.metal = c;
        } else if (l.theme == "cyber" || l.theme == "vampire" || l.theme == "demon" || l.theme == "ninja") {
            l.accent = c;  // dark looks keep their dark clothes; the colour lights them up
            if (l.theme == "cyber") {
                l.eyes = c;
            }
        } else {
            l.top = variant == 0 ? c : mix(c, hueShift(c, variant * 12.f - 18), 0.35f);
        }
    }
    if (o.hair) {
        l.hair = o.hair;
    }
    if (o.eyes) {
        l.eyes = o.eyes;
    }
    if (o.skin) {
        l.skin = o.skin;
    }
    if (o.emblem) {
        l.emblem = *o.emblem;
    }
    if (o.hairStyle) {
        l.hairStyle = *o.hairStyle;
    }
}
}  // namespace

QStringList emblemIds()
{
    return { "heart", "star", "bolt", "moon", "flame", "diamond", "crown", "skull", "creeper", "v" };
}

QStringList emblemRows(const QString& id)
{
    if (id == "heart") return { ".#.#.", "#####", "#####", ".###.", "..#.." };
    if (id == "star") return { "..#..", ".###.", "#####", ".###.", ".#.#." };
    if (id == "skull") return { ".###.", "#####", "#.#.#", "#####", ".#.#." };
    if (id == "creeper") return { "##.##", "##.##", "..#..", ".###.", ".#.#." };
    if (id == "bolt") return { "...##", "..##.", ".####", "..##.", ".##.." };
    if (id == "moon") return { ".###.", "##...", "##...", "##...", ".###." };
    if (id == "flame") return { "..#..", ".##..", ".###.", "#####", ".###." };
    if (id == "diamond") return { ".###.", "#####", ".###.", "..#..", "....." };
    if (id == "crown") return { "#.#.#", "#####", "#####", ".....", "....." };
    if (id == "v") return { "#...#", "#...#", ".#.#.", ".#.#.", "..#.." };
    return {};
}

namespace {

void paintHead(Tex& t, const Look& l)
{
    const bool skull = l.outfit == "skeleton";
    const QRgb skin = l.skin, hair = l.hair;
    const QString style = l.hairStyle;
    t.box(0, 0, 8, 8, 8, [&](int face, int x, int y, int, int) -> QRgb {
        const bool bald = style == "bald" || skull;
        const bool longHair = style == "long" || style == "bun";
        if (face == TOP) {
            if (bald) return skin;
            if (style == "mohawk") return x == 3 || x == 4 ? hair : skin;
            return (x + y) % 5 == 0 ? shade(hair, 1.15f) : hair;
        }
        if (face == BOTTOM) return shade(skin, 0.8f);
        if (face == BACK) {
            if (bald || (style == "mohawk" && x != 3 && x != 4)) return skin;
            return y <= (longHair ? 7 : 5) ? ((x * 3 + y) % 7 == 0 ? shade(hair, 0.85f) : hair) : skin;
        }
        if (face == RIGHT || face == LEFT) {
            if (bald || style == "mohawk") return skin;
            const int nearFace = face == RIGHT ? x : 7 - x;
            if (y <= 1 || (longHair && nearFace <= 5) || (y <= 3 && nearFace <= 4) || (nearFace <= 1 && y <= 6)) return hair;
            return skin;
        }
        if (skull) {
            if (y >= 3 && y <= 4 && (x == 1 || x == 2 || x == 5 || x == 6)) return 0xFF1A1A1F;
            if (y == 5 && (x == 3 || x == 4)) return 0xFF3A3A40;
            if (y == 6 && x >= 2 && x <= 5) return x % 2 == 0 ? 0xFF1A1A1F : 0xFFD8D4C8;
            return skin;
        }
        if (style != "bald" && style != "mohawk") {
            if (y == 0) return hair;
            if (y == 1 && ((style == "long" || style == "spiky") ? ((x != 3 && x != 4) || (style == "spiky" && x % 2 == 0)) : (x <= 1 || x >= 6))) return hair;
            if (y >= 2 && y <= 5 && style == "long" && (x == 0 || x == 7)) return hair;
        }
        if (y == 3 && (x == 1 || x == 2 || x == 5 || x == 6)) return shade(hair, 0.8f);
        if (y == 4) {
            if (l.eyepatch && (x == 1 || x == 2)) return 0xFF16161D;
            if (x == 1 || x == 6) return l.glowEyes ? shade(l.eyes, 1.25f) : 0xFFF4F4F4;
            if (x == 2 || x == 5) return l.eyes;
        }
        if (l.girl && y == 5 && (x == 1 || x == 6)) return mix(skin, 0xFFE57A8A, 0.45f);
        if (y == 5 && (x == 3 || x == 4)) return shade(skin, 0.9f);
        if (l.beard && y >= 5) {
            if (y == 6 && (x == 3 || x == 4)) return shade(hair, 0.6f);
            return (x >= 1 && x <= 6) || y == 7 ? hair : skin;
        }
        if (y == 6 && (x == 3 || x == 4)) return l.girl ? 0xFFC75B6E : shade(skin, 0.72f);
        return skin;
    }, 0.05f);

    const QString head = l.head;
    t.box(32, 0, 8, 8, 8, [&](int face, int x, int y, int, int) -> QRgb {
        if (head == "hood") {
            if (face == FRONT) return (y == 0 || x == 0 || x == 7 || (y == 1 && (x <= 1 || x >= 6))) ? shade(l.top, 0.8f) : 0;
            return face == BOTTOM ? 0 : l.top;
        }
        if (head == "helmet") {
            if (face == BOTTOM) return 0;
            if (face == FRONT) {
                if (y == 3 && x >= 1 && x <= 6) return 0xFF101014;
                if ((y == 5 && (x == 2 || x == 5)) || (y == 6 && (x == 3 || x == 4))) return shade(l.metal, 0.5f);
            }
            if (face == TOP && (x == 3 || x == 4)) return l.accent;
            return (x + y) % 4 == 0 ? shade(l.metal, 1.15f) : (y == 0 && face != TOP) ? shade(l.metal, 1.2f) : l.metal;
        }
        if (head == "bandana") {
            if (face == TOP) return 0xFFC62828;
            if (face == BOTTOM) return 0;
            if (y <= 1) return (x + y) % 3 == 0 ? 0xFFF4F4F4 : 0xFFC62828;
            if (face == BACK && y <= 4 && (x == 3 || x == 4)) return 0xFFA81F1F;
            return 0;
        }
        if (head == "crown") {
            if (face == TOP || face == BOTTOM) return 0;
            if (y == 0) return x % 2 == 0 ? 0xFFF2C230 : 0;
            if (y == 1) {
                if (face == FRONT && (x == 2 || x == 5)) return x == 2 ? 0xFFE53935 : 0xFF3B6FD6;
                return 0xFFE0A526;
            }
            return (l.hairStyle == "long" && face == BACK && y >= 2) ? l.hair : 0;
        }
        if (head == "mask") {
            if (face == TOP || face == BOTTOM) return 0;
            if (y == 2) return l.accent;
            if (face == BACK && y >= 3 && y <= 5 && (x == 3 || x == 4)) return shade(l.accent, 0.8f);
            if (y >= 5) return (x + y) % 3 == 0 ? shade(l.top, 1.3f) : shade(l.top, 1.1f);
            return 0;
        }
        if (head == "tiara") {
            if (face == FRONT && y == 1 && x >= 2 && x <= 5) return 0xFFF2C230;
            if (face == FRONT && y == 0 && (x == 3 || x == 4)) return x == 3 ? 0xFFEC6FA8 : 0xFFF2C230;
        } else if (head == "cap") {
            if (face == TOP) return l.top;
            if (face != BOTTOM && y <= 1) return (face == FRONT && y == 1) ? shade(l.top, 0.7f) : l.top;
            return 0;
        } else if (head == "visor") {
            if ((face == FRONT || face == LEFT || face == RIGHT) && y == 4) return x % 3 == 0 ? shade(l.accent, 1.2f) : l.accent;
        } else if (head == "viking") {
            if (face == BOTTOM) return 0;
            if (face == TOP || y <= 2) return (x + y) % 3 == 0 ? shade(0xFF9AA3AE, 1.15f) : 0xFF9AA3AE;
            if (face == FRONT && (x == 3 || x == 4) && y <= 4) return 0xFF7D858F;
            if (((face == RIGHT && x >= 5) || (face == LEFT && x <= 2)) && y == 3) return 0xFFEDE3C8;
            return 0;
        } else if (head == "space") {
            if (face == BOTTOM) return 0;
            if (face == FRONT && y >= 2 && y <= 6 && x >= 1 && x <= 6) return (x == 2 && y == 3) ? 0xFFBFE6FF : (y == 2 || x == 1) ? 0xFF2A3A6A : 0xFF16203F;
            return (x + y) % 5 == 0 ? 0xFFDADADA : 0xFFF4F4F4;
        } else if (head == "headband") {
            if (face != TOP && face != BOTTOM && y == 2) return l.accent;
        } else if (head == "horns") {
            if (face == FRONT && y == 0 && (x == 0 || x == 7)) return 0xFF2B1B12;
            if (((face == RIGHT && x == 7) || (face == LEFT && x == 0)) && y <= 1) return 0xFF2B1B12;
        } else if (head == "halo") {
            if (face == TOP && (x == 0 || x == 7 || y == 0 || y == 7)) return 0xFFF2D46A;
        }
        if (l.hairStyle == "long" && (face == BACK || face == RIGHT || face == LEFT) && y >= 3 && head != "hood" && head != "helmet" && head != "space") {
            const int nearFace = face == RIGHT ? x : face == LEFT ? 7 - x : 0;
            return nearFace <= 5 ? ((x + y) % 4 == 0 ? shade(l.hair, 0.85f) : l.hair) : 0;
        }
        if (l.hairStyle == "spiky" && face == FRONT && y == 0 && x % 2 == 1 && head.isEmpty()) return shade(l.hair, 1.1f);
        if (l.hairStyle == "bun" && face == BACK && y >= 1 && y <= 3 && x >= 2 && x <= 5 && head.isEmpty()) return shade(l.hair, 1.1f);
        return 0;
    }, 0.04f);
}

QRgb cloth(const Look& l, int face, int x, int y, int, int h, bool torso)
{
    const QRgb top = l.top, acc = l.accent;
    const QString& o = l.outfit;
    if (o == "armor") {
        const QRgb m = l.metal;
        if (torso && face == FRONT && (x == 3 || x == 4) && y <= 9) return shade(m, 1.18f);
        if (torso && y == 10) return face == FRONT && (x == 3 || x == 4) ? 0xFFE0A526 : 0xFF4A2C1A;
        if ((x + y * 2) % 7 == 0) return shade(m, 1.25f);
        return y == 0 ? shade(m, 1.12f) : m;
    }
    if (o == "ninja") {
        if (torso && y == 7) return acc;
        return (x + y) % 4 == 0 ? shade(top, 1.35f) : top;
    }
    if (o == "robe") {
        if (torso && face == FRONT && (x == 3 || x == 4)) return acc;
        if (torso && (y == 6 || y == 7)) return shade(acc, 0.85f);
        if ((x * 3 + y * 5) % 17 == 0) return shade(acc, 1.1f);
        return top;
    }
    if (o == "suit") {
        if (torso && face == FRONT) {
            if ((x == 3 || x == 4) && y <= 8) return y == 0 ? 0xFFF4F4F4 : acc;
            if ((x == 2 || x == 5) && y <= 3) return 0xFFF4F4F4;
            if (x == 2 && y >= 5 && y % 3 == 2) return shade(top, 1.6f);
        }
        return top;
    }
    if (o == "pirate") {
        if (torso && face == FRONT && x >= 2 && x <= 5 && y <= 6) return (x == 2 || x == 5) ? shade(top, 1.2f) : 0xFFF1EEE6;
        if (torso && (y == 8 || y == 9)) return 0xFFC62828;
        if (torso && face == FRONT && (x == 1 || x == 6) && y % 3 == 1) return 0xFFE0A526;
        return top;
    }
    if (o == "space") {
        const QRgb panel[] = { 0xFFE53935, 0xFF39FF88, 0xFF4FA3E0, 0xFFF2C230 };
        if (torso && face == FRONT && y >= 3 && y <= 5 && x >= 2 && x <= 5) return panel[(x + y) % 4];
        if (y == 1 || (torso && y == 9)) return acc;
        return (x + y) % 6 == 0 ? shade(top, 0.93f) : top;
    }
    if (o == "skeleton") {
        if (torso) {
            if (face == FRONT && (x == 3 || x == 4)) return l.skin;
            if (face == FRONT && y >= 1 && y <= 8 && y % 2 == 1 && x >= 1 && x <= 6) return l.skin;
            if (face == BACK && (x == 3 || x == 4)) return l.skin;
        }
        return top;
    }
    if (o == "zombie") {
        if ((x * 7 + y * 3) % 11 == 0) return l.skin;
        if (y == h - 1 && x % 2 == 0) return l.skin;
        return top;
    }
    if (o == "fur") {
        if (torso && face == FRONT && x >= 2 && x <= 5) return 0xFF3A2E26;
        if (torso && y == 9) return face == FRONT && (x == 3 || x == 4) ? 0xFFC0C6CE : 0xFF2B1B12;
        return (x * 5 + y * 3) % 4 == 0 ? shade(acc, 1.15f) : (x + y) % 3 == 0 ? shade(acc, 0.85f) : acc;
    }
    if (o == "camo") {
        const QRgb camo[] = { top, shade(top, 0.75f), 0xFF6B5A3A, shade(top, 1.2f) };
        return camo[((x / 2) * 3 + (y / 2) * 5 + x * y) % 4];
    }
    if (o == "coat") {
        if (torso && face == FRONT && (x == 3 || x == 4)) return y <= 1 ? acc : shade(top, 0.9f);
        if (torso && face == FRONT && x == 2 && y % 3 == 1) return 0xFF8A8F98;
        return top;
    }
    if (o == "jersey") {
        if (face == BACK && torso && y >= 3 && y <= 7) {
            const char* number[] = { "##.##", ".#..#", ".#.##", ".#.#.", "###.#" };
            const int nx = x - 1;
            if (nx >= 0 && nx < 5 && number[y - 3][nx] == '#') return 0xFFF4F4F4;
        }
        if (y == 0 || (!torso && y == 3)) return 0xFFF4F4F4;
        return top;
    }
    if (o == "dress") {
        if (torso && y == 6) return acc;
        if (torso && face == FRONT && y <= 1 && x >= 2 && x <= 5) return l.skin;
        return (x + y) % 5 == 0 ? shade(top, 1.12f) : top;
    }
    if (o == "hoodie") {
        if (torso && face == FRONT && (x == 3 || x == 4) && y >= 1 && y <= 3) return 0xFFF1EEE6;
        if (torso && face == FRONT && y >= 7 && y <= 9 && x >= 1 && x <= 6) return shade(top, 0.82f);
        return top;
    }
    if (o == "jacket") {
        if (torso && face == FRONT && (x == 3 || x == 4)) return acc ? mix(acc, 0xFFFFFFFF, 0.3f) : 0xFFF1EEE6;
        if (torso && face == FRONT && (x == 2 || x == 5) && y <= 2) return shade(top, 1.25f);
        if (l.belt && torso && y == 10) return face == FRONT && (x == 3 || x == 4) ? 0xFFE0A526 : 0xFF3B2A1E;
        return top;
    }
    if (torso && face == FRONT && y == 0 && (x == 3 || x == 4)) return l.skin;
    if (torso && y == 11 && face != TOP && face != BOTTOM) return shade(top, 0.85f);
    return top;
}

void paintBody(Tex& t, const Look& l)
{
    t.box(16, 16, 8, 12, 4, [&](int face, int x, int y, int w, int h) -> QRgb {
        if (face == TOP) return (x >= 3 && x <= 4 && l.outfit != "armor" && l.outfit != "space") ? l.skin : cloth(l, face, x, y, w, h, true);
        return cloth(l, face, x, y, w, h, true);
    }, l.outfit == "armor" ? 0.03f : 0.07f);
    const QStringList rows = emblemRows(l.emblem);
    if (!rows.isEmpty()) {
        const QRgb c = l.outfit == "armor" ? 0xFFC62828 : contrast(l.top, l.accent);
        for (int j = 0; j < 5; j++) {
            for (int i = 0; i < 5; i++) {
                if (rows[j][i] == '#') {
                    t.set(22 + i - 1, 23 + j, j == 0 ? shade(c, 1.15f) : c);
                }
            }
        }
    }
    t.box(16, 32, 8, 12, 4, [&](int face, int x, int y, int, int) -> QRgb {
        if (face == TOP || face == BOTTOM) return 0;
        const QString& o = l.outfit;
        if (o == "jacket" || o == "pirate" || o == "coat") {
            if (face == FRONT && (x <= 1 || x >= 6)) return shade(l.top, 1.08f);
            if (face == RIGHT || face == LEFT || face == BACK) return y >= 10 ? shade(l.top, 0.95f) : 0;
            return 0;
        }
        if (o == "fur") return ((face == FRONT && (x <= 1 || x >= 6)) || (face != FRONT && y <= 3)) ? ((x + y) % 2 == 0 ? shade(l.accent, 1.2f) : l.accent) : 0;
        if (o == "robe" || o == "dress") return y >= 9 ? shade(l.top, 1.05f) : 0;
        if (o == "hoodie") return face == BACK && y <= 2 ? shade(l.top, 0.85f) : 0;
        if (o == "armor") return face == FRONT && y <= 1 ? shade(l.metal, 1.3f) : 0;
        return 0;
    }, 0.05f);
}

void paintArm(Tex& t, const Look& l, int u, int v, int ou, int ov, int armW)
{
    const bool shortSleeves = (l.outfit == "tshirt" && !l.longSleeves) || l.outfit == "jersey" || l.outfit == "fur";
    t.box(u, v, armW, 12, 4, [&](int face, int x, int y, int w, int h) -> QRgb {
        if (l.outfit == "skeleton") {
            const bool bone = (face == FRONT || face == BACK) ? (x == w / 2 || (x == w / 2 - 1 && w > 3)) : (x == 1 || x == 2);
            return (bone || face == BOTTOM || face == TOP) ? l.skin : l.top;
        }
        if (face == BOTTOM) return (l.gloves || l.outfit == "armor") ? shade(l.metal, 0.8f) : shade(l.skin, 0.9f);
        if (face == TOP) return l.outfit == "armor" ? shade(l.metal, 1.1f) : l.top;
        const int hand = 9;
        if (y >= hand + 1) {
            if (l.outfit == "armor" || l.outfit == "space") return l.outfit == "space" ? 0xFF8A8F98 : shade(l.metal, 0.85f);
            return y == 11 ? shade(l.skin, 0.92f) : l.skin;
        }
        if (shortSleeves && y >= 4) {
            if (l.outfit == "fur" && y >= 7) return (y == 7 || y == 9) ? 0xFF2B1B12 : 0xFF5B3A26;
            return l.skin;
        }
        if (y == hand) return l.outfit == "armor" ? shade(l.metal, 1.2f) : shade(cloth(l, face, x, y, w, h, false), 0.85f);
        if (l.outfit == "armor") return y <= 3 ? cloth(l, face, x, y, w, h, false) : ((x + y) % 2 == 0 ? 0xFF6F7680 : 0xFF525860);
        if (l.outfit == "ninja" && y % 3 == 2) return shade(l.top, 1.5f);
        if (l.outfit == "space" && y == 5) return l.accent;
        return cloth(l, face, x, y, w, h, false);
    }, 0.06f);
    t.box(ou, ov, armW, 12, 4, [&](int face, int x, int y, int, int) -> QRgb {
        if (face == BOTTOM) return 0;
        if (l.outfit == "armor" && y <= 3) return (y == 0 || face == TOP) ? shade(l.metal, 1.3f) : shade(l.metal, 1.1f);
        if ((l.outfit == "robe" || l.outfit == "dress") && y >= 6 && y <= 9 && face != TOP) return y == 9 ? l.accent : shade(l.top, 1.05f);
        if (l.outfit == "fur" && y <= 2 && face != TOP) return (x + y) % 2 == 0 ? shade(l.accent, 1.2f) : l.accent;
        return 0;
    }, 0.04f);
}

void paintLeg(Tex& t, const Look& l, int u, int v, int ou, int ov, bool right)
{
    t.box(u, v, 4, 12, 4, [&](int face, int x, int y, int w, int h) -> QRgb {
        if (l.outfit == "skeleton") return (x == 1 || x == 2 || face == BOTTOM) ? l.skin : l.top;
        const int shoe = l.outfit == "dress" ? 10 : 9;
        if (face == BOTTOM) return shade(l.shoes, 0.6f);
        if (face == TOP) return l.pants;
        if (y >= shoe + 1 || (y == shoe && l.outfit == "pirate") || (l.theme == "viking" && y >= 8)) {
            if (face == FRONT && y == 11) return mix(l.shoes, 0xFFFFFFFF, 0.25f);
            return y == 11 ? shade(l.shoes, 0.8f) : l.shoes;
        }
        if (l.outfit == "pirate" && y >= 6) return y == 6 ? shade(l.shoes, 1.3f) : l.shoes;
        if (l.outfit == "dress") return y <= 4 ? l.top : l.skin;
        if (l.outfit == "armor") return (y == 4 || y == 5) ? shade(l.metal, 1.2f) : (x + y) % 5 == 0 ? shade(l.metal, 1.15f) : l.metal;
        if (l.outfit == "camo" || l.outfit == "space" || l.outfit == "ninja") return cloth(l, face, x, y, w, h, false);
        if (l.outfit == "robe") return y <= 7 ? l.top : shade(l.top, 0.8f);
        QRgb p = l.pants;
        if (right ? face == LEFT : face == RIGHT) p = shade(p, 0.88f);
        if (face == FRONT && (right ? x == 3 : x == 0)) p = shade(p, 0.9f);
        if (y == 5 && face == FRONT) p = shade(p, 1.08f);
        return p;
    }, 0.07f);
    t.box(ou, ov, 4, 12, 4, [&](int face, int, int y, int, int) -> QRgb {
        if (face == TOP || face == BOTTOM) return 0;
        if (l.outfit == "dress" && y <= 6) return y == 6 ? l.accent : shade(l.top, 1.06f);
        if (l.outfit == "robe" && y <= 9) return y == 9 ? l.accent : l.top;
        return 0;
    }, 0.05f);
}

QImage paint(const Look& l, QRandomGenerator& rnd)
{
    Tex t(64, 64, rnd);
    const int armW = l.girl ? 3 : 4;
    paintHead(t, l);
    paintBody(t, l);
    paintArm(t, l, 40, 16, 40, 32, armW);
    paintArm(t, l, 32, 48, 48, 48, armW);
    paintLeg(t, l, 0, 16, 0, 32, true);
    paintLeg(t, l, 16, 48, 0, 48, false);
    return t.img;
}

QImage cape(QRgb main, QRgb second, const QString& style, const QString& emblem, QRandomGenerator& rnd)
{
    Tex t(64, 32, rnd);
    const QRgb dark = shade(main, 0.55f);
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 10; x++) {
            const float g = y / 15.f;
            QRgb c;
            if (style == "stripes") {
                c = (x / 2) % 2 == 0 ? main : second;
            } else if (style == "checker") {
                c = ((x / 2) + (y / 2)) % 2 == 0 ? main : second;
            } else if (style == "diagonal") {
                c = ((x + y) / 3) % 2 == 0 ? main : second;
            } else if (style == "galaxy") {
                const QRgb space = mix(0xFF0B0B2A, 0xFF2A0B3A, g);
                c = mix(space, main, float(0.25 + 0.2 * std::sin(x * 0.9 + y * 0.5)));
                if (rnd.bounded(14) == 0) {
                    c = rnd.bounded(3) == 0 ? 0xFFFFF2B0 : 0xFFFFFFFF;
                }
            } else if (style == "flames") {
                const float heat = g + float(std::sin(x * 1.7 + y * 0.6)) * 0.18f;
                c = heat > 0.85f ? 0xFFFFE06A : heat > 0.6f ? 0xFFFF9A1E : heat > 0.35f ? 0xFFE53935 : mix(0xFF2A0B0B, main, 0.4f);
            } else if (style == "rainbow") {
                const QRgb rb[] = { 0xFFE53935, 0xFFEF7C1E, 0xFFF2C230, 0xFF3E9B47, 0xFF2C5BD6, 0xFF7B3FC4 };
                c = rb[(y * 6) / 16];
            } else if (style == "ice") {
                c = mix(0xFFE8F6FF, 0xFF5AB4F0, g);
                if ((x * 7 + y * 3) % 11 == 0) {
                    c = 0xFFFFFFFF;
                }
            } else if (style == "leaves") {
                const QRgb leaves[] = { 0xFF2E7D32, 0xFF3E9B47, 0xFF66BB6A, 0xFF1B5E20 };
                c = leaves[(x * 3 + y * 5 + rnd.bounded(2)) % 4];
            } else {
                c = mix(main, shade(second, 0.7f), g * 0.8f);
            }
            if (style == "border" && (x == 0 || x == 9 || y == 0 || y == 15)) {
                c = second;
            }
            t.set(1 + x, 1 + y, shade(c, 1 + (float(rnd.generateDouble()) - 0.5f) * 0.08f));
        }
    }
    const QStringList rows = emblemRows(emblem);
    if (!rows.isEmpty()) {
        QRgb ec = contrast(main, second == main ? 0xFFF4F4F4 : second);
        if (style == "galaxy" || style == "flames") {
            ec = 0xFFF4F4F4;
        }
        for (int pass = 0; pass < 2; pass++) {
            for (int j = 0; j < 5; j++) {
                for (int i = 0; i < 5; i++) {
                    if (rows[j][i] == '#') {
                        if (pass == 0) {
                            t.set(4 + i, 7 + j, shade(ec, 0.4f));
                        } else {
                            t.set(3 + i, 6 + j, j == 0 ? shade(ec, 1.15f) : ec);
                        }
                    }
                }
            }
        }
    }
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 10; x++) {
            t.set(12 + x, 1 + y, shade(mix(main, 0xFF101014, 0.55f), 1 + (float(rnd.generateDouble()) - 0.5f) * 0.06f));
        }
        t.set(0, 1 + y, dark);
        t.set(11, 1 + y, dark);
    }
    for (int x = 0; x < 10; x++) {
        t.set(1 + x, 0, shade(main, 1.1f));
        t.set(11 + x, 0, dark);
    }
    return t.img;
}
}  // namespace

QList<Skin> skins(const QString& prompt, int count, qint64 seed, const Options& options)
{
    QList<Skin> out;
    for (int i = 0; i < count; i++) {
        QRandomGenerator rnd(quint32(seed * 31 + i * 7919 + qHash(prompt)));
        const Look l = look(prompt, rnd, i, options);
        out.append({ paint(l, rnd), l.girl, QString("%1 #%2").arg(l.title).arg(i + 1) });
    }
    return out;
}

QList<QImage> capes(const QString& prompt, int count, qint64 seed, const Options& o)
{
    const QStringList w = words(prompt);
    const QList<QRgb> said = clothesColors(w);
    QString emblem;
    for (const auto& e : EMBLEMS) {
        if (has(w, { QString::fromUtf8(e.first) })) {
            emblem = e.second;
            break;
        }
    }
    QString style;
    if (has(w, L({ "космос", "галакт", "galaxy", "space", "звезд", "stars", "ночн", "night" }))) {
        style = "galaxy";
    } else if (has(w, L({ "огон", "огн", "пламя", "fire", "flame", "лава", "lava" }))) {
        style = "flames";
    } else if (has(w, L({ "полос", "stripe" }))) {
        style = "stripes";
    } else if (has(w, L({ "клетк", "шахмат", "check" }))) {
        style = "checker";
    } else if (has(w, L({ "радуг", "rainbow" }))) {
        style = "rainbow";
    } else if (has(w, L({ "лед", "снег", "ice", "snow", "зим", "frost" }))) {
        style = "ice";
    } else if (has(w, L({ "лес", "природ", "лист", "forest", "leaf", "nature" }))) {
        style = "leaves";
    }
    const char* styles[] = { "gradient", "emblem", "stripes", "galaxy", "flames", "checker", "border", "diagonal" };
    QList<QImage> out;
    for (int i = 0; i < count; i++) {
        QRandomGenerator rnd(quint32(seed * 131 + i * 104729 + qHash(prompt)));
        QRgb main = !said.isEmpty() ? said.first() : HUE[rnd.bounded(10)];
        if (i > 0 && said.isEmpty()) {
            main = HUE[(rnd.bounded(10) + i) % 10];
        }
        if (o.clothes) {
            main = o.clothes;
        }
        QRgb second = said.size() > 1 ? said[1] : i % 2 == 0 ? shade(main, 0.55f) : mix(main, 0xFFFFFFFF, 0.35f);
        if (o.second) {
            second = o.second;
        }
        QString s = !style.isEmpty() ? style : QString(styles[(i * 3 + rnd.bounded(8)) % 8]);
        if (o.pattern) {
            s = *o.pattern;
        }
        QString mark = !emblem.isEmpty() ? emblem : (s == "emblem" || s == "border") ? QString(EMBLEM_CHOICES[rnd.bounded(7)]) : QString();
        if (o.emblem) {
            mark = *o.emblem;
        } else if (o.pattern && emblem.isEmpty()) {
            // one pattern for all four: they differ by the picture on them
            mark = i == 0 ? QString() : QString(EMBLEM_CHOICES[(i * 2 + rnd.bounded(3)) % 7]);
        }
        out.append(cape(main, second, s, mark, rnd));
    }
    return out;
}

QString surprise(bool cape)
{
    auto* rnd = QRandomGenerator::global();
    auto any = [rnd](const QStringList& list) { return list.at(int(rnd->bounded(quint32(list.size())))); };
    QString text;
    if (cape) {
        const QString what = any(L({ "галактика", "огненный плащ", "ледяной плащ", "плащ в полоску", "плащ в клетку", "радужный плащ", "лесной плащ",
                                     "плащ с рамкой", "плащ" }));
        const QString color = any(L({ "красный", "бордовый", "синий", "зелёный", "фиолетовый", "золотой", "чёрный", "белый", "розовый", "оранжевый",
                                      "бирюзовый", "голубой" }));
        const bool own = what.startsWith(QString::fromUtf8("галактика")) || what.startsWith(QString::fromUtf8("радужн")) ||
                         what.startsWith(QString::fromUtf8("огнен")) || what.startsWith(QString::fromUtf8("ледян")) || what.startsWith(QString::fromUtf8("лесн"));
        text = own ? what : color + ' ' + what;
        const QString mark = any(L({ "со звездой", "с сердцем", "с молнией", "с короной", "с луной", "с черепом", "с крипером", "с алмазом", "с пламенем", "", "" }));
        if (!mark.isEmpty()) {
            text += ' ' + mark;
        }
    } else {
        text = any(L({ "рыцарь", "ниндзя", "маг", "пират", "принцесса", "космонавт", "демон", "кибер", "зомби", "скелет", "эльф", "викинг", "вампир", "ангел",
                       "король", "солдат", "хакер в худи", "агент в костюме", "повар", "доктор", "футболист", "ведьма", "девушка-маг", "девушка-эльф",
                       "девушка в худи" }));
        const QString color = any(L({ "в красном", "в бордовом", "в синем", "в зелёном", "в фиолетовом", "в золотом", "в чёрном", "в белом", "в розовом",
                                      "в оранжевом", "в бирюзовом", "в голубом", "" }));
        if (!color.isEmpty()) {
            text += ' ' + color;
        }
        const QString detail = any(L({ "с короной", "с бородой", "со светящимися глазами", "с черепом", "со звездой", "с молнией", "с длинными волосами",
                                       "с ирокезом", "в капюшоне", "в шлеме", "с сердцем", "рыжий", "блондин", "", "" }));
        if (!detail.isEmpty()) {
            text += ' ' + detail;
        }
    }
    return text.left(1).toUpper() + text.mid(1);
}
}  // namespace SkinGen
