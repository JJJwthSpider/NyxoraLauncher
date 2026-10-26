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
#include "QuickPage.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "Application.h"
#include "meta/Index.h"
#include "minecraft/VanillaInstanceCreationTask.h"
#include "ui/dialogs/NewInstanceDialog.h"

QuickPage::QuickPage(NewInstanceDialog* dialog, QWidget* parent) : QWidget(parent), m_dialog(dialog)
{
    m_loaders = {
        { "", "", "" },
        { "neoforge", "net.neoforged", "NeoForge" },
        { "fabric", "net.fabricmc.fabric-loader", "Fabric" },
        { "forge", "net.minecraftforge", "Forge" },
        { "quilt", "org.quiltmc.quilt-loader", "Quilt" },
    };

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    m_title = new QLabel(this);
    auto titleFont = m_title->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.25);
    titleFont.setBold(true);
    m_title->setFont(titleFont);
    layout->addWidget(m_title);

    // step 1: Minecraft version
    m_versionLabel = new QLabel(this);
    layout->addWidget(m_versionLabel);
    auto* versionRow = new QHBoxLayout;
    m_versionBox = new QComboBox(this);
    m_versionBox->setMinimumHeight(34);
    m_versionBox->setMaxVisibleItems(15);
    versionRow->addWidget(m_versionBox, 1);
    m_snapshots = new QCheckBox(this);
    versionRow->addWidget(m_snapshots);
    layout->addLayout(versionRow);

    // step 2: loader as big buttons, one click instead of a list
    m_loaderLabel = new QLabel(this);
    layout->addWidget(m_loaderLabel);
    auto* loaderRow = new QHBoxLayout;
    m_loaderGroup = new QButtonGroup(this);
    m_loaderGroup->setExclusive(true);
    for (int i = 0; i < m_loaders.size(); i++) {
        auto* button = new QPushButton(this);
        button->setObjectName("loaderCard");
        button->setCheckable(true);
        button->setMinimumHeight(44);
        button->setCursor(Qt::PointingHandCursor);
        m_loaderGroup->addButton(button, i);
        loaderRow->addWidget(button);
        m_loaderButtons << button;
    }
    m_loaderButtons.first()->setChecked(true);
    layout->addLayout(loaderRow);
    m_loaderDescription = new QLabel(this);
    m_loaderDescription->setWordWrap(true);
    m_loaderDescription->setObjectName("hintLabel");
    layout->addWidget(m_loaderDescription);

    // step 3: loader version, pre-selected
    m_loaderVersionLabel = new QLabel(this);
    layout->addWidget(m_loaderVersionLabel);
    m_loaderVersionBox = new QComboBox(this);
    m_loaderVersionBox->setMinimumHeight(34);
    m_loaderVersionBox->setMaxVisibleItems(15);
    layout->addWidget(m_loaderVersionBox);

    m_status = new QLabel(this);
    m_status->setWordWrap(true);
    m_status->setTextFormat(Qt::RichText);
    layout->addWidget(m_status);

    layout->addStretch(1);

    m_tip = new QLabel(this);
    m_tip->setWordWrap(true);
    m_tip->setObjectName("hintLabel");
    layout->addWidget(m_tip);

    connect(m_versionBox, &QComboBox::currentIndexChanged, this, [this] { loadLoaderVersions(); });
    connect(m_snapshots, &QCheckBox::toggled, this, [this] { fillMinecraftVersions(); });
    connect(m_loaderGroup, &QButtonGroup::idToggled, this, [this](int, bool checked) {
        if (checked) {
            updateLoaderDescription();
            loadLoaderVersions();
        }
    });
    connect(m_loaderVersionBox, &QComboBox::currentIndexChanged, this, [this] { suggestCurrent(); });

    retranslate();
}

QuickPage::~QuickPage() = default;

void QuickPage::retranslate()
{
    m_title->setText(tr("Create an instance in two clicks"));
    m_versionLabel->setText(tr("1. Minecraft version"));
    m_snapshots->setText(tr("Show snapshots"));
    m_loaderLabel->setText(tr("2. Mod loader"));
    m_loaderButtons.at(0)->setText(tr("No mods"));
    for (int i = 1; i < m_loaders.size(); i++) {
        m_loaderButtons.at(i)->setText(m_loaders.at(i).name);
    }
    m_loaderVersionLabel->setText(tr("3. Loader version (the recommended one is already selected)"));
    m_tip->setText(
        tr("Want a ready pack with mods? Open the \"AI pack\" page and describe it in your own words, "
           "for example \"zombie pack neoforge 1.21.1\"."));
    updateLoaderDescription();
}

const QuickPage::LoaderInfo& QuickPage::currentLoader() const
{
    auto id = m_loaderGroup->checkedId();
    return m_loaders.at(id < 0 ? 0 : id);
}

void QuickPage::updateLoaderDescription()
{
    const auto key = currentLoader().key;
    QString text;
    if (key.isEmpty()) {
        text = tr("Plain Minecraft without mods. You can add a loader later in the instance settings.");
    } else if (key == "neoforge") {
        text =
            tr("NeoForge is the modern successor of Forge. Most big content mods (technology, magic, adventure) "
               "for Minecraft 1.20.2 and newer are made for it.");
    } else if (key == "fabric") {
        text = tr("Fabric is light and quick to update. Best choice for performance mods, small tweaks and new Minecraft versions.");
    } else if (key == "forge") {
        text = tr("Forge is the classic loader. Choose it for Minecraft 1.20.1 and older, where most big mods live.");
    } else if (key == "quilt") {
        text = tr("Quilt is a fork of Fabric that also runs most Fabric mods.");
    }
    m_loaderDescription->setText(text);
    m_loaderVersionLabel->setVisible(!key.isEmpty());
    m_loaderVersionBox->setVisible(!key.isEmpty());
}

void QuickPage::openedImpl()
{
    if (!m_minecraftList) {
        loadMinecraftVersions();
    }
    suggestCurrent();
}

void QuickPage::loadMinecraftVersions()
{
    m_minecraftList = APPLICATION->metadataIndex()->get("net.minecraft");
    if (m_minecraftList->isLoaded()) {
        fillMinecraftVersions();
        return;
    }
    m_status->setText(tr("Loading the list of Minecraft versions..."));
    m_minecraftTask = m_minecraftList->getLoadTask();
    connect(m_minecraftTask.get(), &Task::succeeded, this, [this] {
        m_minecraftTask.reset();
        fillMinecraftVersions();
    });
    connect(m_minecraftTask.get(), &Task::failed, this, [this](const QString& reason) {
        m_minecraftTask.reset();
        m_status->setText(tr("<b>Could not load Minecraft versions:</b> %1").arg(reason.toHtmlEscaped()));
    });
    m_minecraftTask->start();
}

void QuickPage::fillMinecraftVersions()
{
    if (!m_minecraftList) {
        return;
    }
    const auto previous = m_versionBox->currentText();
    auto versions = m_minecraftList->versions();
    std::stable_sort(versions.begin(), versions.end(),
                     [](const Meta::Version::Ptr& a, const Meta::Version::Ptr& b) { return a->rawTime() > b->rawTime(); });

    m_versionBox->blockSignals(true);
    m_versionBox->clear();
    m_minecraftVersions.clear();
    int select = -1;
    for (const auto& version : versions) {
        const auto type = version->type();
        const bool release = type == "release";
        if (!release && !(m_snapshots->isChecked() && type == "snapshot")) {
            continue;
        }
        if (select < 0 && release) {
            select = m_minecraftVersions.size();  // newest release by default
        }
        if (version->version() == previous) {
            select = m_minecraftVersions.size();
        }
        m_versionBox->addItem(release ? version->version() : tr("%1 (snapshot)").arg(version->version()));
        m_minecraftVersions << version;
    }
    m_versionBox->setCurrentIndex(std::max(select, 0));
    m_versionBox->blockSignals(false);
    loadLoaderVersions();
}

void QuickPage::loadLoaderVersions()
{
    m_loaderVersionBox->clear();
    m_loaderVersions.clear();
    if (m_loaderTask) {
        disconnect(m_loaderTask.get(), nullptr, this, nullptr);
        m_loaderTask.reset();
    }

    const auto& loader = currentLoader();
    if (loader.key.isEmpty()) {
        m_loaderList.reset();
        suggestCurrent();
        return;
    }

    m_loaderList = APPLICATION->metadataIndex()->get(loader.uid);
    if (m_loaderList->isLoaded()) {
        fillLoaderVersions();
        return;
    }
    m_status->setText(tr("Loading %1 versions...").arg(loader.name));
    m_dialog->setSuggestedPack();
    m_loaderTask = m_loaderList->getLoadTask();
    connect(m_loaderTask.get(), &Task::succeeded, this, [this] {
        m_loaderTask.reset();
        fillLoaderVersions();
    });
    connect(m_loaderTask.get(), &Task::failed, this, [this](const QString& reason) {
        m_loaderTask.reset();
        m_status->setText(tr("<b>Could not load loader versions:</b> %1").arg(reason.toHtmlEscaped()));
    });
    m_loaderTask->start();
}

void QuickPage::fillLoaderVersions()
{
    const auto& loader = currentLoader();
    const int mcIndex = m_versionBox->currentIndex();
    if (!m_loaderList || mcIndex < 0 || mcIndex >= m_minecraftVersions.size()) {
        suggestCurrent();
        return;
    }
    const auto minecraft = m_minecraftVersions.at(mcIndex)->version();
    // Fabric and Quilt loaders are not tied to a Minecraft version, Forge and NeoForge are
    const bool anyMinecraft = loader.key == "fabric" || loader.key == "quilt";

    auto versions = m_loaderList->versions();
    std::stable_sort(versions.begin(), versions.end(),
                     [](const Meta::Version::Ptr& a, const Meta::Version::Ptr& b) { return a->rawTime() > b->rawTime(); });

    m_loaderVersionBox->blockSignals(true);
    m_loaderVersionBox->clear();
    m_loaderVersions.clear();
    int select = -1;
    for (const auto& version : versions) {
        if (!anyMinecraft) {
            bool matches = false;
            for (const auto& req : version->requiredSet()) {
                if (req.uid == "net.minecraft" && req.equalsVersion == minecraft) {
                    matches = true;
                    break;
                }
            }
            if (!matches) {
                continue;
            }
        }
        if (version->isRecommended() && select < 0) {
            select = m_loaderVersions.size();
        }
        m_loaderVersionBox->addItem(version->isRecommended() ? tr("%1 (recommended)").arg(version->version()) : version->version());
        m_loaderVersions << version;
    }
    m_loaderVersionBox->setCurrentIndex(m_loaderVersions.isEmpty() ? -1 : std::max(select, 0));
    m_loaderVersionBox->blockSignals(false);
    suggestCurrent();
}

void QuickPage::suggestCurrent()
{
    if (!isOpened) {
        return;
    }
    const int mcIndex = m_versionBox->currentIndex();
    if (mcIndex < 0 || mcIndex >= m_minecraftVersions.size()) {
        m_dialog->setSuggestedPack();
        return;
    }
    const auto minecraft = m_minecraftVersions.at(mcIndex);
    const auto& loader = currentLoader();

    if (loader.key.isEmpty()) {
        m_status->setText(tr("Ready: <b>Minecraft %1</b> without mods.").arg(minecraft->version()));
        m_dialog->setSuggestedPack(minecraft->version(), new VanillaCreationTask(minecraft));
        m_dialog->setSuggestedIcon("default");
        return;
    }

    const int loaderIndex = m_loaderVersionBox->currentIndex();
    if (m_loaderTask) {
        return;  // still loading, the status already says so
    }
    if (loaderIndex < 0 || loaderIndex >= m_loaderVersions.size()) {
        m_status->setText(
            tr("<b>%1 is not available for Minecraft %2.</b> Pick another version or loader.").arg(loader.name, minecraft->version()));
        m_dialog->setSuggestedPack();
        return;
    }
    const auto loaderVersion = m_loaderVersions.at(loaderIndex);
    m_status->setText(tr("Ready: <b>Minecraft %1</b> with <b>%2 %3</b>. Press OK to create the instance.")
                          .arg(minecraft->version(), loader.name, loaderVersion->version()));
    m_dialog->setSuggestedPack(QString("%1 %2").arg(minecraft->version(), loader.name),
                               new VanillaCreationTask(minecraft, loader.uid, loaderVersion));
    m_dialog->setSuggestedIcon("default");
}
