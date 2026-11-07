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

#include "SkinEditorDialog.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QShortcut>
#include <QToolButton>
#include <QVBoxLayout>
#include <iterator>

#include "minecraft/skins/SkinArt.h"
#include "ui/dialogs/skins/PixelCanvas.h"

namespace {

const QString s_editorCape = QStringLiteral("editor");

// colors that skins use a lot: skin tones, hair, clothes, metals and the sculk glow
const QRgb s_palette[] = {
    0xfff3d2b3, 0xffe0ac8a, 0xffc68a62, 0xff9c633e, 0xff6e432a, 0xff2b1d14, 0xff5a3a22, 0xffb8863b,
    0xffe8d38c, 0xff8c2f1c, 0xffffffff, 0xffb0b0b5, 0xff6d6d72, 0xff2a2a2e, 0xff000000, 0xffb01e1e,
    0xfff26a1b, 0xffffd23a, 0xff3a6b35, 0xff4f8a3b, 0xff2f9ea8, 0xff2f4a7a, 0xff4b2a7a, 0xff29dfeb,
};

QIcon swatch(const QColor& color, int size = 18)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setPen(QColor(0, 0, 0, 120));
    p.setBrush(color);
    p.drawRoundedRect(QRectF(0.5, 0.5, size - 1, size - 1), 3, 3);
    return QIcon(pixmap);
}

/// the outside of a cape and both elytra wings, for the preview without OpenGL
QPixmap flatCapePreview(const QImage& cape)
{
    QImage combined(36, 20, QImage::Format_ARGB32);
    combined.fill(Qt::transparent);
    QPainter p(&combined);
    p.drawImage(0, 2, cape.copy(1, 1, 10, 16));
    p.drawImage(14, 0, cape.copy(24, 2, 10, 20));
    p.drawImage(25, 0, cape.copy(24, 2, 10, 20).mirrored(true, false));
    p.end();
    return QPixmap::fromImage(combined.scaled(36 * 7, 20 * 7, Qt::KeepAspectRatio, Qt::FastTransformation));
}

}  // namespace

SkinEditorDialog::SkinEditorDialog(QWidget* parent, Mode mode, const QImage& start, bool slim, const QImage& previewSkin, const QString& name)
    : QDialog(parent), m_mode(mode), m_seed(QRandomGenerator::global()->generate())
{
    const bool cape = mode == Mode::Cape;
    setWindowTitle(cape ? tr("Cape editor") : tr("Skin editor"));
    resize(1180, 760);

    auto* layout = new QVBoxLayout(this);

    // generators: a ready-made look to start from
    auto* top = new QHBoxLayout;
    top->addWidget(new QLabel(cape ? tr("Pattern:") : tr("Look:"), this));
    m_style = new QComboBox(this);
    m_style->addItems(cape ? SkinArt::capeStyles() : SkinArt::skinStyles());
    m_style->setToolTip(cape ? tr("A pattern for the cape and the elytra; you can paint over it afterwards")
                             : tr("A ready-made character; you can paint over it afterwards"));
    top->addWidget(m_style);
    auto* create = new QPushButton(tr("Create"), this);
    create->setObjectName("primaryAction");
    create->setToolTip(tr("Replace the picture with the chosen look (can be undone)"));
    top->addWidget(create);
    auto* another = new QPushButton(tr("Another variant"), this);
    another->setToolTip(tr("The same look with other colors and details"));
    top->addWidget(another);
    auto* clear = new QPushButton(tr("Clear"), this);
    clear->setToolTip(tr("Start from an empty texture"));
    top->addWidget(clear);
    top->addStretch(1);
    if (!cape) {
        top->addWidget(new QLabel(tr("Model:"), this));
        m_classic = new QRadioButton(tr("Classic"), this);
        m_slim = new QRadioButton(tr("Slim"), this);
        (slim ? m_slim : m_classic)->setChecked(true);
        m_classic->setToolTip(tr("Arms 4 pixels wide, like Steve"));
        m_slim->setToolTip(tr("Arms 3 pixels wide, like Alex"));
        top->addWidget(m_classic);
        top->addWidget(m_slim);
    }
    layout->addLayout(top);

    auto* middle = new QHBoxLayout;

    // tools, colors and options on the left
    auto* side = new QVBoxLayout;
    auto* toolBox = new QGroupBox(tr("Tool"), this);
    auto* toolLayout = new QGridLayout(toolBox);
    m_tools = new QButtonGroup(this);
    struct ToolInfo {
        PixelCanvas::Tool tool;
        QString text;
        QString tip;
        QKeySequence key;
    };
    const QList<ToolInfo> tools{
        { PixelCanvas::Tool::Pencil, tr("Pencil"), tr("Paints with the color (P). Right button erases."), QKeySequence(Qt::Key_P) },
        { PixelCanvas::Tool::Eraser, tr("Eraser"), tr("Makes pixels transparent (E): useful on the 2nd layer"), QKeySequence(Qt::Key_E) },
        { PixelCanvas::Tool::Fill, tr("Fill"), tr("Fills an area of one color on one face (F). Right button clears it."), QKeySequence(Qt::Key_F) },
        { PixelCanvas::Tool::Picker, tr("Picker"), tr("Takes the color of a pixel (I)"), QKeySequence(Qt::Key_I) },
        { PixelCanvas::Tool::Shade, tr("Shade"), tr("Left button darkens, right button lightens (S): shadows and highlights"), QKeySequence(Qt::Key_S) },
    };
    for (int i = 0; i < tools.size(); i++) {
        auto* button = new QToolButton(toolBox);
        button->setText(tools[i].text);
        button->setToolTip(tools[i].tip);
        button->setCheckable(true);
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_tools->addButton(button, int(tools[i].tool));
        toolLayout->addWidget(button, i / 2, i % 2);
        auto* shortcut = new QShortcut(tools[i].key, this);
        connect(shortcut, &QShortcut::activated, button, &QToolButton::click);
    }
    m_tools->button(int(PixelCanvas::Tool::Pencil))->setChecked(true);
    side->addWidget(toolBox);

    auto* colorBox = new QGroupBox(tr("Color"), this);
    auto* colorLayout = new QGridLayout(colorBox);
    colorLayout->setSpacing(3);
    m_colorButton = new QToolButton(colorBox);
    m_colorButton->setText(tr("Other color..."));
    m_colorButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_colorButton->setIconSize(QSize(22, 22));
    m_colorButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    colorLayout->addWidget(m_colorButton, 0, 0, 1, 6);
    for (int i = 0; i < int(std::size(s_palette)); i++) {
        const QColor color = QColor::fromRgba(s_palette[i]);
        auto* button = new QToolButton(colorBox);
        button->setIcon(swatch(color));
        button->setIconSize(QSize(18, 18));
        button->setAutoRaise(true);
        button->setToolTip(color.name());
        connect(button, &QToolButton::clicked, this, [this, color] { setColor(color); });
        colorLayout->addWidget(button, 1 + i / 6, i % 6);
    }
    connect(m_colorButton, &QToolButton::clicked, this, [this] {
        const auto color = QColorDialog::getColor(m_color, this, tr("Choose a color"));
        if (color.isValid()) {
            setColor(color);
        }
    });
    side->addWidget(colorBox);

    auto* optionsBox = new QGroupBox(tr("Options"), this);
    auto* optionsLayout = new QVBoxLayout(optionsBox);
    m_mirror = new QCheckBox(cape ? tr("Symmetric") : tr("Mirror left and right"), optionsBox);
    m_mirror->setToolTip(cape ? tr("Paints both halves of the cape at once")
                              : tr("Paints the other arm or leg and the other half of the head and body at the same time"));
    optionsLayout->addWidget(m_mirror);
    auto* grid = new QCheckBox(tr("Grid"), optionsBox);
    grid->setChecked(true);
    optionsLayout->addWidget(grid);
    if (!cape) {
        m_layer = new QComboBox(optionsBox);
        m_layer->addItems({ tr("Both layers"), tr("Highlight 1st layer"), tr("Highlight 2nd layer") });
        m_layer->setToolTip(tr("The 2nd layer (hat, jacket, sleeves) lies over the body and may be transparent"));
        optionsLayout->addWidget(m_layer);
    } else {
        m_elytra = new QCheckBox(tr("Show elytra"), optionsBox);
        optionsLayout->addWidget(m_elytra);
    }
    side->addWidget(optionsBox);

    auto* history = new QHBoxLayout;
    m_undo = new QPushButton(tr("Undo"), this);
    m_undo->setToolTip(tr("Ctrl+Z"));
    m_redo = new QPushButton(tr("Redo"), this);
    m_redo->setToolTip(tr("Ctrl+Y"));
    history->addWidget(m_undo);
    history->addWidget(m_redo);
    side->addLayout(history);
    side->addStretch(1);
    middle->addLayout(side);

    // the canvas in the middle, with the name of the part under the mouse
    auto* center = new QVBoxLayout;
    m_canvas = new PixelCanvas(this);
    m_canvas->setBoxes(cape ? SkinArt::capeBoxes() : SkinArt::skinBoxes(slim));
    center->addWidget(m_canvas, 1);
    m_hover = new QLabel(this);
    m_hover->setObjectName("hintLabel");
    m_hover->setMinimumHeight(m_hover->fontMetrics().height() + 4);
    center->addWidget(m_hover);
    middle->addLayout(center, 1);

    // the 3D preview on the right
    auto* right = new QVBoxLayout;
    m_previewSkin.setTexture(cape ? (previewSkin.isNull() ? SkinArt::generateSkin(0, m_seed, false) : previewSkin) : SkinArt::blankSkin());
    m_previewSkin.setCapeId(cape ? s_editorCape : QString());
    m_previewSkin.setModel(slim ? SkinModel::SLIM : SkinModel::CLASSIC);
    if (SkinOpenGLWindow::hasOpenGL()) {
        m_preview3d = new SkinOpenGLWindow(this, palette().color(QPalette::Normal, QPalette::Base));
        if (cape) {
            m_preview3d->setYaw(270);  // the cape hangs on the back
        }
        auto* container = QWidget::createWindowContainer(m_preview3d, this);
        container->setMinimumSize(300, 360);
        right->addWidget(container, 1);
    } else {
        m_preview2d = new QLabel(this);
        m_preview2d->setMinimumSize(300, 360);
        m_preview2d->setAlignment(Qt::AlignCenter);
        right->addWidget(m_preview2d, 1);
    }
    auto* rotate = new QLabel(m_preview3d ? tr("Drag to turn the model, scroll to zoom.") : QString(), this);
    rotate->setObjectName("hintLabel");
    rotate->setWordWrap(true);
    right->addWidget(rotate);
    middle->addLayout(right);
    layout->addLayout(middle, 1);

    // name, a note about capes and the buttons
    auto* bottom = new QHBoxLayout;
    bottom->addWidget(new QLabel(tr("Name:"), this));
    m_name = new QLineEdit(name, this);
    m_name->setMaximumWidth(260);
    bottom->addWidget(m_name);
    auto* note = new QLabel(this);
    note->setObjectName("hintLabel");
    note->setWordWrap(true);
    if (cape) {
        note->setText(tr("Mojang lets players wear only the capes they were given, so your own cape is shown here, in the "
                         "launcher. It is saved with your skins and you can pick it in the cape list to see it on any skin."));
    } else {
        note->setText(tr("Body parts must not be transparent, the 2nd layer may be. After saving, choose the skin and press OK "
                         "to wear it in the game."));
    }
    bottom->addWidget(note, 1);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setText(tr("Save"));
    buttons->button(QDialogButtonBox::Save)->setObjectName("primaryAction");
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    bottom->addWidget(buttons);
    layout->addLayout(bottom);

    connect(buttons, &QDialogButtonBox::accepted, this, &SkinEditorDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &SkinEditorDialog::reject);
    connect(m_tools, &QButtonGroup::idClicked, this, [this](int id) { m_canvas->setTool(PixelCanvas::Tool(id)); });
    connect(m_mirror, &QCheckBox::toggled, m_canvas, &PixelCanvas::setMirror);
    connect(grid, &QCheckBox::toggled, m_canvas, &PixelCanvas::setGridVisible);
    if (m_layer) {
        connect(m_layer, &QComboBox::currentIndexChanged, m_canvas, &PixelCanvas::setHighlightOverlay);
    }
    if (m_elytra) {
        connect(m_elytra, &QCheckBox::toggled, this, [this](bool on) {
            if (m_preview3d) {
                m_preview3d->setElytraVisible(on);
                m_preview3d->updateCape(image());
            } else {
                updatePreview();
            }
        });
    }
    connect(m_canvas, &PixelCanvas::changed, this, &SkinEditorDialog::updatePreview);
    connect(m_canvas, &PixelCanvas::changed, this, &SkinEditorDialog::updateUndoButtons);
    connect(m_canvas, &PixelCanvas::colorPicked, this, [this](const QColor& color) {
        setColor(color);
        // after picking a color the next stroke usually paints with it
        m_tools->button(int(PixelCanvas::Tool::Pencil))->click();
    });
    connect(m_canvas, &PixelCanvas::hovered, this, [this](const QString& text) {
        m_hover->setText(text.isEmpty() ? tr("Point at the texture to see which part of the body it is.") : text);
    });
    connect(m_undo, &QPushButton::clicked, m_canvas, &PixelCanvas::undo);
    connect(m_redo, &QPushButton::clicked, m_canvas, &PixelCanvas::redo);
    connect(new QShortcut(QKeySequence::Undo, this), &QShortcut::activated, m_canvas, &PixelCanvas::undo);
    connect(new QShortcut(QKeySequence::Redo, this), &QShortcut::activated, m_canvas, &PixelCanvas::redo);
    connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Y), this), &QShortcut::activated, m_canvas, &PixelCanvas::redo);
    connect(create, &QPushButton::clicked, this, [this] { generate(false); });
    connect(another, &QPushButton::clicked, this, [this] { generate(true); });
    // a cape starts over as a white sheet, a skin as an empty texture
    connect(clear, &QPushButton::clicked, this, [this, cape] { m_canvas->setImage(cape ? SkinArt::whiteCape() : SkinArt::blankSkin(), false); });
    if (!cape) {
        connect(m_slim, &QRadioButton::toggled, this, [this](bool on) {
            m_canvas->setBoxes(SkinArt::skinBoxes(on));
            m_previewSkin.setModel(on ? SkinModel::SLIM : SkinModel::CLASSIC);
            // an untouched Steve becomes Alex with the slim model and back
            if (!m_generating && !m_canvas->canUndo() && m_style->currentIndex() <= 1) {
                m_style->setCurrentIndex(on ? 1 : 0);
                m_canvas->setImage(SkinArt::generateSkin(on ? 1 : 0, m_seed, on), true);
            }
            updatePreview();
        });
    }

    setColor(cape ? QColor(0x29, 0xdf, 0xeb) : QColor(0x2f, 0x9e, 0xa8));
    m_hover->setText(tr("Point at the texture to see which part of the body it is."));
    if (start.isNull()) {
        // a fresh texture starts as a generated look, so there is something to paint on
        // a new skin starts as Steve (Alex for the slim model), a new cape as a white sheet
        m_style->setCurrentIndex(cape ? 0 : (slim ? 1 : 0));
        m_canvas->setImage(cape ? SkinArt::whiteCape() : SkinArt::generateSkin(slim ? 1 : 0, m_seed, slim), true);
    } else {
        QImage texture = start.convertToFormat(QImage::Format_ARGB32);
        if (!cape && texture.height() == 32) {
            // old 64x32 skins: the editor works on the modern layout
            SkinModel converted;
            converted.setTexture(texture);
            texture = converted.getTexture();
        }
        m_canvas->setImage(texture, true);
    }
    updateUndoButtons();
}

SkinEditorDialog::~SkinEditorDialog()
{
    delete m_preview3d;
}

QHash<QString, QImage> SkinEditorDialog::capes()
{
    if (m_mode == Mode::Cape) {
        return { { s_editorCape, image() } };
    }
    return {};
}

QImage SkinEditorDialog::image() const
{
    return m_canvas->image();
}

QString SkinEditorDialog::name() const
{
    return m_name->text().trimmed();
}

bool SkinEditorDialog::slim() const
{
    return m_slim && m_slim->isChecked();
}

void SkinEditorDialog::setColor(const QColor& color)
{
    m_color = color;
    m_canvas->setColor(color);
    m_colorButton->setIcon(swatch(color, 22));
    // a new color is meant to be painted with
    if (m_tools->checkedId() == int(PixelCanvas::Tool::Eraser)) {
        m_tools->button(int(PixelCanvas::Tool::Pencil))->click();
    }
}

void SkinEditorDialog::generate(bool newSeed)
{
    if (newSeed) {
        m_seed = QRandomGenerator::global()->generate();
    }
    const int style = m_style->currentIndex();
    if (m_mode == Mode::Skin && style <= 1) {
        // Steve wears the classic model, Alex the slim one
        m_generating = true;
        (style == 1 ? m_slim : m_classic)->setChecked(true);
        m_generating = false;
    }
    const auto texture = m_mode == Mode::Cape ? SkinArt::generateCape(style, m_seed) : SkinArt::generateSkin(style, m_seed, slim());
    m_canvas->setImage(texture, false);
}

void SkinEditorDialog::updateUndoButtons()
{
    m_undo->setEnabled(m_canvas->canUndo());
    m_redo->setEnabled(m_canvas->canRedo());
}

void SkinEditorDialog::updatePreview()
{
    if (m_mode == Mode::Skin) {
        m_previewSkin.setTexture(image());
        if (m_preview3d) {
            m_preview3d->updateScene(&m_previewSkin);
        } else {
            m_preview2d->setPixmap(QPixmap::fromImage(m_previewSkin.getPreview())
                                       .scaled(m_preview2d->size(), Qt::KeepAspectRatio, Qt::FastTransformation));
        }
        return;
    }
    if (m_preview3d) {
        m_preview3d->updateCape(image());
    } else {
        m_preview2d->setPixmap(flatCapePreview(image()));
    }
}

void SkinEditorDialog::accept()
{
    if (name().isEmpty()) {
        m_name->setFocus();
        QMessageBox::information(this, windowTitle(), tr("Give it a name first."));
        return;
    }
    static const QRegularExpression s_bad(QStringLiteral("[\\\\/:*?\"<>|]"));
    if (name().contains(s_bad)) {
        QMessageBox::information(this, windowTitle(), tr("The name must not contain \\ / : * ? \" < > |"));
        return;
    }
    QDialog::accept();
}
