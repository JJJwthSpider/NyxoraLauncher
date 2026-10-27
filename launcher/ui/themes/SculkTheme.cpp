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

QPixmap SculkTheme::texture(const QColor& background, const QColor& glow, int pixelSize)
{
    static QHash<QString, QPixmap> cache;
    const QString key = background.name() + glow.name() + QString::number(pixelSize);
    if (auto it = cache.constFind(key); it != cache.constEnd()) {
        return *it;
    }

    // like the sculk block: blotches of slightly darker and lighter skin, crossed by veins that end in glowing dots
    constexpr int grid = 32;
    const bool lightBackground = background.lightnessF() > 0.6;
    const QColor shadow = lightBackground ? background.darker(108) : background.darker(118);
    const QColor shine = lightBackground ? background.darker(103) : background.lighter(112);
    const QColor vein = mix(background, glow, lightBackground ? 0.22 : 0.30);
    const QColor soul = mix(background, glow, lightBackground ? 0.45 : 0.62);

    QImage image(grid, grid, QImage::Format_RGB32);
    for (int y = 0; y < grid; y++) {
        for (int x = 0; x < grid; x++) {
            // blotches: the noise of 2x2 blocks with a bit of per-pixel noise on top, wrapped so the tile is seamless
            const quint32 blotch = cellHash(x / 2, y / 2, 1) % 100;
            const quint32 grain = cellHash(x, y, 2) % 100;
            QColor c = background;
            if (blotch < 30) {
                c = shadow;
            } else if (blotch > 78) {
                c = shine;
            }
            if (grain < 10) {
                c = c.darker(lightBackground ? 104 : 110);
            }
            image.setPixel(x, y, c.rgb());
        }
    }
    // veins: short random walks that wrap around the edges of the tile
    for (int v = 0; v < 14; v++) {
        int x = int(cellHash(v, 0, 3) % grid);
        int y = int(cellHash(v, 0, 4) % grid);
        const int length = 4 + int(cellHash(v, 0, 5) % 5);
        for (int step = 0; step < length; step++) {
            image.setPixel(x, y, (step == length - 1 ? soul : vein).rgb());
            const quint32 dir = cellHash(v, step, 6) % 4;
            x = (x + (dir == 0 ? 1 : dir == 1 ? grid - 1 : 0)) % grid;
            y = (y + (dir == 2 ? 1 : dir == 3 ? grid - 1 : 0)) % grid;
        }
    }

    const QPixmap pixmap = QPixmap::fromImage(image.scaled(grid * pixelSize, grid * pixelSize, Qt::IgnoreAspectRatio, Qt::FastTransformation));
    cache.insert(key, pixmap);
    return pixmap;
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
    switch (m_variant) {
        case Variant::Dark:
            return s_dark;
        case Variant::Light:
            return s_light;
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
        default:
            return QObject::tr("Sculk");
    }
}

QString SculkTheme::tooltip()
{
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
    background-color: @bar@;
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
    background-color: @bar@;
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
    background-color: @panel@;
    border-left: 1px solid @border@;
    padding: 8px 6px;
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
    background-color: @bar@;
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
    const auto& c = colors();
    const std::pair<const char*, QString> values[] = {
        { "@text@", c.text },         { "@disabled@", c.disabled },         { "@window@", c.window },
        { "@border@", c.border },     { "@surface@", c.surface },           { "@bar@", c.bar },
        { "@hover@", c.hover },       { "@glow@", c.glow },                 { "@accent@", c.accent },
        { "@onAccent@", c.onAccent }, { "@panel@", c.panel },               { "@disabledIcon@", c.disabledIcon },
        { "@deep@", c.deep },         { "@base@", c.base },                 { "@muted@", c.muted },
        { "@itemHover@", c.itemHover }, { "@accentHover@", c.accentHover }, { "@chipText@", c.chipText },
        { "@arrows@", m_variant == Variant::Light ? QStringLiteral(":/sculk_theme/dark-") : QStringLiteral(":/sculk_theme/") },
    };
    for (const auto& [token, value] : values) {
        qss.replace(QLatin1String(token), value);
    }
    return qss;
}
