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

#include <QObject>

// Sculk palette
//   deep      #050f12  lists, editors, consoles
//   window    #0a1a1f  window background
//   surface   #0f252c  buttons, toolbars, alternating rows
//   hover     #143440  hovered controls
//   border    #1b4651  frames and separators
//   glow      #29dfeb  accent (sculk sensor tendrils / catalyst glow)
//   glow dim  #0e8f99  pressed accent, selection background
//   text      #d6f4f5  primary text
//   muted     #7fa3a8  secondary text

QString SculkTheme::id()
{
    return "sculk";
}

QString SculkTheme::name()
{
    return QObject::tr("Sculk (Dark)");
}

QString SculkTheme::tooltip()
{
    return QObject::tr("Modern dark theme in the colors of Minecraft sculk");
}

QPalette SculkTheme::colorScheme()
{
    QPalette p;
    p.setColor(QPalette::Window, QColor(0x0a, 0x1a, 0x1f));
    p.setColor(QPalette::WindowText, QColor(0xd6, 0xf4, 0xf5));
    p.setColor(QPalette::Base, QColor(0x05, 0x0f, 0x12));
    p.setColor(QPalette::AlternateBase, QColor(0x0b, 0x1f, 0x25));
    p.setColor(QPalette::ToolTipBase, QColor(0x0f, 0x25, 0x2c));
    p.setColor(QPalette::ToolTipText, QColor(0xd6, 0xf4, 0xf5));
    p.setColor(QPalette::Text, QColor(0xd6, 0xf4, 0xf5));
    p.setColor(QPalette::Button, QColor(0x0f, 0x25, 0x2c));
    p.setColor(QPalette::ButtonText, QColor(0xd6, 0xf4, 0xf5));
    p.setColor(QPalette::BrightText, QColor(0x29, 0xdf, 0xeb));
    p.setColor(QPalette::Link, QColor(0x29, 0xdf, 0xeb));
    p.setColor(QPalette::LinkVisited, QColor(0x1f, 0xb3, 0xbd));
    p.setColor(QPalette::Highlight, QColor(0x0e, 0x8f, 0x99));
    p.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
    p.setColor(QPalette::PlaceholderText, QColor(0x5a, 0x80, 0x86));
    p.setColor(QPalette::Light, QColor(0x1b, 0x46, 0x51));
    p.setColor(QPalette::Midlight, QColor(0x14, 0x34, 0x40));
    p.setColor(QPalette::Mid, QColor(0x0f, 0x25, 0x2c));
    p.setColor(QPalette::Dark, QColor(0x05, 0x0f, 0x12));
    p.setColor(QPalette::Shadow, QColor(0x00, 0x00, 0x00));
    return fadeInactive(p, fadeAmount(), fadeColor());
}

double SculkTheme::fadeAmount()
{
    return 0.55;
}

QColor SculkTheme::fadeColor()
{
    return QColor(0x0a, 0x1a, 0x1f);
}

LogColors SculkTheme::logColorScheme()
{
    LogColors result;
    result.background[MessageLevel::Fatal] = QColor(0x3a, 0x08, 0x10);

    result.foreground[MessageLevel::Launcher] = QColor(0x29, 0xdf, 0xeb);
    result.foreground[MessageLevel::Debug] = QColor(0x7f, 0xa3, 0xa8);
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
    return QStringLiteral(R"QSS(
/* ---------- base ---------- */
QWidget {
    color: #d6f4f5;
    selection-background-color: #0e8f99;
    selection-color: #ffffff;
}
QWidget:disabled {
    color: #4b6a70;
}
QMainWindow, QDialog, QWizard {
    background-color: #0a1a1f;
}
QMainWindow::separator {
    background: #1b4651;
    width: 1px;
    height: 1px;
}

/* ---------- tooltips ---------- */
QToolTip {
    color: #d6f4f5;
    background-color: #0f252c;
    border: 1px solid #29dfeb;
    border-radius: 6px;
    padding: 6px 8px;
}

/* ---------- menus ---------- */
QMenuBar {
    background-color: #071418;
    border-bottom: 1px solid #1b4651;
    padding: 2px 4px;
}
QMenuBar::item {
    background: transparent;
    padding: 6px 12px;
    border-radius: 6px;
}
QMenuBar::item:selected {
    background-color: #143440;
    color: #29dfeb;
}
QMenuBar::item:pressed {
    background-color: #0e8f99;
    color: #ffffff;
}
QMenu {
    background-color: #0b1f25;
    border: 1px solid #1b4651;
    border-radius: 8px;
    padding: 6px;
}
QMenu::item {
    padding: 7px 28px 7px 28px;
    border-radius: 6px;
    background: transparent;
}
QMenu::item:selected {
    background-color: #0e8f99;
    color: #ffffff;
}
QMenu::item:disabled {
    color: #4b6a70;
}
QMenu::separator {
    height: 1px;
    background: #1b4651;
    margin: 6px 8px;
}
QMenu::icon {
    padding-left: 8px;
}

/* ---------- tool bars ---------- */
QToolBar {
    background-color: #071418;
    border: none;
    spacing: 4px;
    padding: 4px;
}
QToolBar#mainToolBar {
    border-bottom: 1px solid #1b4651;
}
QToolBar#newsToolBar {
    border-top: 1px solid #1b4651;
}
QToolBar#instanceToolBar {
    background-color: #0b1f25;
    border-left: 1px solid #1b4651;
    padding: 8px 6px;
}
QToolBar::separator {
    background: #1b4651;
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
    background-color: #143440;
    border-color: #1b4651;
    color: #29dfeb;
}
QToolBar QToolButton:pressed, QToolBar QToolButton:checked {
    background-color: #0e8f99;
    border-color: #29dfeb;
    color: #ffffff;
}
QToolBar QToolButton:disabled {
    color: #3d6b72;
}

/* ---------- buttons ---------- */
QPushButton {
    background-color: #0f252c;
    border: 1px solid #1b4651;
    border-radius: 8px;
    padding: 7px 16px;
    min-height: 18px;
}
QPushButton:hover {
    background-color: #143440;
    border-color: #29dfeb;
    color: #29dfeb;
}
QPushButton:pressed, QPushButton:checked {
    background-color: #0e8f99;
    border-color: #29dfeb;
    color: #ffffff;
}
QPushButton:default {
    background-color: #0a5a63;
    border-color: #29dfeb;
    color: #ffffff;
}
QPushButton:default:hover {
    background-color: #0e8f99;
}
QPushButton:disabled {
    background-color: #0a1a1f;
    border-color: #143440;
    color: #3d6b72;
}
QPushButton:focus {
    outline: none;
}
QPushButton::menu-indicator {
    image: url(:/sculk_theme/arrow-down.svg);
    subcontrol-origin: padding;
    subcontrol-position: center right;
    width: 12px;
    right: 4px;
}
QToolButton {
    background-color: #0f252c;
    border: 1px solid #1b4651;
    border-radius: 8px;
    padding: 5px;
}
QToolButton:hover {
    background-color: #143440;
    border-color: #29dfeb;
}
QToolButton:pressed, QToolButton:checked {
    background-color: #0e8f99;
    border-color: #29dfeb;
}
QToolButton[popupMode="1"] {
    padding-right: 18px;
}
QToolButton::menu-button {
    border: none;
    border-left: 1px solid #1b4651;
    width: 16px;
}
QToolButton::menu-arrow, QToolButton::menu-indicator {
    image: url(:/sculk_theme/arrow-down.svg);
    width: 10px;
    height: 10px;
}
QToolButton::menu-indicator {
    subcontrol-position: bottom right;
}

/* ---------- inputs ---------- */
QLineEdit, QPlainTextEdit, QTextEdit, QTextBrowser, QAbstractSpinBox, QKeySequenceEdit {
    background-color: #050f12;
    border: 1px solid #1b4651;
    border-radius: 8px;
    padding: 5px 8px;
    selection-background-color: #0e8f99;
}
QPlainTextEdit, QTextEdit, QTextBrowser {
    padding: 4px;
}
QLineEdit:hover, QPlainTextEdit:hover, QTextEdit:hover, QAbstractSpinBox:hover {
    border-color: #0e8f99;
}
QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus, QAbstractSpinBox:focus {
    border: 1px solid #29dfeb;
}
QLineEdit:disabled, QAbstractSpinBox:disabled, QPlainTextEdit:disabled, QTextEdit:disabled {
    background-color: #0a1a1f;
    border-color: #143440;
}
QAbstractSpinBox::up-button, QAbstractSpinBox::down-button {
    subcontrol-origin: padding;
    width: 20px;
    border: none;
    border-left: 1px solid #1b4651;
    background: #0f252c;
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
    background: #143440;
}
QAbstractSpinBox::up-arrow {
    image: url(:/sculk_theme/arrow-up.svg);
    width: 10px;
    height: 10px;
}
QAbstractSpinBox::down-arrow {
    image: url(:/sculk_theme/arrow-down.svg);
    width: 10px;
    height: 10px;
}
QAbstractSpinBox::up-arrow:disabled, QAbstractSpinBox::up-arrow:off {
    image: url(:/sculk_theme/arrow-up-disabled.svg);
}
QAbstractSpinBox::down-arrow:disabled, QAbstractSpinBox::down-arrow:off {
    image: url(:/sculk_theme/arrow-down-disabled.svg);
}

QComboBox {
    background-color: #0f252c;
    border: 1px solid #1b4651;
    border-radius: 8px;
    padding: 5px 30px 5px 10px;
    min-height: 18px;
}
QComboBox:editable {
    background-color: #050f12;
}
QComboBox:hover {
    border-color: #0e8f99;
}
QComboBox:focus, QComboBox:on {
    border-color: #29dfeb;
}
QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: center right;
    width: 26px;
    border: none;
}
QComboBox::down-arrow {
    image: url(:/sculk_theme/arrow-down.svg);
    width: 12px;
    height: 12px;
}
QComboBox::down-arrow:disabled {
    image: url(:/sculk_theme/arrow-down-disabled.svg);
}
QComboBox QAbstractItemView {
    background-color: #0b1f25;
    border: 1px solid #1b4651;
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
    border: 1px solid #1b4651;
    background-color: #050f12;
}
QCheckBox::indicator, QGroupBox::indicator, QAbstractItemView::indicator {
    border-radius: 4px;
}
QRadioButton::indicator {
    border-radius: 9px;
}
QCheckBox::indicator:hover, QRadioButton::indicator:hover, QGroupBox::indicator:hover,
QAbstractItemView::indicator:hover {
    border-color: #29dfeb;
}
QCheckBox::indicator:checked, QGroupBox::indicator:checked, QAbstractItemView::indicator:checked {
    background-color: #29dfeb;
    border-color: #29dfeb;
    image: url(:/sculk_theme/check.svg);
}
QCheckBox::indicator:indeterminate, QAbstractItemView::indicator:indeterminate {
    background-color: #29dfeb;
    border-color: #29dfeb;
    image: url(:/sculk_theme/partial.svg);
}
QRadioButton::indicator:checked {
    background-color: #29dfeb;
    border-color: #29dfeb;
    image: url(:/sculk_theme/radio.svg);
}
QCheckBox::indicator:disabled, QRadioButton::indicator:disabled, QGroupBox::indicator:disabled {
    background-color: #0a1a1f;
    border-color: #143440;
}
QCheckBox::indicator:checked:disabled, QRadioButton::indicator:checked:disabled,
QGroupBox::indicator:checked:disabled {
    background-color: #1b4651;
}

/* ---------- group boxes ---------- */
QGroupBox {
    background-color: #0b1f25;
    border: 1px solid #1b4651;
    border-radius: 10px;
    margin-top: 14px;
    padding: 14px 8px 8px 8px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 12px;
    padding: 0 6px;
    color: #29dfeb;
    background-color: transparent;
}

/* ---------- tabs ---------- */
QTabWidget::pane {
    background-color: #0a1a1f;
    border: 1px solid #1b4651;
    border-radius: 8px;
    top: -1px;
}
QTabBar {
    qproperty-drawBase: 0;
}
QTabBar::tab {
    background: transparent;
    color: #7fa3a8;
    border: none;
    border-bottom: 2px solid transparent;
    padding: 8px 16px;
    margin-right: 2px;
}
QTabBar::tab:hover {
    color: #d6f4f5;
    background-color: #0f252c;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
}
QTabBar::tab:selected {
    color: #29dfeb;
    border-bottom: 2px solid #29dfeb;
}
QTabBar::tab:disabled {
    color: #3d6b72;
}
QTabBar::close-button:hover {
    background-color: #143440;
    border-radius: 4px;
}

/* ---------- item views ---------- */
QAbstractItemView {
    background-color: #050f12;
    alternate-background-color: #0b1f25;
    border: 1px solid #1b4651;
    border-radius: 8px;
    outline: none;
}
QAbstractItemView::item {
    padding: 3px;
    border-radius: 6px;
}
QAbstractItemView::item:hover {
    background-color: #0f2b33;
}
QAbstractItemView::item:selected {
    background-color: #0e8f99;
    color: #ffffff;
}
QAbstractItemView::item:selected:!active {
    background-color: #0a5a63;
}
QTreeView::branch {
    background: transparent;
}
QTreeView::branch:has-children:!has-siblings:closed,
QTreeView::branch:closed:has-children:has-siblings {
    image: url(:/sculk_theme/arrow-right.svg);
}
QTreeView::branch:open:has-children:!has-siblings,
QTreeView::branch:open:has-children:has-siblings {
    image: url(:/sculk_theme/arrow-down.svg);
}
QHeaderView {
    background-color: #0b1f25;
    border: none;
}
QHeaderView::section {
    background-color: #0b1f25;
    color: #7fa3a8;
    border: none;
    border-right: 1px solid #143440;
    border-bottom: 1px solid #1b4651;
    padding: 6px 8px;
}
QHeaderView::section:hover {
    color: #29dfeb;
}
QTableCornerButton::section {
    background-color: #0b1f25;
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
    background-color: #1b4651;
    border-radius: 4px;
}
QScrollBar::handle:vertical {
    min-height: 32px;
}
QScrollBar::handle:horizontal {
    min-width: 32px;
}
QScrollBar::handle:hover {
    background-color: #0e8f99;
}
QScrollBar::handle:pressed {
    background-color: #29dfeb;
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
    background-color: #050f12;
    border: 1px solid #1b4651;
    border-radius: 7px;
    text-align: center;
    color: #d6f4f5;
    min-height: 14px;
}
QProgressBar::chunk {
    border-radius: 6px;
    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #0a5a63, stop:0.6 #0e8f99, stop:1 #29dfeb);
}
QSlider::groove:horizontal {
    height: 4px;
    background: #1b4651;
    border-radius: 2px;
}
QSlider::sub-page:horizontal {
    background: #0e8f99;
    border-radius: 2px;
}
QSlider::handle:horizontal {
    background: #29dfeb;
    width: 16px;
    height: 16px;
    margin: -6px 0;
    border-radius: 8px;
}
QSlider::groove:vertical {
    width: 4px;
    background: #1b4651;
    border-radius: 2px;
}
QSlider::handle:vertical {
    background: #29dfeb;
    width: 16px;
    height: 16px;
    margin: 0 -6px;
    border-radius: 8px;
}

/* ---------- misc ---------- */
QStatusBar {
    background-color: #071418;
    border-top: 1px solid #1b4651;
    color: #7fa3a8;
}
QStatusBar::item {
    border: none;
}
QStatusBar QLabel {
    color: #7fa3a8;
    padding: 0 4px;
}
QSplitter::handle {
    background-color: #1b4651;
}
QSplitter::handle:horizontal {
    width: 1px;
}
QSplitter::handle:vertical {
    height: 1px;
}
QSplitter::handle:hover {
    background-color: #29dfeb;
}
QDockWidget::title {
    background-color: #0b1f25;
    padding: 6px;
}
QLabel {
    background: transparent;
}
QLabel:disabled {
    color: #4b6a70;
}
QFrame[frameShape="4"], QFrame[frameShape="5"] {
    color: #1b4651;
}
QDialogButtonBox QPushButton {
    min-width: 80px;
}
)QSS");
}
