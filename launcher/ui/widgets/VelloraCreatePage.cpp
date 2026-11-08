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
#include "VelloraCreatePage.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>
#include <QUrlQuery>
#include <QVBoxLayout>

#include "Application.h"
#include "InstanceImportTask.h"
#include "InstanceList.h"
#include "BaseInstance.h"
#include "meta/Index.h"
#include "minecraft/VanillaInstanceCreationTask.h"
#include "settings/SettingsObject.h"

namespace {
const char* MODRINTH = "https://api.modrinth.com/v2";

QLabel* caption(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setStyleSheet("color: #ff8090; font-weight: bold; background: transparent;");
    return label;
}

QLabel* heading(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setStyleSheet("color: #fbeff1; font-size: 16pt; font-weight: bold; background: transparent;");
    return label;
}

QFrame* panel(QWidget* parent)
{
    auto* frame = new QFrame(parent);
    frame->setObjectName("createPanel");
    frame->setStyleSheet(
        "QFrame#createPanel { background-color: rgba(20, 5, 9, 235); border: 2px solid #3a121b; border-bottom: 5px solid #0b0305; "
        "border-radius: 2px; }");
    return frame;
}

/// {major, minor, patch} of a release like 1.20.1; empty for snapshots
QList<int> parts(const QString& version)
{
    const auto p = version.split('.');
    if (p.size() < 2) {
        return {};
    }
    QList<int> out;
    for (int i = 0; i < 3; i++) {
        bool ok = true;
        out << (i < p.size() ? p.at(i).toInt(&ok) : 0);
        if (!ok) {
            return {};
        }
    }
    return out;
}

bool atLeast(const QList<int>& v, int minor, int patch)
{
    return v.at(0) > 1 || v.at(1) > minor || (v.at(1) == minor && v.at(2) >= patch);
}

QNetworkRequest request(const QUrl& url)
{
    QNetworkRequest r(url);
    r.setHeader(QNetworkRequest::UserAgentHeader, "Vellora-Client (github.com/nvsdjonvljw/Script)");
    return r;
}
}  // namespace

QIcon VelloraCreatePage::loaderSign(const QString& key)
{
    // the sign of a loader drawn in pixels: a cloth for Fabric, anvils for Forge and NeoForge, patches for Quilt, grass
    QStringList rows;
    QHash<QChar, QColor> colors;
    if (key == "fabric") {
        rows = { "..wwww..", ".wbwwbw.", "wwwbwwbw", "wbwwbwww", "wwbwwbww", ".wwbwwb.", "..wwww.." };
        colors = { { 'w', QColor(0xdb, 0xd0, 0xb4) }, { 'b', QColor(0x8f, 0x84, 0x66) } };
    } else if (key == "quilt") {
        rows = { "ppp.bbb", "ppp.bbb", "ppp.bbb", ".......", "ooo.mmm", "ooo.mmm", "ooo.mmm" };
        colors = { { 'p', QColor(0xb0, 0x7c, 0xe8) }, { 'b', QColor(0x4f, 0xa3, 0xe0) }, { 'o', QColor(0xf0, 0xa0, 0x40) }, { 'm', QColor(0xe8, 0x60, 0x8c) } };
    } else if (key == "vanilla") {
        rows = { "gggggggg", "gGgggGgg", "dgdddgdd", "dddddddd", "ddDddddd", "dddddDdd", "dDdddddd" };
        colors = { { 'g', QColor(0x6d, 0xbe, 0x45) }, { 'G', QColor(0x8f, 0xd4, 0x60) }, { 'd', QColor(0x8b, 0x5a, 0x2b) }, { 'D', QColor(0x6b, 0x43, 0x20) } };
    } else {
        rows = { "wwwwwwwww...", ".wwwwwwwwwww", "..lllllll...", "....www.....", "....www.....", "..wwwwwww...", ".wwwwwwwww.." };
        const bool neo = key == "neoforge";
        colors = { { 'w', neo ? QColor(0xe8, 0x74, 0x3c) : QColor(0x8a, 0x8f, 0x9c) }, { 'l', neo ? QColor(0xb0, 0x4e, 0x20) : QColor(0x5c, 0x61, 0x6c) } };
    }
    int cols = 0;
    for (const auto& row : rows) {
        cols = std::max(cols, int(row.size()));
    }
    const int u = 4;
    QPixmap pixmap(cols * u, rows.size() * u);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    for (int j = 0; j < rows.size(); j++) {
        for (int i = 0; i < rows.at(j).size(); i++) {
            const QChar c = rows.at(j).at(i);
            if (colors.contains(c)) {
                p.fillRect(i * u, j * u, u, u, colors.value(c));
            }
        }
    }
    p.end();
    return QIcon(pixmap);
}

VelloraCreatePage::VelloraCreatePage(QWidget* parent) : QWidget(parent)
{
    setObjectName("velloraCreate");
    m_loaders = {
        { "fabric", "net.fabricmc.fabric-loader", "Fabric", tr("light, fast") },
        { "neoforge", "net.neoforged", "NeoForge", tr("new mods") },
        { "forge", "net.minecraftforge", "Forge", tr("the classic") },
        { "quilt", "org.quiltmc.quilt-loader", "Quilt", tr("like Fabric") },
        { "vanilla", "", "Vanilla", tr("no mods") },
    };

    auto* outer = new QHBoxLayout(this);
    outer->setContentsMargins(12, 10, 12, 12);
    outer->setSpacing(12);

    /* ---------------- the new instance */
    auto* left = panel(this);
    auto* form = new QVBoxLayout(left);
    form->setContentsMargins(16, 12, 16, 14);
    form->setSpacing(8);
    form->addWidget(heading(tr("New instance"), left));
    form->addWidget(caption(tr("Name"), left));
    m_name = new QLineEdit(left);
    m_name->setMinimumHeight(34);
    form->addWidget(m_name);

    form->addWidget(caption(tr("Loader"), left));
    auto* loaders = new QHBoxLayout;
    loaders->setSpacing(6);
    m_loaderGroup = new QButtonGroup(this);
    m_loaderGroup->setExclusive(true);
    for (int i = 0; i < m_loaders.size(); i++) {
        const auto& loader = m_loaders.at(i);
        auto* tile = new QToolButton(left);
        tile->setObjectName("loaderTile");
        tile->setCheckable(true);
        tile->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        tile->setIcon(loaderSign(loader.key));
        tile->setIconSize(QSize(36, 28));
        tile->setMinimumSize(96, 78);
        tile->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        tile->setCursor(Qt::PointingHandCursor);
        tile->setText(loader.name + "\n" + loader.note);
        m_loaderGroup->addButton(tile, i);
        m_loaderButtons << tile;
        loaders->addWidget(tile);
    }
    m_loaderButtons.first()->setChecked(true);
    form->addLayout(loaders);
    connect(m_loaderGroup, &QButtonGroup::idClicked, this, [this](int id) {
        m_loader = m_loaders.at(id).key;
        updateLoaders();
    });

    auto* versionHead = new QHBoxLayout;
    versionHead->addWidget(caption(tr("Minecraft version"), left), 1);
    m_versionSearch = new QLineEdit(left);
    m_versionSearch->setPlaceholderText(tr("Find"));
    m_versionSearch->setClearButtonEnabled(true);
    m_versionSearch->setMaximumWidth(160);
    versionHead->addWidget(m_versionSearch);
    m_snapshots = new QCheckBox(tr("Snapshots and betas"), left);
    versionHead->addWidget(m_snapshots);
    form->addLayout(versionHead);
    m_versions = new QListWidget(left);
    m_versions->setObjectName("versionGrid");
    m_versions->setViewMode(QListView::IconMode);
    m_versions->setFlow(QListView::LeftToRight);
    m_versions->setWrapping(true);
    m_versions->setResizeMode(QListView::Adjust);
    m_versions->setMovement(QListView::Static);
    m_versions->setGridSize(QSize(84, 36));
    m_versions->setUniformItemSizes(true);
    m_versions->setSpacing(2);
    m_versions->setStyleSheet(
        "QListWidget#versionGrid { background: transparent; border: none; }"
        "QListWidget#versionGrid::item { background-color: #2b0e15; border: 2px solid #060103; border-bottom: 4px solid #12040a; "
        "color: #fbeff1; min-width: 70px; min-height: 22px; }"
        "QListWidget#versionGrid::item:selected { background-color: #e8364f; color: white; border-bottom-color: #8e1428; }");
    form->addWidget(m_versions, 1);
    connect(m_versionSearch, &QLineEdit::textChanged, this, [this] { fillVersions(); });
    connect(m_snapshots, &QCheckBox::toggled, this, [this] { fillVersions(); });
    connect(m_versions, &QListWidget::currentItemChanged, this, [this] {
        updateLoaders();
        updateName();
    });

    m_openMods = new QCheckBox(tr("Right after creating, open the mod browser (Modrinth and CurseForge)"), left);
    m_openMods->setChecked(true);
    form->addWidget(m_openMods);
    m_status = new QLabel(left);
    m_status->setWordWrap(true);
    m_status->setStyleSheet("color: #d1a3aa; background: transparent;");
    form->addWidget(m_status);

    auto* foot = new QHBoxLayout;
    auto* ai = new QPushButton(tr("Build with AI"), left);
    ai->setMinimumHeight(44);
    connect(ai, &QPushButton::clicked, this, [this] {
        emit agentRequested(tr("Build a pack for Minecraft %1 %2").arg(selectedVersion(), m_loader == "vanilla" ? QString() : m_loaderGroup->checkedButton()->text().section('\n', 0, 0)));
    });
    foot->addWidget(ai, 1);
    m_create = new QPushButton(tr("Create"), left);
    m_create->setObjectName("accentButton");
    m_create->setMinimumHeight(48);
    m_create->setStyleSheet("font-size: 15pt;");
    connect(m_create, &QPushButton::clicked, this, &VelloraCreatePage::create);
    foot->addWidget(m_create, 1);
    form->addLayout(foot);
    outer->addWidget(left, 13);

    /* ---------------- the ready packs */
    auto* right = panel(this);
    auto* ready = new QVBoxLayout(right);
    ready->setContentsMargins(16, 12, 16, 14);
    ready->setSpacing(8);
    ready->addWidget(heading(tr("Ready-made packs"), right));
    ready->addWidget(caption(tr("Start from an idea — Arkan builds it for your version"), right));
    auto* ideas = new QGridLayout;
    ideas->setSpacing(6);
    const QList<QPair<QString, QString>> ideaList = {
        { tr("With friends"), tr("a survival pack to play with friends") },
        { tr("For a weak PC"), tr("a light pack for a weak PC with lots of FPS") },
        { tr("Tech"), tr("technology and automation, factories") },
        { tr("Magic"), tr("magic and spells") },
        { tr("Adventure"), tr("adventure, dungeons, bosses, RPG") },
        { tr("Horror"), tr("a scary horror pack") },
        { tr("Beautiful graphics"), tr("beautiful graphics with shaders") },
        { tr("Zombies"), tr("survival in a zombie apocalypse") },
    };
    for (int i = 0; i < ideaList.size(); i++) {
        const auto idea = ideaList.at(i);
        auto* button = new QPushButton(idea.first, right);
        button->setMinimumHeight(34);
        connect(button, &QPushButton::clicked, this, [this, idea] {
            QString text = tr("Build %1, Minecraft %2").arg(idea.second, selectedVersion());
            if (m_loader != "vanilla") {
                text += ", " + m_loaderGroup->checkedButton()->text().section('\n', 0, 0);
            }
            emit agentRequested(text);
        });
        ideas->addWidget(button, i / 4, i % 4);
    }
    ready->addLayout(ideas);
    auto* packsHead = new QHBoxLayout;
    packsHead->addWidget(caption(tr("Popular packs"), right), 1);
    auto* all = new QPushButton(tr("All in the catalog ›"), right);
    connect(all, &QPushButton::clicked, this, &VelloraCreatePage::catalogRequested);
    packsHead->addWidget(all);
    ready->addLayout(packsHead);
    auto* scroll = new QScrollArea(right);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: transparent; } QScrollArea > QWidget > QWidget { background: transparent; }");
    auto* packsBox = new QWidget(scroll);
    m_packs = new QVBoxLayout(packsBox);
    m_packs->setContentsMargins(0, 0, 4, 0);
    m_packs->setSpacing(6);
    m_packsStatus = new QLabel(tr("Loading the most popular packs from Modrinth..."), packsBox);
    m_packsStatus->setStyleSheet("color: #d1a3aa; background: transparent;");
    m_packsStatus->setWordWrap(true);
    m_packs->addWidget(m_packsStatus);
    m_packs->addStretch(1);
    scroll->setWidget(packsBox);
    ready->addWidget(scroll, 1);
    outer->addWidget(right, 10);

    setStyleSheet(
        "QToolButton#loaderTile { background-color: #2b0e15; color: #fbeff1; border: 2px solid #060103; border-bottom: 5px solid #12040a; "
        "padding: 4px; }"
        "QToolButton#loaderTile:checked { background-color: #e8364f; color: white; border-bottom-color: #8e1428; }"
        "QToolButton#loaderTile:disabled { color: #6a4a50; background-color: #1a070b; }");
}

void VelloraCreatePage::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (!m_minecraftList) {
        loadVersions();
    }
    if (!m_packsLoaded) {
        m_packsLoaded = true;
        loadPacks();
    }
    updateName();
}

void VelloraCreatePage::loadVersions()
{
    m_minecraftList = APPLICATION->metadataIndex()->get("net.minecraft");
    if (m_minecraftList->isLoaded()) {
        fillVersions();
        return;
    }
    m_status->setText(tr("Loading the list of Minecraft versions..."));
    m_minecraftTask = m_minecraftList->getLoadTask();
    connect(m_minecraftTask.get(), &Task::succeeded, this, [this] {
        m_minecraftTask.reset();
        m_status->clear();
        fillVersions();
    });
    connect(m_minecraftTask.get(), &Task::failed, this, [this](const QString& reason) {
        m_minecraftTask.reset();
        m_status->setText(tr("Could not load Minecraft versions: %1").arg(reason));
    });
    m_minecraftTask->start();
}

void VelloraCreatePage::fillVersions()
{
    if (!m_minecraftList) {
        return;
    }
    const auto previous = selectedVersion();
    auto versions = m_minecraftList->versions();
    std::stable_sort(versions.begin(), versions.end(),
                     [](const Meta::Version::Ptr& a, const Meta::Version::Ptr& b) { return a->rawTime() > b->rawTime(); });
    const auto query = m_versionSearch->text().trimmed().toLower();
    m_versions->blockSignals(true);
    m_versions->clear();
    QListWidgetItem* select = nullptr;
    for (const auto& version : versions) {
        const bool release = version->type() == "release";
        if (!release && !m_snapshots->isChecked()) {
            continue;
        }
        if (!query.isEmpty() && !version->version().toLower().contains(query)) {
            continue;
        }
        auto* item = new QListWidgetItem(version->version(), m_versions);
        item->setTextAlignment(Qt::AlignCenter);
        item->setSizeHint(QSize(78, 30));
        if (!release) {
            item->setForeground(QColor(0xd1, 0xa3, 0xaa));
        }
        if (version->version() == previous || (!select && previous.isEmpty() && version->version() == "1.21.1")) {
            select = item;
        }
    }
    if (!select && m_versions->count() > 0) {
        select = m_versions->item(0);
    }
    m_versions->setCurrentItem(select);
    m_versions->blockSignals(false);
    updateLoaders();
    updateName();
}

QString VelloraCreatePage::selectedVersion() const
{
    return m_versions->currentItem() ? m_versions->currentItem()->text() : QString();
}

QString VelloraCreatePage::whyNot(const QString& loader, const QString& version) const
{
    if (loader == "vanilla" || version.isEmpty()) {
        return {};
    }
    const auto v = parts(version);
    if (v.isEmpty()) {
        return loader == "fabric" ? QString() : tr("not for snapshots");
    }
    if (loader == "neoforge" && !atLeast(v, 20, 1)) {
        return tr("from 1.20.1");
    }
    if ((loader == "fabric" || loader == "quilt") && !atLeast(v, 14, 0)) {
        return tr("from 1.14");
    }
    return {};
}

void VelloraCreatePage::updateLoaders()
{
    const auto version = selectedVersion();
    if (!whyNot(m_loader, version).isEmpty()) {
        m_loader = whyNot("fabric", version).isEmpty() ? "fabric" : "vanilla";
    }
    for (int i = 0; i < m_loaders.size(); i++) {
        const auto& loader = m_loaders.at(i);
        const auto why = whyNot(loader.key, version);
        auto* tile = m_loaderButtons.at(i);
        tile->setEnabled(why.isEmpty());
        tile->setText(loader.name + "\n" + (why.isEmpty() ? loader.note : why));
        tile->setChecked(loader.key == m_loader);
    }
    m_openMods->setVisible(m_loader != "vanilla");
}

QString VelloraCreatePage::autoName() const
{
    const auto base = tr("Pack %1").arg(selectedVersion());
    QStringList names;
    auto* list = APPLICATION->instances();
    for (int i = 0; i < list->count(); i++) {
        names << list->at(i)->name();
    }
    if (!names.contains(base)) {
        return base;
    }
    for (int i = 2;; i++) {
        const auto name = QString("%1 (%2)").arg(base).arg(i);
        if (!names.contains(name)) {
            return name;
        }
    }
}

void VelloraCreatePage::updateName()
{
    m_name->setPlaceholderText(autoName());
}

void VelloraCreatePage::create()
{
    const auto version = selectedVersion();
    if (!m_minecraftList || version.isEmpty()) {
        m_status->setText(tr("Pick a Minecraft version first."));
        return;
    }
    const auto minecraft = m_minecraftList->getVersion(version);
    Loader loader;
    for (const auto& l : m_loaders) {
        if (l.key == m_loader) {
            loader = l;
        }
    }
    if (loader.key == "vanilla") {
        createWith(minecraft, loader);
        return;
    }
    // the loader's versions: the recommended one for this Minecraft version is taken
    m_loaderList = APPLICATION->metadataIndex()->get(loader.uid);
    if (m_loaderList->isLoaded()) {
        createWith(minecraft, loader);
        return;
    }
    m_create->setEnabled(false);
    m_status->setText(tr("Loading %1 versions...").arg(loader.name));
    m_loaderTask = m_loaderList->getLoadTask();
    connect(m_loaderTask.get(), &Task::succeeded, this, [this, minecraft, loader] {
        m_loaderTask.reset();
        m_create->setEnabled(true);
        createWith(minecraft, loader);
    });
    connect(m_loaderTask.get(), &Task::failed, this, [this](const QString& reason) {
        m_loaderTask.reset();
        m_create->setEnabled(true);
        m_status->setText(tr("Could not load loader versions: %1").arg(reason));
    });
    m_loaderTask->start();
}

void VelloraCreatePage::createWith(const Meta::Version::Ptr& minecraft, const Loader& loader)
{
    InstanceTask* task = nullptr;
    if (loader.key == "vanilla") {
        task = new VanillaCreationTask(minecraft);
    } else {
        const bool anyMinecraft = loader.key == "fabric" || loader.key == "quilt";
        auto versions = m_loaderList->versions();
        std::stable_sort(versions.begin(), versions.end(),
                         [](const Meta::Version::Ptr& a, const Meta::Version::Ptr& b) { return a->rawTime() > b->rawTime(); });
        Meta::Version::Ptr chosen;
        for (const auto& version : versions) {
            bool matches = anyMinecraft;
            for (const auto& req : version->requiredSet()) {
                if (req.uid == "net.minecraft" && req.equalsVersion == minecraft->version()) {
                    matches = true;
                }
            }
            if (!matches) {
                continue;
            }
            if (!chosen || (version->isRecommended() && !chosen->isRecommended())) {
                chosen = version;
            }
            if (chosen->isRecommended()) {
                break;
            }
        }
        if (!chosen) {
            m_status->setText(tr("%1 is not available for Minecraft %2. Pick another version or loader.").arg(loader.name, minecraft->version()));
            return;
        }
        task = new VanillaCreationTask(minecraft, loader.uid, chosen);
    }
    const auto name = m_name->text().trimmed().isEmpty() ? autoName() : m_name->text().trimmed();
    InstanceName instName(name, minecraft->version());
    instName.setName(name);
    task->setName(instName);
    task->setGroup(APPLICATION->settings()->get("LastUsedGroupForNewInstance").toString());
    task->setIcon("default");
    m_status->clear();
    m_name->clear();
    emit taskReady(task, loader.key != "vanilla" && m_openMods->isChecked());
    updateName();
}

void VelloraCreatePage::loadPacks()
{
    QUrl url(QString(MODRINTH) + "/search");
    QUrlQuery query;
    query.addQueryItem("facets", "[[\"project_type:modpack\"]]");
    query.addQueryItem("index", "downloads");
    query.addQueryItem("limit", "10");
    url.setQuery(query);
    auto* reply = APPLICATION->network()->get(request(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_packsStatus->setText(tr("No connection to Modrinth — the packs will show up when there is internet."));
            m_packsLoaded = false;
            return;
        }
        const auto hits = QJsonDocument::fromJson(reply->readAll()).object().value("hits").toArray();
        m_packsStatus->hide();
        for (const auto& hit : hits) {
            m_packs->insertWidget(m_packs->count() - 1, packRow(hit.toObject()));
        }
    });
}

QWidget* VelloraCreatePage::packRow(const QJsonObject& hit)
{
    auto* row = new QFrame;
    row->setObjectName("packRow");
    row->setStyleSheet(
        "QFrame#packRow { background-color: #2b0e15; border: 2px solid #060103; border-bottom: 4px solid #12040a; }"
        "QLabel { background: transparent; }");
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(8, 6, 8, 6);
    auto* icon = new QLabel(row);
    icon->setFixedSize(40, 40);
    icon->setScaledContents(true);
    layout->addWidget(icon);
    const auto iconUrl = hit.value("icon_url").toString();
    if (!iconUrl.isEmpty()) {
        auto* reply = APPLICATION->network()->get(request(QUrl(iconUrl)));
        QPointer<QLabel> target(icon);
        connect(reply, &QNetworkReply::finished, this, [reply, target] {
            reply->deleteLater();
            QPixmap pixmap;
            if (target && reply->error() == QNetworkReply::NoError && pixmap.loadFromData(reply->readAll())) {
                target->setPixmap(pixmap);
            }
        });
    }
    auto* texts = new QVBoxLayout;
    texts->setSpacing(0);
    auto* title = new QLabel(hit.value("title").toString(), row);
    title->setStyleSheet("color: #fbeff1; font-weight: bold;");
    texts->addWidget(title);
    const auto downloads = hit.value("downloads").toDouble();
    const auto count = downloads >= 1e6 ? tr("%1M").arg(downloads / 1e6, 0, 'f', 1) : downloads >= 1e3 ? tr("%1K").arg(int(downloads / 1e3))
                                                                                                         : QString::number(int(downloads));
    auto* sub = new QLabel(QString("⬇ %1 · %2").arg(count, hit.value("description").toString()), row);
    sub->setStyleSheet("color: #d1a3aa;");
    sub->setMaximumWidth(360);
    sub->setText(sub->fontMetrics().elidedText(sub->text(), Qt::ElideRight, 340));
    texts->addWidget(sub);
    layout->addLayout(texts, 1);
    auto* install = new QPushButton(tr("Install"), row);
    install->setObjectName("accentButton");
    const auto projectId = hit.value("project_id").toString();
    const auto name = hit.value("title").toString();
    connect(install, &QPushButton::clicked, this, [this, projectId, name] { installPack(projectId, name); });
    layout->addWidget(install);
    return row;
}

void VelloraCreatePage::installPack(const QString& projectId, const QString& title)
{
    // the newest version of the pack: its .mrpack is imported like Modrinth's page in "Add instance" does
    auto* reply = APPLICATION->network()->get(request(QUrl(QString(MODRINTH) + "/project/" + projectId + "/version")));
    m_status->setText(tr("Looking for the newest version of %1...").arg(title));
    connect(reply, &QNetworkReply::finished, this, [this, reply, projectId, title] {
        reply->deleteLater();
        const auto versions = QJsonDocument::fromJson(reply->readAll()).array();
        if (reply->error() != QNetworkReply::NoError || versions.isEmpty()) {
            m_status->setText(tr("Could not get %1 from Modrinth.").arg(title));
            return;
        }
        const auto version = versions.first().toObject();
        QString fileUrl;
        for (const auto& file : version.value("files").toArray()) {
            const auto f = file.toObject();
            if (fileUrl.isEmpty() || f.value("primary").toBool()) {
                fileUrl = f.value("url").toString();
            }
        }
        if (fileUrl.isEmpty()) {
            m_status->setText(tr("Could not get %1 from Modrinth.").arg(title));
            return;
        }
        QMap<QString, QString> extra;
        extra.insert("pack_id", projectId);
        extra.insert("pack_version_id", version.value("id").toString());
        auto* task = new InstanceImportTask(QUrl(fileUrl), true, this, std::move(extra));
        InstanceName name(title, version.value("version_number").toString());
        task->setName(name);
        task->setGroup(APPLICATION->settings()->get("LastUsedGroupForNewInstance").toString());
        task->setIcon("default");
        m_status->clear();
        emit taskReady(task, false);
    });
}
