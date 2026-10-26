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
#include "AgentPage.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollBar>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "Application.h"
#include "modplatform/agent/AgentImportTask.h"
#include "settings/SettingsObject.h"
#include "ui/dialogs/NewInstanceDialog.h"

namespace {
constexpr int s_modIndexRole = Qt::UserRole + 1;

// examples are full requests in the user's language, clicking one fills the request field
const char* s_examples[] = {
    QT_TRANSLATE_NOOP("AgentPage", "Zombie apocalypse with guns NeoForge 1.21.1"),
    QT_TRANSLATE_NOOP("AgentPage", "Magic and adventure Fabric 1.20.1"),
    QT_TRANSLATE_NOOP("AgentPage", "Technology and space Forge 1.20.1"),
    QT_TRANSLATE_NOOP("AgentPage", "Just optimization for a weak PC 1.21.1"),
};
}  // namespace

AgentPage::AgentPage(NewInstanceDialog* dialog, QWidget* parent) : QWidget(parent), m_dialog(dialog)
{
    m_planner = new ModpackAgent::ClaudePlanner(this);
    m_builder = new ModpackAgent::ModpackBuilder(this);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_title = new QLabel(this);
    auto titleFont = m_title->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.25);
    titleFont.setBold(true);
    m_title->setFont(titleFont);
    layout->addWidget(m_title);

    m_hint = new QLabel(this);
    m_hint->setWordWrap(true);
    layout->addWidget(m_hint);

    auto* requestRow = new QHBoxLayout;
    m_request = new QLineEdit(this);
    m_request->setClearButtonEnabled(true);
    m_request->setMinimumHeight(34);
    requestRow->addWidget(m_request, 1);
    m_buildButton = new QPushButton(this);
    m_buildButton->setObjectName("accentButton");
    m_buildButton->setMinimumHeight(34);
    requestRow->addWidget(m_buildButton);
    m_stopButton = new QPushButton(this);
    m_stopButton->setMinimumHeight(34);
    m_stopButton->setVisible(false);
    requestRow->addWidget(m_stopButton);
    layout->addLayout(requestRow);

    auto* examplesRow = new QHBoxLayout;
    for (const auto* example : s_examples) {
        auto* chip = new QPushButton(this);
        chip->setObjectName("quickChip");
        chip->setProperty("sourceText", QString::fromUtf8(example));
        chip->setCursor(Qt::PointingHandCursor);
        connect(chip, &QPushButton::clicked, this, [this, chip] {
            m_request->setText(chip->text());
            startBuild();
        });
        examplesRow->addWidget(chip);
        m_examples << chip;
    }
    examplesRow->addStretch();
    layout->addLayout(examplesRow);

    m_progress = new QProgressBar(this);
    m_progress->setTextVisible(false);
    m_progress->setMaximumHeight(8);
    m_progress->setVisible(false);
    layout->addWidget(m_progress);

    m_summary = new QLabel(this);
    m_summary->setWordWrap(true);
    m_summary->setTextFormat(Qt::RichText);
    layout->addWidget(m_summary);

    auto* splitter = new QSplitter(Qt::Vertical, this);
    m_modTree = new QTreeWidget(splitter);
    m_modTree->setColumnCount(2);
    m_modTree->setRootIsDecorated(true);
    m_modTree->setUniformRowHeights(true);
    m_modTree->setWordWrap(false);
    m_modTree->setAlternatingRowColors(true);
    m_modTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_modTree->header()->setStretchLastSection(true);
    m_log = new QPlainTextEdit(splitter);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(500);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    layout->addWidget(splitter, 1);

    m_aiBox = new QGroupBox(this);
    auto* aiLayout = new QVBoxLayout(m_aiBox);
    m_aiHint = new QLabel(m_aiBox);
    m_aiHint->setWordWrap(true);
    m_aiHint->setOpenExternalLinks(true);
    aiLayout->addWidget(m_aiHint);
    m_apiKey = new QLineEdit(m_aiBox);
    m_apiKey->setEchoMode(QLineEdit::Password);
    m_apiKey->setClearButtonEnabled(true);
    m_apiKey->setText(APPLICATION->settings()->get("AgentClaudeApiKey").toString());
    connect(m_apiKey, &QLineEdit::editingFinished, this,
            [this] { APPLICATION->settings()->set("AgentClaudeApiKey", m_apiKey->text().trimmed()); });
    aiLayout->addWidget(m_apiKey);
    layout->addWidget(m_aiBox);

    connect(m_request, &QLineEdit::returnPressed, this, &AgentPage::startBuild);
    connect(m_buildButton, &QPushButton::clicked, this, &AgentPage::startBuild);
    connect(m_stopButton, &QPushButton::clicked, this, &AgentPage::stopBuild);
    connect(m_modTree, &QTreeWidget::itemChanged, this, &AgentPage::modItemChanged);

    connect(m_planner, &ModpackAgent::ClaudePlanner::planned, this, [this](const ModpackAgent::Plan& plan) {
        appendLog(tr("Claude understood the request: %1").arg(plan.summary));
        startBuilder(plan);
    });
    connect(m_planner, &ModpackAgent::ClaudePlanner::failed, this, [this](const QString& reason) {
        appendLog(tr("Claude is not available (%1). Using the built-in rules instead.").arg(reason));
        startBuilder(ModpackAgent::parseLocally(m_request->text()));
    });
    connect(m_builder, &ModpackAgent::ModpackBuilder::log, this, &AgentPage::appendLog);
    connect(m_builder, &ModpackAgent::ModpackBuilder::progress, this, [this](int done, int total) {
        m_progress->setMaximum(std::max(total, 1));
        m_progress->setValue(done);
    });
    connect(m_builder, &ModpackAgent::ModpackBuilder::finished, this, &AgentPage::builderFinished);
    connect(m_builder, &ModpackAgent::ModpackBuilder::failed, this, &AgentPage::builderFailed);

    retranslate();
}

AgentPage::~AgentPage() = default;

void AgentPage::retranslate()
{
    m_title->setText(tr("Describe the pack you want"));
    m_hint->setText(
        tr("The agent understands requests like \"zombie pack neoforge 1.21.1\": it picks fitting mods on Modrinth, "
           "adds performance and quality-of-life mods and every library the mods need, and checks that everything "
           "fits your Minecraft version and loader. You can untick mods before creating the instance."));
    m_request->setPlaceholderText(tr("For example: zombie pack neoforge 1.21.1"));
    m_buildButton->setText(tr("Build pack"));
    m_stopButton->setText(tr("Stop"));
    for (auto* chip : m_examples) {
        chip->setText(tr(chip->property("sourceText").toByteArray().constData()));
    }
    m_modTree->setHeaderLabels({ tr("Mod"), tr("What it does") });
    m_aiBox->setTitle(tr("Smarter understanding with Claude (optional)"));
    m_aiHint->setText(
        tr("Without a key the agent uses built-in rules. With an Anthropic API key Claude reads your request, "
           "suggests themes and well known mods. Get a key at "
           "<a href=\"https://console.anthropic.com/\">console.anthropic.com</a>. The key is stored in the launcher "
           "settings on this computer."));
    m_apiKey->setPlaceholderText(tr("Anthropic API key (sk-ant-...)"));
}

void AgentPage::openedImpl()
{
    suggestCurrent();
    m_request->setFocus();
}

void AgentPage::setBusy(bool busy)
{
    m_buildButton->setVisible(!busy);
    m_stopButton->setVisible(busy);
    m_progress->setVisible(busy);
    m_request->setEnabled(!busy);
    for (auto* chip : m_examples) {
        chip->setEnabled(!busy);
    }
    if (busy) {
        m_progress->setMaximum(0);  // busy indicator until the first request is counted
        m_progress->setValue(0);
    }
}

void AgentPage::startBuild()
{
    auto request = m_request->text().trimmed();
    if (request.isEmpty()) {
        request = m_request->placeholderText().section(':', 1).trimmed();
        m_request->setText(request);
    }

    m_hasResult = false;
    suggestCurrent();
    m_log->clear();
    m_modTree->clear();
    m_summary->clear();
    setBusy(true);

    const auto apiKey = m_apiKey->text().trimmed();
    APPLICATION->settings()->set("AgentClaudeApiKey", apiKey);
    if (!apiKey.isEmpty()) {
        appendLog(tr("Asking Claude to plan the pack..."));
        m_planner->plan(request, apiKey);
    } else {
        startBuilder(ModpackAgent::parseLocally(request));
    }
}

void AgentPage::startBuilder(const ModpackAgent::Plan& plan)
{
    QStringList themes;
    for (const auto& theme : plan.themes) {
        themes << theme.title;
    }
    appendLog(themes.isEmpty() ? tr("Plan: performance and quality-of-life mods only.") : tr("Plan: %1.").arg(themes.join(", ")));
    m_builder->build(plan);
}

void AgentPage::stopBuild()
{
    m_planner->abort();
    m_builder->abort();
    appendLog(tr("Stopped."));
    setBusy(false);
}

void AgentPage::builderFailed(const QString& reason)
{
    appendLog(reason);
    m_summary->setText(QString("<b>%1</b> %2").arg(tr("Could not build the pack."), reason.toHtmlEscaped()));
    setBusy(false);
}

void AgentPage::builderFinished()
{
    setBusy(false);
    m_hasResult = true;
    fillModTree();
    suggestCurrent();
}

void AgentPage::appendLog(const QString& line)
{
    m_log->appendPlainText(line);
    m_log->verticalScrollBar()->setValue(m_log->verticalScrollBar()->maximum());
}

void AgentPage::fillModTree()
{
    m_fillingTree = true;
    m_modTree->clear();

    const auto& plan = m_builder->plan();
    auto& mods = m_builder->mods();

    QMap<QString, QTreeWidgetItem*> groups;
    for (int i = 0; i < mods.size(); i++) {
        const auto& mod = mods.at(i);
        auto*& group = groups[mod.group];
        if (!group) {
            group = new QTreeWidgetItem(m_modTree, { mod.group });
            auto font = group->font(0);
            font.setBold(true);
            group->setFont(0, font);
            group->setFlags(Qt::ItemIsEnabled);
            group->setFirstColumnSpanned(true);
        }
        auto* item = new QTreeWidgetItem(group, { mod.title, mod.description });
        item->setData(0, s_modIndexRole, i);
        item->setToolTip(0, QString("%1 %2\n%3").arg(mod.title, mod.versionNumber, mod.fileName));
        item->setToolTip(1, mod.description);
        if (mod.dependency) {
            // libraries are needed by other mods, switching them off would break the game
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            item->setToolTip(0, tr("Needed by other mods in the pack"));
        } else {
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
            item->setCheckState(0, mod.enabled ? Qt::Checked : Qt::Unchecked);
        }
    }
    m_modTree->expandAll();

    m_summary->setText(QString("<b>%1</b> — Minecraft %2, %3 %4. %5<br>%6")
                           .arg(plan.name.toHtmlEscaped(), plan.minecraftVersion, ModpackAgent::loaderDisplayName(plan.loader),
                                m_builder->loaderVersion(), tr("%n mod(s).", "", mods.size()), plan.summary.toHtmlEscaped()));
    m_fillingTree = false;
}

void AgentPage::modItemChanged(QTreeWidgetItem* item, int column)
{
    if (m_fillingTree || column != 0) {
        return;
    }
    bool ok = false;
    const int index = item->data(0, s_modIndexRole).toInt(&ok);
    auto& mods = m_builder->mods();
    if (!ok || index < 0 || index >= mods.size()) {
        return;
    }
    mods[index].enabled = item->checkState(0) == Qt::Checked;
    suggestCurrent();
}

void AgentPage::suggestCurrent()
{
    if (!isOpened) {
        return;
    }
    if (!m_hasResult) {
        m_dialog->setSuggestedPack();
        return;
    }
    m_dialog->setSuggestedPack(m_builder->plan().name, new ModpackAgent::AgentImportTask(m_builder->makeIndex(), this));
}
