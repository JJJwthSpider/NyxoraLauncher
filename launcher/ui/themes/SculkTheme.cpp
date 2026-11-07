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
#include "SculkTheme.h"

#include <QDir>
#include <algorithm>
#include <QFileInfo>
#include <QHash>
#include <QObject>
#include <QPainter>

// The style sheet below uses @tokens@ that are filled from the color table of the variant:
//   window / panel / surface / hover   backgrounds from the window down to hovered controls
//   base                               lists, editors, consoles
//   border, bar                        frames, menu and tool bars
//   glow / accent / deep               the cyan sculk glow, selections, pressed states
//   text / muted / disabled            primary, secondary and disabled text

SculkTheme::SculkTheme(Variant variant) : m_variant(variant) {}

namespace {
bool s_active = false;
bool s_lunar = false;

// a small integer hash, so the texture is random looking but the same on every start
quint32 cellHash(int x, int y, int salt)
{
    quint32 h = quint32(x) * 374761393u + quint32(y) * 668265263u + quint32(salt) * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

QColor mix(const QColor& a, const QColor& b, double t)
{
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t, a.greenF() + (b.greenF() - a.greenF()) * t,
                            a.blueF() + (b.blueF() - a.blueF()) * t);
}
}  // namespace

bool SculkTheme::isActive()
{
    return s_active;
}

void SculkTheme::setActive(bool active)
{
    s_active = active;
}

bool SculkTheme::isLunar()
{
    return s_lunar;
}

void SculkTheme::setLunar(bool lunar)
{
    s_lunar = lunar;
}

namespace {
// 48x48 sculk pixels per tile: large enough that the repetition is hard to spot
constexpr int s_grid = 48;

// smooth value noise on a lattice of `cells` points per side, wrapped so the tile stays seamless
double valueNoise(int x, int y, int cells, int salt)
{
    const double step = double(s_grid) / cells;
    const double fx = x / step, fy = y / step;
    const int x0 = int(fx), y0 = int(fy);
    const double tx = fx - x0, ty = fy - y0;
    auto at = [cells, salt](int lx, int ly) { return (cellHash(((lx % cells) + cells) % cells, ((ly % cells) + cells) % cells, salt) % 1000) / 1000.0; };
    auto smooth = [](double t) { return t * t * (3 - 2 * t); };
    const double sx = smooth(tx), sy = smooth(ty);
    const double top = at(x0, y0) + (at(x0 + 1, y0) - at(x0, y0)) * sx;
    const double bottom = at(x0, y0 + 1) + (at(x0 + 1, y0 + 1) - at(x0, y0 + 1)) * sx;
    return top + (bottom - top) * sy;
}

// the glowing part of the texture: branching tendrils that end in soul dots, and a few sensor-like clusters.
// calls paint(x, y, isSoul) for every pixel; everything wraps around the edges of the tile
template <typename Paint>
void forEachVeinPixel(Paint paint)
{
    static const int dx[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
    static const int dy[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
    auto wrap = [](int v) { return ((v % s_grid) + s_grid) % s_grid; };
    auto walk = [&](int x, int y, int dir, int length, int salt, auto&& self, int depth) -> void {
        for (int step = 0; step < length; step++) {
            paint(x, y, step == length - 1);
            const quint32 r = cellHash(salt, step, 7) % 100;
            if (r < 22) {
                dir = (dir + 1) % 8;  // the tendril bends a little, never turns back
            } else if (r < 44) {
                dir = (dir + 7) % 8;
            }
            if (depth == 0 && step == length / 2) {
                self(x, y, (dir + 2) % 8, 3 + int(cellHash(salt, 1, 8) % 3), salt * 31 + 7, self, 1);  // one side branch
            }
            x = wrap(x + dx[dir]);
            y = wrap(y + dy[dir]);
        }
    };
    for (int v = 0; v < 10; v++) {
        walk(int(cellHash(v, 0, 3) % s_grid), int(cellHash(v, 0, 4) % s_grid), int(cellHash(v, 0, 5) % 8),
             7 + int(cellHash(v, 0, 6) % 8), v + 1, walk, 0);
    }
    for (int c = 0; c < 3; c++) {
        const int x = int(cellHash(c, 9, 9) % s_grid), y = int(cellHash(c, 9, 10) % s_grid);
        for (int i = 0; i < 4; i++) {
            paint(wrap(x + i % 2), wrap(y + i / 2), true);
        }
    }
}

QPixmap enlarge(const QImage& image, int pixelSize)
{
    return QPixmap::fromImage(image.scaled(s_grid * pixelSize, s_grid * pixelSize, Qt::IgnoreAspectRatio, Qt::FastTransformation));
}
}  // namespace

QPixmap SculkTheme::starTexture(const QColor& background, int size)
{
    static QHash<QString, QPixmap> cache;
    const QString key = background.name() + QString::number(size);
    if (auto it = cache.constFind(key); it != cache.constEnd()) {
        return *it;
    }
    // Vellora: the still red rain behind the panels: faint slanted streaks; the tile wraps around without seams
    QImage image(size, size, QImage::Format_RGB32);
    image.fill(background);
    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing, true);
    const int count = size * size / 1400;
    for (int i = 0; i < count; i++) {
        const double x = cellHash(i, 1, 41) % (size * 10) / 10.0;
        const double y = cellHash(i, 2, 41) % (size * 10) / 10.0;
        const double length = 8 + cellHash(i, 3, 41) % 14;
        QColor c(255, 60, 80);
        c.setAlphaF(0.06 + (cellHash(i, 5, 41) % 12) / 100.0);
        p.setPen(QPen(c, 1.0, Qt::SolidLine, Qt::RoundCap));
        for (int dx = -size; dx <= size; dx += size) {
            for (int dy = -size; dy <= size; dy += size) {
                p.drawLine(QPointF(x + dx, y + dy), QPointF(x + dx + length * 0.16, y + dy + length));
            }
        }
    }
    p.end();
    const QPixmap pixmap = QPixmap::fromImage(image);
    cache.insert(key, pixmap);
    return pixmap;
}

QPixmap SculkTheme::texture(const QColor& background, const QColor& glow, int pixelSize)
{
    if (s_lunar) {
        // the Vellora theme (id "lunar") has no sculk: everything lies on the burgundy night with red rain
        return starTexture(background, 64 * pixelSize);
    }
    static QHash<QString, QPixmap> cache;
    const QString key = background.name() + glow.name() + QString::number(pixelSize);
    if (auto it = cache.constFind(key); it != cache.constEnd()) {
        return *it;
    }

    // like the sculk block: soft blotches in five shades of the skin, crossed by glowing tendrils
    const bool lightBackground = background.lightnessF() > 0.6;
    // on a light background darkening alone gives grey, so the shades lean towards the sculk color instead
    const QColor tint = glow.darker(170);
    const QColor shades[5] = {
        lightBackground ? mix(background, tint, 0.13) : background.darker(140),
        lightBackground ? mix(background, tint, 0.07) : background.darker(118),
        background,
        lightBackground ? mix(background, tint, 0.03) : background.lighter(110),
        lightBackground ? background.lighter(102) : background.lighter(122),
    };
    const QColor vein = mix(background, glow, lightBackground ? 0.22 : 0.32);
    const QColor soul = mix(background, glow, lightBackground ? 0.45 : 0.66);

    QImage image(s_grid, s_grid, QImage::Format_RGB32);
    for (int y = 0; y < s_grid; y++) {
        for (int x = 0; x < s_grid; x++) {
            const double n = 0.6 * valueNoise(x, y, 6, 11) + 0.3 * valueNoise(x, y, 12, 12) + 0.1 * ((cellHash(x, y, 2) % 100) / 100.0);
            const int level = std::clamp(int((n - 0.18) / 0.64 * 5), 0, 4);
            image.setPixel(x, y, shades[level].rgb());
        }
    }
    forEachVeinPixel([&](int x, int y, bool isSoul) { image.setPixel(x, y, (isSoul ? soul : vein).rgb()); });

    const QPixmap pixmap = enlarge(image, pixelSize);
    cache.insert(key, pixmap);
    return pixmap;
}

QPixmap SculkTheme::veinTexture(const QColor& glow, int pixelSize)
{
    static QHash<QString, QPixmap> cache;
    const QString key = glow.name() + QString::number(pixelSize);
    if (auto it = cache.constFind(key); it != cache.constEnd()) {
        return *it;
    }
    QImage image(s_grid, s_grid, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    const QColor soul = glow.lighter(130);
    forEachVeinPixel([&](int x, int y, bool isSoul) { image.setPixel(x, y, (isSoul ? soul : glow).rgba()); });
    const QPixmap pixmap = enlarge(image, pixelSize);
    cache.insert(key, pixmap);
    return pixmap;
}

QString SculkTheme::textureFile(const QColor& background, const QColor& glow)
{
    // style sheets can only load images from files, so the generated texture is written to the temp folder once
    const QString path = QDir::temp().filePath(
        QString("vellora-texture-v5-%1-%2-%3.png").arg(s_lunar ? "rain" : "sculk", background.name().mid(1), glow.name().mid(1)));
    if (!QFileInfo::exists(path)) {
        texture(background, glow).save(path, "PNG");
    }
    return QDir::fromNativeSeparators(path);
}

const SculkTheme::Colors& SculkTheme::colors() const
{
    // the same sculk hues in three brightness levels
    static const Colors s_default{
        "#f0fdfd", "#7aa0a6", "#1c4049", "#3e7884", "#24515c", "#173a43", "#2c6170", "#3ff0f8", "#12a3ad",
        "#ffffff", "#214a54", "#6b9aa1", "#0f7c86", "#163640", "#b0d3d7", "#24525d", "#18bcc7", "#a6f6f9",
    };
    static const Colors s_dark{
        "#d6f4f5", "#4b6a70", "#0a1a1f", "#1b4651", "#0f252c", "#071418", "#143440", "#29dfeb", "#0e8f99",
        "#ffffff", "#0b1f25", "#3d6b72", "#0a5a63", "#050f12", "#7fa3a8", "#0f2b33", "#12a6b1", "#7fe9ee",
    };
    static const Colors s_light{
        "#0c2a31", "#8eacb1", "#e9f6f7", "#9fd0d5", "#d8eef0", "#dbf0f2", "#c6e7ea", "#0aa5b0", "#0e8f99",
        "#ffffff", "#e0f2f4", "#a9c4c8", "#0b7680", "#ffffff", "#4f7379", "#d4eef1", "#10a6b1", "#0b7680",
    };
    // Vellora Client (the old "lunar" id): burgundy night and crimson; the crimson gradients are added by the style sheet
    static const Colors s_lunar{
        "#fbeff1", "#8a5a62", "#0e0407", "#5a1e2a", "#2b0e15", "#140509", "#3d121c", "#ff4d5e", "#e8364f",
        "#ffffff", "#1c080d", "#7a4a52", "#c81f3a", "#090204", "#d1a3aa", "#2e0d14", "#ff5a6e", "#ffffff",
    };
    switch (m_variant) {
        case Variant::Dark:
            return s_dark;
        case Variant::Light:
            return s_light;
        case Variant::Lunar:
            return s_lunar;
        default:
            return s_default;
    }
}

QString SculkTheme::id()
{
    switch (m_variant) {
        case Variant::Dark:
            return "sculk_dark";
        case Variant::Light:
            return "sculk_light";
        case Variant::Lunar:
            return "lunar";
        default:
            return "sculk";
    }
}

QString SculkTheme::name()
{
    switch (m_variant) {
        case Variant::Dark:
            return QObject::tr("Sculk (Dark)");
        case Variant::Light:
            return QObject::tr("Sculk (Light)");
        case Variant::Lunar:
            return QObject::tr("Vellora (Red rain)");
        default:
            return QObject::tr("Sculk");
    }
}

QString SculkTheme::tooltip()
{
    if (m_variant == Variant::Lunar) {
        return QObject::tr("Burgundy and crimson with red rain falling in the background");
    }
    return QObject::tr("Modern theme in the colors of Minecraft sculk");
}

QPalette SculkTheme::colorScheme()
{
    const auto& c = colors();
    const bool light = m_variant == Variant::Light;
    QPalette p;
    p.setColor(QPalette::Window, QColor(c.window));
    p.setColor(QPalette::WindowText, QColor(c.text));
    p.setColor(QPalette::Base, QColor(c.base));
    p.setColor(QPalette::AlternateBase, QColor(c.panel));
    p.setColor(QPalette::ToolTipBase, QColor(c.surface));
    p.setColor(QPalette::ToolTipText, QColor(c.text));
    p.setColor(QPalette::Text, QColor(c.text));
    p.setColor(QPalette::Button, QColor(c.surface));
    p.setColor(QPalette::ButtonText, QColor(c.text));
    p.setColor(QPalette::BrightText, QColor(c.glow));
    p.setColor(QPalette::Link, QColor(light ? c.deep : c.glow));
    p.setColor(QPalette::LinkVisited, QColor(c.accent));
    p.setColor(QPalette::Highlight, QColor(c.accent));
    p.setColor(QPalette::HighlightedText, QColor(c.onAccent));
    p.setColor(QPalette::PlaceholderText, QColor(c.muted));
    p.setColor(QPalette::Light, QColor(c.border));
    p.setColor(QPalette::Midlight, QColor(c.hover));
    p.setColor(QPalette::Mid, QColor(c.surface));
    p.setColor(QPalette::Dark, QColor(light ? c.border : c.base));
    p.setColor(QPalette::Shadow, QColor(0, 0, 0));
    return fadeInactive(p, fadeAmount(), fadeColor());
}

double SculkTheme::fadeAmount()
{
    return 0.55;
}

QColor SculkTheme::fadeColor()
{
    return QColor(colors().window);
}

LogColors SculkTheme::logColorScheme()
{
    LogColors result;
    if (m_variant == Variant::Light) {
        result.background[MessageLevel::Fatal] = QColor(0xff, 0xdd, 0xe1);
        result.foreground[MessageLevel::Launcher] = QColor(0x0b, 0x76, 0x80);
        result.foreground[MessageLevel::Debug] = QColor(0x4f, 0x73, 0x79);
        result.foreground[MessageLevel::Warning] = QColor(0x9a, 0x62, 0x00);
        result.foreground[MessageLevel::Error] = QColor(0xc0, 0x1f, 0x35);
        result.foreground[MessageLevel::Fatal] = QColor(0x9b, 0x10, 0x22);
        return result;
    }
    result.background[MessageLevel::Fatal] = QColor(0x3a, 0x08, 0x10);
    result.foreground[MessageLevel::Launcher] = QColor(colors().glow);
    result.foreground[MessageLevel::Debug] = QColor(colors().muted);
    result.foreground[MessageLevel::Warning] = QColor(0xf2, 0xc2, 0x5c);
    result.foreground[MessageLevel::Error] = QColor(0xff, 0x6b, 0x7a);
    result.foreground[MessageLevel::Fatal] = QColor(0xff, 0x9a, 0xa5);
    return result;
}

bool SculkTheme::hasStyleSheet()
{
    return true;
}

namespace {
QString glass(const QString& color, double alpha)
{
    const QColor c(color);
    return QString("rgba(%1, %2, %3, %4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(int(alpha * 255));
}
}  // namespace

QString SculkTheme::appStyleSheet()
{
    QString qss = QStringLiteral(R"QSS(
/* ---------- base ---------- */
QWidget {
    color: @text@;
    selection-background-color: @accent@;
    selection-color: @onAccent@;
}
QWidget:disabled {
    color: @disabled@;
}
QMainWindow, QDialog, QWizard {
    background-color: @window@;
}
QMainWindow {
    background-image: url("@windowTexture@");
}
QMainWindow::separator {
    background: @border@;
    width: 1px;
    height: 1px;
}

/* ---------- tooltips ---------- */
QToolTip {
    color: @text@;
    background-color: @surface@;
    border: 1px solid @glow@;
    border-radius: 6px;
    padding: 6px 8px;
}

/* ---------- menus ---------- */
QMenuBar {
    background-color: @barGlass@;
    border-bottom: 1px solid @border@;
    padding: 2px 4px;
}
QMenuBar::item {
    background: transparent;
    padding: 6px 12px;
    border-radius: 6px;
}
QMenuBar::item:selected {
    background-color: @hover@;
    color: @glow@;
}
QMenuBar::item:pressed {
    background-color: @accent@;
    color: @onAccent@;
}
QMenu {
    background-color: @panel@;
    border: 1px solid @border@;
    border-radius: 8px;
    padding: 6px;
}
QMenu::item {
    padding: 7px 28px 7px 28px;
    border-radius: 6px;
    background: transparent;
}
QMenu::item:selected {
    background-color: @accent@;
    color: @onAccent@;
}
QMenu::item:disabled {
    color: @disabled@;
}
QMenu::separator {
    height: 1px;
    background: @border@;
    margin: 6px 8px;
}
QMenu::icon {
    padding-left: 8px;
}

/* ---------- tool bars ---------- */
QToolBar {
    background-color: @barGlass@;
    border: none;
    spacing: 4px;
    padding: 4px;
}
QToolBar#mainToolBar {
    border-bottom: 1px solid @border@;
}
QToolBar#newsToolBar {
    border-top: 1px solid @border@;
}
QToolBar#instanceToolBar {
    background-color: @panelGlass@;
    border-left: 1px solid @border@;
    padding: 8px 6px;
}
/* the main action of the instance panel stands out, deleting warns in red */
QToolBar#instanceToolBar QToolButton#primaryAction {
    background-color: @accent@;
    color: @onAccent@;
    font-weight: bold;
    border: 1px solid @glow@;
    border-radius: 8px;
    padding: 9px 8px;
    margin: 2px 0 6px 0;
}
QToolBar#instanceToolBar QToolButton#primaryAction:hover {
    background-color: @accentHover@;
}
QToolBar#instanceToolBar QToolButton#primaryAction:disabled {
    background-color: @surface@;
    color: @disabled@;
    border-color: @border@;
}
QToolBar#instanceToolBar QToolButton#dangerAction:hover {
    background-color: #b3261e;
    border-color: #e5534b;
    color: #ffffff;
}
/* search and sorting above the instance list */
QWidget#instanceSearchBar {
    background-color: @barGlass@;
    border-bottom: 1px solid @border@;
}
QLineEdit#instanceSearch {
    padding: 6px 10px;
    border-radius: 8px;
}
QLabel#instanceCount {
    color: @muted@;
    padding: 0 4px;
}
QToolBar::separator {
    background: @border@;
    width: 1px;
    height: 1px;
    margin: 6px 4px;
}
QToolBar QToolButton {
    background: transparent;
    border: 1px solid transparent;
    border-radius: 8px;
    padding: 6px 10px;
    margin: 1px;
}
/* the main tool bar holds many labelled buttons: tighter, so they fit into smaller windows too */
QToolBar#mainToolBar QToolButton {
    padding: 6px 4px;
}
QToolBar QToolButton:hover {
    background-color: @hover@;
    border-color: @border@;
    color: @glow@;
}
QToolBar QToolButton:pressed, QToolBar QToolButton:checked {
    background-color: @accent@;
    border-color: @glow@;
    color: @onAccent@;
}
QToolBar QToolButton:disabled {
    color: @disabledIcon@;
}

/* ---------- buttons ---------- */
QPushButton {
    background-color: @surface@;
    border: 1px solid @border@;
    border-radius: 8px;
    padding: 7px 16px;
    min-height: 18px;
}
QPushButton:hover {
    background-color: @hover@;
    border-color: @glow@;
    color: @glow@;
}
QPushButton:pressed, QPushButton:checked {
    background-color: @accent@;
    border-color: @glow@;
    color: @onAccent@;
}
QPushButton:default {
    background-color: @deep@;
    border-color: @glow@;
    color: @onAccent@;
}
QPushButton:default:hover {
    background-color: @accent@;
}
QPushButton:disabled {
    background-color: @window@;
    border-color: @hover@;
    color: @disabledIcon@;
}
QPushButton:focus {
    outline: none;
}
QPushButton::menu-indicator {
    image: url(@arrows@arrow-down.svg);
    subcontrol-origin: padding;
    subcontrol-position: center right;
    width: 12px;
    right: 4px;
}
/* one-click search / example chips */
QPushButton#quickChip {
    background-color: @panel@;
    border: 1px solid @border@;
    border-radius: 13px;
    padding: 4px 12px;
    min-height: 0px;
    color: @chipText@;
}
QPushButton#quickChip:hover {
    background-color: @hover@;
    border-color: @glow@;
    color: @glow@;
}
QPushButton#quickChip:pressed {
    background-color: @accent@;
    color: @onAccent@;
}
/* the main action of a page */
QPushButton#accentButton {
    background-color: @accent@;
    border: 1px solid @glow@;
    color: @onAccent@;
    font-weight: bold;
    padding: 7px 22px;
}
QPushButton#accentButton:hover {
    background-color: @accentHover@;
    color: @onAccent@;
}
/* big loader choice buttons on the quick start page */
QPushButton#loaderCard {
    border-radius: 10px;
    padding: 10px 16px;
    font-weight: bold;
}
QPushButton#loaderCard:checked {
    background-color: @deep@;
    border: 2px solid @glow@;
    color: @onAccent@;
}
QLabel#hintLabel {
    color: @muted@;
}
QToolButton {
    background-color: @surface@;
    border: 1px solid @border@;
    border-radius: 8px;
    padding: 5px;
}
QToolButton:hover {
    background-color: @hover@;
    border-color: @glow@;
}
QToolButton:pressed, QToolButton:checked {
    background-color: @accent@;
    border-color: @glow@;
}
QToolButton[popupMode="1"] {
    padding-right: 18px;
}
QToolButton::menu-button {
    border: none;
    border-left: 1px solid @border@;
    width: 16px;
}
QToolButton::menu-arrow, QToolButton::menu-indicator {
    image: url(@arrows@arrow-down.svg);
    width: 10px;
    height: 10px;
}
QToolButton::menu-indicator {
    subcontrol-position: bottom right;
}

/* ---------- inputs ---------- */
QLineEdit, QPlainTextEdit, QTextEdit, QTextBrowser, QAbstractSpinBox, QKeySequenceEdit {
    background-color: @base@;
    border: 1px solid @border@;
    border-radius: 8px;
    padding: 5px 8px;
    selection-background-color: @accent@;
}
QPlainTextEdit, QTextEdit, QTextBrowser {
    padding: 4px;
}
QLineEdit:hover, QPlainTextEdit:hover, QTextEdit:hover, QAbstractSpinBox:hover {
    border-color: @accent@;
}
QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus, QAbstractSpinBox:focus {
    border: 1px solid @glow@;
}
QLineEdit:disabled, QAbstractSpinBox:disabled, QPlainTextEdit:disabled, QTextEdit:disabled {
    background-color: @window@;
    border-color: @hover@;
}
QAbstractSpinBox::up-button, QAbstractSpinBox::down-button {
    subcontrol-origin: padding;
    width: 20px;
    border: none;
    border-left: 1px solid @border@;
    background: @surface@;
}
QAbstractSpinBox::up-button {
    subcontrol-position: top right;
    border-top-right-radius: 8px;
}
QAbstractSpinBox::down-button {
    subcontrol-position: bottom right;
    border-bottom-right-radius: 8px;
}
QAbstractSpinBox::up-button:hover, QAbstractSpinBox::down-button:hover {
    background: @hover@;
}
QAbstractSpinBox::up-arrow {
    image: url(@arrows@arrow-up.svg);
    width: 10px;
    height: 10px;
}
QAbstractSpinBox::down-arrow {
    image: url(@arrows@arrow-down.svg);
    width: 10px;
    height: 10px;
}
QAbstractSpinBox::up-arrow:disabled, QAbstractSpinBox::up-arrow:off {
    image: url(@arrows@arrow-up-disabled.svg);
}
QAbstractSpinBox::down-arrow:disabled, QAbstractSpinBox::down-arrow:off {
    image: url(@arrows@arrow-down-disabled.svg);
}

QComboBox {
    background-color: @surface@;
    border: 1px solid @border@;
    border-radius: 8px;
    padding: 5px 30px 5px 10px;
    min-height: 18px;
}
QComboBox:editable {
    background-color: @base@;
}
QComboBox:hover {
    border-color: @accent@;
}
QComboBox:focus, QComboBox:on {
    border-color: @glow@;
}
QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: center right;
    width: 26px;
    border: none;
}
QComboBox::down-arrow {
    image: url(@arrows@arrow-down.svg);
    width: 12px;
    height: 12px;
}
QComboBox::down-arrow:disabled {
    image: url(@arrows@arrow-down-disabled.svg);
}
QComboBox QAbstractItemView {
    background-color: @panel@;
    border: 1px solid @border@;
    border-radius: 6px;
    padding: 4px;
    outline: none;
}

/* ---------- check boxes & radio buttons ---------- */
QCheckBox, QRadioButton {
    spacing: 8px;
    background: transparent;
}
QCheckBox::indicator, QRadioButton::indicator, QGroupBox::indicator,
QAbstractItemView::indicator {
    width: 16px;
    height: 16px;
    border: 1px solid @border@;
    background-color: @base@;
}
QCheckBox::indicator, QGroupBox::indicator, QAbstractItemView::indicator {
    border-radius: 4px;
}
QRadioButton::indicator {
    border-radius: 9px;
}
QCheckBox::indicator:hover, QRadioButton::indicator:hover, QGroupBox::indicator:hover,
QAbstractItemView::indicator:hover {
    border-color: @glow@;
}
QCheckBox::indicator:checked, QGroupBox::indicator:checked, QAbstractItemView::indicator:checked {
    background-color: @glow@;
    border-color: @glow@;
    image: url(:/sculk_theme/check.svg);
}
QCheckBox::indicator:indeterminate, QAbstractItemView::indicator:indeterminate {
    background-color: @glow@;
    border-color: @glow@;
    image: url(:/sculk_theme/partial.svg);
}
QRadioButton::indicator:checked {
    background-color: @glow@;
    border-color: @glow@;
    image: url(:/sculk_theme/radio.svg);
}
QCheckBox::indicator:disabled, QRadioButton::indicator:disabled, QGroupBox::indicator:disabled {
    background-color: @window@;
    border-color: @hover@;
}
QCheckBox::indicator:checked:disabled, QRadioButton::indicator:checked:disabled,
QGroupBox::indicator:checked:disabled {
    background-color: @border@;
}

/* ---------- group boxes ---------- */
QGroupBox {
    background-color: @panel@;
    border: 1px solid @border@;
    border-radius: 10px;
    margin-top: 14px;
    padding: 14px 8px 8px 8px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 12px;
    padding: 0 6px;
    color: @glow@;
    background-color: transparent;
}

/* ---------- tabs ---------- */
QTabWidget::pane {
    background-color: @window@;
    border: 1px solid @border@;
    border-radius: 8px;
    top: -1px;
}
QTabBar {
    qproperty-drawBase: 0;
}
QTabBar::tab {
    background: transparent;
    color: @muted@;
    border: none;
    border-bottom: 2px solid transparent;
    padding: 8px 16px;
    margin-right: 2px;
}
QTabBar::tab:hover {
    color: @text@;
    background-color: @surface@;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
}
QTabBar::tab:selected {
    color: @glow@;
    border-bottom: 2px solid @glow@;
}
QTabBar::tab:disabled {
    color: @disabledIcon@;
}
QTabBar::close-button:hover {
    background-color: @hover@;
    border-radius: 4px;
}

/* ---------- item views ---------- */
QAbstractItemView {
    background-color: @base@;
    alternate-background-color: @panel@;
    border: 1px solid @border@;
    border-radius: 8px;
    outline: none;
}
QAbstractItemView::item {
    padding: 3px;
    border-radius: 6px;
}
QAbstractItemView::item:hover {
    background-color: @itemHover@;
}
QAbstractItemView::item:selected {
    background-color: @accent@;
    color: @onAccent@;
}
QAbstractItemView::item:selected:!active {
    background-color: @deep@;
}
/* tables and version lists: one continuous row per entry */
QTreeView::item, QTableView::item {
    border-radius: 0px;
    padding: 4px 6px;
    min-height: 20px;
}
QComboBox QAbstractItemView::item {
    min-height: 24px;
    padding: 2px 8px;
}
QTreeView::branch {
    background: transparent;
}
QTreeView::branch:has-children:!has-siblings:closed,
QTreeView::branch:closed:has-children:has-siblings {
    image: url(@arrows@arrow-right.svg);
}
QTreeView::branch:open:has-children:!has-siblings,
QTreeView::branch:open:has-children:has-siblings {
    image: url(@arrows@arrow-down.svg);
}
QHeaderView {
    background-color: @panel@;
    border: none;
}
QHeaderView::section {
    background-color: @panel@;
    color: @muted@;
    border: none;
    border-right: 1px solid @hover@;
    border-bottom: 1px solid @border@;
    padding: 6px 8px;
}
QHeaderView::section:hover {
    color: @glow@;
}
QTableCornerButton::section {
    background-color: @panel@;
    border: none;
}

/* ---------- scroll bars ---------- */
QScrollBar:vertical {
    background: transparent;
    width: 12px;
    margin: 2px;
}
QScrollBar:horizontal {
    background: transparent;
    height: 12px;
    margin: 2px;
}
QScrollBar::handle {
    background-color: @border@;
    border-radius: 4px;
}
QScrollBar::handle:vertical {
    min-height: 32px;
}
QScrollBar::handle:horizontal {
    min-width: 32px;
}
QScrollBar::handle:hover {
    background-color: @accent@;
}
QScrollBar::handle:pressed {
    background-color: @glow@;
}
QScrollBar::add-line, QScrollBar::sub-line {
    width: 0px;
    height: 0px;
    border: none;
    background: none;
}
QScrollBar::add-page, QScrollBar::sub-page {
    background: none;
}

/* ---------- progress & sliders ---------- */
QProgressBar {
    background-color: @base@;
    border: 1px solid @border@;
    border-radius: 7px;
    text-align: center;
    color: @text@;
    min-height: 14px;
}
QProgressBar::chunk {
    border-radius: 6px;
    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 @deep@, stop:0.6 @accent@, stop:1 @glow@);
}
QSlider::groove:horizontal {
    height: 4px;
    background: @border@;
    border-radius: 2px;
}
QSlider::sub-page:horizontal {
    background: @accent@;
    border-radius: 2px;
}
QSlider::handle:horizontal {
    background: @glow@;
    width: 16px;
    height: 16px;
    margin: -6px 0;
    border-radius: 8px;
}
QSlider::groove:vertical {
    width: 4px;
    background: @border@;
    border-radius: 2px;
}
QSlider::handle:vertical {
    background: @glow@;
    width: 16px;
    height: 16px;
    margin: 0 -6px;
    border-radius: 8px;
}

/* ---------- misc ---------- */
QStatusBar {
    background-color: @barGlass@;
    border-top: 1px solid @border@;
    color: @muted@;
}
QStatusBar::item {
    border: none;
}
QStatusBar QLabel {
    color: @muted@;
    padding: 0 4px;
}
QSplitter::handle {
    background-color: @border@;
}
QSplitter::handle:horizontal {
    width: 1px;
}
QSplitter::handle:vertical {
    height: 1px;
}
QSplitter::handle:hover {
    background-color: @glow@;
}
QDockWidget::title {
    background-color: @panel@;
    padding: 6px;
}
QLabel {
    background: transparent;
}
QLabel:disabled {
    color: @disabled@;
}
QFrame[frameShape="4"], QFrame[frameShape="5"] {
    color: @border@;
}
QDialogButtonBox QPushButton {
    min-width: 80px;
}
)QSS");
    if (m_variant == Variant::Lunar) {
        // crimson gradients over the burgundy night: selections, the main buttons and progress
        qss += QStringLiteral(R"QSS(
QAbstractItemView::item:selected, QMenu::item:selected, QMenuBar::item:selected {
    background-color: @rainbowSoft@;
    color: #ffffff;
}
QAbstractItemView::item:selected:!active {
    background-color: @rainbowFaint@;
}
QToolBar#instanceToolBar QToolButton#primaryAction, QPushButton#primaryAction, QPushButton#accentButton, QPushButton:default {
    background-color: @rainbowLight@;
    color: #ffffff;
    border: 1px solid #ff8a98;
}
QToolBar#instanceToolBar QToolButton#primaryAction:hover, QPushButton#primaryAction:hover, QPushButton#accentButton:hover,
QPushButton:default:hover {
    background-color: #ff5a6e;
    color: #ffffff;
}
QPushButton:pressed, QPushButton:checked, QToolButton:pressed, QToolButton:checked, QPushButton#quickChip:pressed,
QToolBar QToolButton:pressed, QToolBar QToolButton:checked, QPushButton#loaderCard:checked {
    background-color: @rainbowSoft@;
    color: #ffffff;
    border-color: #ff8a98;
}
QProgressBar::chunk {
    background-color: @rainbowStrong@;
}
QTabBar::tab:selected {
    border-bottom: 2px solid @rainbowStrong@;
}
QSlider::sub-page:horizontal {
    background: @rainbowStrong@;
}
/* Vellora, made like Millida Launcher: buttons are raised blocks with a dark outline, a light top edge and a thick
   dark bottom edge; pressed, the bottom edge goes away as if the block was pushed in */
QPushButton, QToolBar QToolButton, QToolButton#quickChip {
    background-color: #2b0e15;
    color: #fbeff1;
    border-radius: 2px;
    border: 2px solid #060103;
    border-top: 2px solid #4a1a26;
    border-bottom: 5px solid #12040a;
    padding: 4px 12px 3px 12px;
    font-weight: bold;
}
QPushButton:hover, QToolBar QToolButton:hover {
    background-color: #3d121c;
    border-top-color: #6a2232;
}
QPushButton:pressed, QToolBar QToolButton:pressed {
    border-bottom: 2px solid #12040a;
    padding: 7px 12px 3px 12px;
}
QPushButton:disabled, QToolBar QToolButton:disabled {
    background-color: #1c080d;
    color: #8a5a62;
    border-bottom-color: #0b0205;
}
QToolBar#instanceToolBar QToolButton#primaryAction, QPushButton#primaryAction, QPushButton#accentButton, QPushButton:default {
    background-color: #e8364f;
    color: #ffffff;
    border: 2px solid #1e0408;
    border-top: 2px solid #ff7586;
    border-bottom: 5px solid #8e1428;
}
QToolBar#instanceToolBar QToolButton#primaryAction:hover, QPushButton#primaryAction:hover, QPushButton#accentButton:hover,
QPushButton:default:hover {
    background-color: #ff4a62;
    color: #ffffff;
}
QPushButton:checked, QToolBar QToolButton:checked {
    background-color: #e8364f;
    color: #ffffff;
    border-top-color: #ff7586;
    border-bottom: 5px solid #8e1428;
}
QLineEdit, QComboBox, QSpinBox, QPlainTextEdit, QTextEdit {
    border-radius: 2px;
    border: 2px solid #5a1e2a;
    background-color: #140509;
}
QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QPlainTextEdit:focus, QTextEdit:focus {
    border-color: #e8364f;
}
QTabBar::tab {
    border-radius: 2px;
    border: 2px solid #060103;
    border-bottom: 4px solid #12040a;
    background-color: #2b0e15;
    padding: 5px 14px;
    margin-right: 4px;
    font-weight: bold;
}
QTabBar::tab:selected {
    background-color: #e8364f;
    color: #ffffff;
    border-bottom: 4px solid #8e1428;
}
QMenu, QToolTip {
    border-radius: 2px;
    border: 2px solid #5a1e2a;
}
QProgressBar {
    border-radius: 2px;
    border: 2px solid #060103;
}
QProgressBar::chunk {
    border-radius: 0px;
}
)QSS");
    }
    const auto& c = colors();
    // the rainbow at different strengths: over dark backgrounds the soft one looks like light through glass
    auto rainbow = [](int alpha, bool pastel) {
        // Vellora: crimson shades instead of the old rainbow
        const QColor bands[] = { QColor(255, 90, 110), QColor(232, 54, 79), QColor(200, 31, 58), QColor(168, 22, 46),
                                 QColor(200, 31, 58), QColor(232, 54, 79) };
        QStringList stops;
        for (int i = 0; i < 6; i++) {
            QColor b = pastel ? QColor((bands[i].red() + 255 * 2) / 3, (bands[i].green() + 255 * 2) / 3, (bands[i].blue() + 255 * 2) / 3)
                              : bands[i];
            stops << QString("stop:%1 rgba(%2, %3, %4, %5)").arg(i / 5.0).arg(b.red()).arg(b.green()).arg(b.blue()).arg(alpha);
        }
        return QString("qlineargradient(x1:0, y1:0, x2:1, y2:0, %1)").arg(stops.join(", "));
    };
    const std::pair<const char*, QString> values[] = {
        { "@rainbowSoft@", rainbow(110, false) },
        { "@rainbowFaint@", rainbow(60, false) },
        { "@rainbowStrong@", rainbow(220, false) },
        { "@rainbowLight@", rainbow(255, false) },
        { "@text@", c.text },         { "@disabled@", c.disabled },         { "@window@", c.window },
        { "@border@", c.border },     { "@surface@", c.surface },           { "@bar@", c.bar },
        { "@hover@", c.hover },       { "@glow@", c.glow },                 { "@accent@", c.accent },
        { "@onAccent@", c.onAccent }, { "@panel@", c.panel },               { "@disabledIcon@", c.disabledIcon },
        { "@deep@", c.deep },         { "@base@", c.base },                 { "@muted@", c.muted },
        { "@itemHover@", c.itemHover }, { "@accentHover@", c.accentHover }, { "@chipText@", c.chipText },
        { "@windowTexture@", textureFile(QColor(c.window), QColor(c.glow)) },
        // bars and side panels are tinted glass over the sculk of the window, so the whole window is one surface
        { "@barGlass@", glass(c.bar, m_variant == Variant::Light ? 0.86 : m_variant == Variant::Lunar ? 0.72 : 0.80) },
        { "@panelGlass@", glass(c.panel, m_variant == Variant::Light ? 0.84 : m_variant == Variant::Lunar ? 0.66 : 0.74) },
        { "@arrows@", m_variant == Variant::Light ? QStringLiteral(":/sculk_theme/dark-") : QStringLiteral(":/sculk_theme/") },
    };
    for (const auto& [token, value] : values) {
        qss.replace(QLatin1String(token), value);
    }
    return qss;
}
