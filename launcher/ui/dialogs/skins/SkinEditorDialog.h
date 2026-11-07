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

#include <QDialog>
#include <QImage>

#include "minecraft/skins/SkinModel.h"
#include "ui/dialogs/skins/draw/SkinOpenGLWindow.h"

class PixelCanvas;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QToolButton;

/// Paints a skin or a cape: a pixel canvas that shows every part of the body, a palette,
/// generators for ready-made looks and a live 3D preview.
class SkinEditorDialog : public QDialog, public SkinProvider {
    Q_OBJECT
   public:
    enum class Mode { Skin, Cape };

    /// start: the texture to begin with (empty: a generated one); previewSkin: in cape mode, the skin the cape is shown on
    SkinEditorDialog(QWidget* parent, Mode mode, const QImage& start, bool slim, const QImage& previewSkin, const QString& name);
    ~SkinEditorDialog() override;

    QImage image() const;
    QString name() const;
    bool slim() const;

    SkinModel* getSelectedSkin() override { return &m_previewSkin; }
    QHash<QString, QImage> capes() override;

   public slots:
    void accept() override;

   private:
    void generate(bool newSeed);
    void updatePreview();
    void setColor(const QColor& color);
    void updateUndoButtons();

    Mode m_mode;
    quint32 m_seed;
    bool m_generating = false;  // the model is being switched to match Steve or Alex
    PixelCanvas* m_canvas = nullptr;
    SkinModel m_previewSkin;
    SkinOpenGLWindow* m_preview3d = nullptr;
    QLabel* m_preview2d = nullptr;

    QComboBox* m_style = nullptr;
    QRadioButton* m_classic = nullptr;
    QRadioButton* m_slim = nullptr;
    QButtonGroup* m_tools = nullptr;
    QToolButton* m_colorButton = nullptr;
    QColor m_color;
    QCheckBox* m_mirror = nullptr;
    QCheckBox* m_elytra = nullptr;
    QComboBox* m_layer = nullptr;
    QPushButton* m_undo = nullptr;
    QPushButton* m_redo = nullptr;
    QLabel* m_hover = nullptr;
    QLineEdit* m_name = nullptr;
};
