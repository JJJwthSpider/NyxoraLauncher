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

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QTextDocument>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "Application.h"
#include "modplatform/agent/AgentImportTask.h"
#include "settings/SettingsObject.h"
#include "ui/Animations.h"
#include "ui/dialogs/NewInstanceDialog.h"
#include "ui/widgets/WardenWidget.h"

namespace {
constexpr int s_modIndexRole = Qt::UserRole + 1;
constexpr int s_bubbleWidth = 460;

// quick replies are full messages in the user's language; clicking one sends it
const char* s_topicReplies[] = {
    QT_TRANSLATE_NOOP("AgentPage", "Zombie apocalypse: cities, zombies and weapons"),
    QT_TRANSLATE_NOOP("AgentPage", "Magic and adventure with dungeons"),
    QT_TRANSLATE_NOOP("AgentPage", "Technology and space"),
    QT_TRANSLATE_NOOP("AgentPage", "Just optimization for a weak PC"),
};
// only examples: any Minecraft version works for which the chosen loader exists
const char* s_setupReplies[] = { "NeoForge 1.21.1", "Fabric 1.20.1", "Forge 1.12.2", "Forge 1.7.10" };
const char* s_loaderReplies[] = { "NeoForge", "Fabric", "Forge", "Quilt" };
const char* s_versionReplies[] = { "1.21.1", "1.20.1", "1.16.5", "1.12.2" };
const char* s_afterReplies[] = {
    QT_TRANSLATE_NOOP("AgentPage", "Add magic"),
    QT_TRANSLATE_NOOP("AgentPage", "Add shaders"),
    QT_TRANSLATE_NOOP("AgentPage", "Start over"),
};

bool isReset(const QString& lower)
{
    for (const auto* word : { "заново", "сначала", "сброс", "новая сборка", "start over", "reset", "new pack" }) {
        if (lower.contains(QString::fromUtf8(word))) {
            return true;
        }
    }
    return false;
}

QStringList translated(const char* const* texts, int count, bool translate)
{
    QStringList out;
    for (int i = 0; i < count; i++) {
        out << (translate ? QCoreApplication::translate("AgentPage", texts[i]) : QString::fromUtf8(texts[i]));
    }
    return out;
}
}  // namespace

AgentPage::AgentPage(NewInstanceDialog* dialog, QWidget* parent) : QWidget(parent), m_dialog(dialog)
{
    m_planner = new ModpackAgent::ClaudePlanner(this);
    m_builder = new ModpackAgent::ModpackBuilder(this);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    // header: the Warden himself, who he is, where he takes mods from
    auto* header = new QHBoxLayout;
    m_warden = new WardenWidget(this);
    m_warden->setFixedSize(72, 64);
    m_warden->setBubbleVisible(true);
    header->addWidget(m_warden);
    auto* titles = new QVBoxLayout;
    m_title = new QLabel(this);
    auto titleFont = m_title->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.3);
    titleFont.setBold(true);
    m_title->setFont(titleFont);
    titles->addWidget(m_title);
    m_subtitle = new QLabel(this);
    m_subtitle->setObjectName("hintLabel");
    m_subtitle->setWordWrap(true);
    titles->addWidget(m_subtitle);
    header->addLayout(titles, 1);
    m_sourcesLabel = new QLabel(this);
    m_sourcesLabel->setObjectName("hintLabel");
    header->addWidget(m_sourcesLabel);
    m_useModrinth = new QCheckBox("Modrinth", this);
    m_useModrinth->setChecked(APPLICATION->settings()->get("AgentUseModrinth").toBool());
    header->addWidget(m_useModrinth);
    m_useCurseForge = new QCheckBox("CurseForge", this);
    m_useCurseForge->setChecked(APPLICATION->settings()->get("AgentUseCurseForge").toBool());
    header->addWidget(m_useCurseForge);
    m_settingsButton = new QToolButton(this);
    m_settingsButton->setIcon(QIcon::fromTheme("settings"));
    m_settingsButton->setCheckable(true);
    header->addWidget(m_settingsButton);
    layout->addLayout(header);
    connect(m_useModrinth, &QCheckBox::toggled, this, [](bool on) { APPLICATION->settings()->set("AgentUseModrinth", on); });
    connect(m_useCurseForge, &QCheckBox::toggled, this, [](bool on) { APPLICATION->settings()->set("AgentUseCurseForge", on); });

    // optional Claude key, folded away behind the settings button
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
    m_aiBox->setVisible(false);
    layout->addWidget(m_aiBox);
    connect(m_settingsButton, &QToolButton::toggled, this, [this](bool on) {
        m_aiBox->setVisible(on);
        if (on) {
            Animations::fadeIn(m_aiBox);
        }
    });

    // the chat on the left, the pack on the right
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);
    m_splitter = splitter;

    m_chatArea = new QScrollArea(splitter);
    m_chatArea->setObjectName("chatArea");
    m_chatArea->setWidgetResizable(true);
    m_chatArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_chatArea->setMinimumWidth(380);
    m_chatHost = new QWidget;
    m_chatHost->setObjectName("chatHost");
    m_chatLayout = new QVBoxLayout(m_chatHost);
    m_chatLayout->setContentsMargins(10, 10, 10, 10);
    m_chatLayout->setSpacing(10);
    m_chatLayout->addStretch(1);
    m_chatArea->setWidget(m_chatHost);
    // like any chat: when new messages make the conversation longer, glide to the newest one
    connect(m_chatArea->verticalScrollBar(), &QScrollBar::rangeChanged, this, [this](int, int max) {
        if (m_followUntil.isValid() && m_followUntil.elapsed() < 1500) {
            auto* bar = m_chatArea->verticalScrollBar();
            auto* animation = new QPropertyAnimation(bar, "value", bar);
            animation->setDuration(180);
            animation->setEndValue(max);
            animation->setEasingCurve(QEasingCurve::OutCubic);
            animation->start(QAbstractAnimation::DeleteWhenStopped);
        }
    });
    // message bubbles in the colors of the current theme
    {
        const auto& pal = palette();
        m_chatArea->setStyleSheet(
            QString("QScrollArea#chatArea { background: %1; border: 1px solid %2; border-radius: 10px; }"
                    "QWidget#chatHost { background: transparent; }"
                    "#agentBubble { background: %3; border: 1px solid %2; border-radius: 12px; border-top-left-radius: 3px;"
                    " padding: 8px 12px; color: %4; }"
                    "#userBubble { background: %5; border-radius: 12px; border-top-right-radius: 3px; padding: 8px 12px; color: %6; }")
                .arg(pal.color(QPalette::Base).name(), pal.color(QPalette::Light).name(), pal.color(QPalette::Button).name(),
                     pal.color(QPalette::Text).name(), pal.color(QPalette::Highlight).name(), pal.color(QPalette::HighlightedText).name()));
    }

    m_resultPanel = new QWidget(splitter);
    auto* resultLayout = new QVBoxLayout(m_resultPanel);
    resultLayout->setContentsMargins(0, 0, 0, 0);
    m_summary = new QLabel(m_resultPanel);
    m_summary->setWordWrap(true);
    m_summary->setTextFormat(Qt::RichText);
    resultLayout->addWidget(m_summary);
    m_modTree = new QTreeWidget(m_resultPanel);
    m_modTree->setColumnCount(3);
    m_modTree->setRootIsDecorated(true);
    m_modTree->setUniformRowHeights(true);
    m_modTree->setWordWrap(false);
    m_modTree->setAlternatingRowColors(true);
    m_modTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_modTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_modTree->header()->setStretchLastSection(true);
    resultLayout->addWidget(m_modTree, 1);
    m_logButton = new QToolButton(m_resultPanel);
    m_logButton->setCheckable(true);
    m_logButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    resultLayout->addWidget(m_logButton);
    m_log = new QPlainTextEdit(m_resultPanel);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(500);
    m_log->setMaximumHeight(130);
    m_log->setVisible(false);
    resultLayout->addWidget(m_log);
    connect(m_logButton, &QToolButton::toggled, m_log, &QWidget::setVisible);
    m_resultPanel->setVisible(false);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    layout->addWidget(splitter, 1);

    // quick replies and the input line
    m_repliesHost = new QWidget(this);
    m_repliesLayout = new QGridLayout(m_repliesHost);
    m_repliesLayout->setContentsMargins(0, 0, 0, 0);
    m_repliesLayout->setHorizontalSpacing(6);
    m_repliesLayout->setVerticalSpacing(6);
    layout->addWidget(m_repliesHost);

    auto* inputRow = new QHBoxLayout;
    m_input = new QLineEdit(this);
    m_input->setMinimumHeight(36);
    m_input->setClearButtonEnabled(true);
    inputRow->addWidget(m_input, 1);
    m_sendButton = new QPushButton(this);
    m_sendButton->setObjectName("accentButton");
    m_sendButton->setMinimumHeight(36);
    inputRow->addWidget(m_sendButton);
    layout->addLayout(inputRow);

    connect(m_input, &QLineEdit::returnPressed, this, &AgentPage::sendInput);
    connect(m_sendButton, &QPushButton::clicked, this, &AgentPage::sendInput);
    connect(m_modTree, &QTreeWidget::itemChanged, this, &AgentPage::modItemChanged);

    m_typingTimer.setInterval(350);
    connect(&m_typingTimer, &QTimer::timeout, this, [this] {
        m_typingDots = (m_typingDots + 1) % 4;
        if (m_typingLabel) {
            m_typingLabel->setText(
                QString("<b>%1</b>%2<br>%3")
                    .arg(tr("The Warden is working"), QString(".").repeated(m_typingDots), m_typingStatus.toHtmlEscaped()));
        }
    });

    connect(m_planner, &ModpackAgent::ClaudePlanner::planned, this, [this](const ModpackAgent::Plan& plan) {
        m_log->appendPlainText(tr("Claude understood the request: %1").arg(plan.summary));
        startBuilder(plan);
    });
    connect(m_planner, &ModpackAgent::ClaudePlanner::failed, this, [this](const QString& reason) {
        m_log->appendPlainText(tr("Claude is not available (%1). Using the built-in rules instead.").arg(reason));
        startBuilder(m_pendingPlan);
    });
    connect(m_builder, &ModpackAgent::ModpackBuilder::log, this, [this](const QString& line) {
        m_log->appendPlainText(line);
        m_typingStatus = line;
    });
    connect(m_builder, &ModpackAgent::ModpackBuilder::progress, this, [this](int done, int total) {
        if (m_typingProgress) {
            m_typingProgress->setMaximum(std::max(total, 1));
            m_typingProgress->setValue(done);
        }
    });
    connect(m_builder, &ModpackAgent::ModpackBuilder::finished, this, &AgentPage::builderFinished);
    connect(m_builder, &ModpackAgent::ModpackBuilder::failed, this, &AgentPage::builderFailed);

    retranslate();
    reset();
}

AgentPage::~AgentPage() = default;

void AgentPage::retranslate()
{
    m_title->setText(tr("Warden, the pack builder"));
    m_subtitle->setText(
        tr("Tell me what the pack should be about, and the loader and version. "
           "I will find the mods, add optimization and everything they need."));
    m_sourcesLabel->setText(tr("Take mods from:"));
    m_settingsButton->setToolTip(tr("Smarter understanding with Claude (optional)"));
    m_aiBox->setTitle(tr("Smarter understanding with Claude (optional)"));
    m_aiHint->setText(
        tr("Without a key the agent uses built-in rules. With an Anthropic API key Claude reads your request, "
           "suggests themes and well known mods. Get a key at "
           "<a href=\"https://console.anthropic.com/\">console.anthropic.com</a>. The key is stored in the launcher "
           "settings on this computer."));
    m_apiKey->setPlaceholderText(tr("Anthropic API key (sk-ant-...)"));
    m_input->setPlaceholderText(tr("Write to the Warden, e.g.: a zombie apocalypse pack with cities, zombies and weapons"));
    m_sendButton->setText(tr("Send"));
    m_modTree->setHeaderLabels({ tr("Mod"), tr("Source"), tr("What it does") });
    m_logButton->setText(tr("Show the work log"));
}

void AgentPage::openedImpl()
{
    suggestCurrent();
    m_input->setFocus();
}

/* ---------------------------------------------------------------- chat widgets */

QWidget* AgentPage::addRow(QWidget* bubble, bool fromAgent)
{
    auto* row = new QWidget(m_chatHost);
    auto* rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(8);
    // bubbles are as wide as their text, up to a comfortable reading width
    if (auto* label = qobject_cast<QLabel*>(bubble)) {
        QTextDocument doc;
        doc.setHtml(label->text());
        const int natural = label->fontMetrics().horizontalAdvance(doc.toPlainText()) + 34;
        label->setMinimumWidth(std::clamp(natural, 60, s_bubbleWidth));
    }
    if (fromAgent) {
        auto* avatar = new QLabel(row);
        avatar->setPixmap(QIcon(":/sculk_theme/agent.svg").pixmap(36, 36));
        rowLayout->addWidget(avatar, 0, Qt::AlignTop);
        rowLayout->addWidget(bubble);
        rowLayout->addStretch(1);
    } else {
        rowLayout->addStretch(1);
        rowLayout->addWidget(bubble);
    }
    // keep the typing indicator the last row
    const int index = m_typingRow ? m_chatLayout->indexOf(m_typingRow) : m_chatLayout->count();
    m_chatLayout->insertWidget(index, row);
    Animations::growIn(row);
    scrollToBottom();
    return row;
}

void AgentPage::agentSay(const QString& html, const QStringList& replies)
{
    auto* bubble = new QLabel(html);
    bubble->setObjectName("agentBubble");
    bubble->setWordWrap(true);
    bubble->setTextFormat(Qt::RichText);
    bubble->setTextInteractionFlags(Qt::TextSelectableByMouse);
    bubble->setMaximumWidth(s_bubbleWidth);
    addRow(bubble, true);
    setReplies(replies);
}

void AgentPage::addUserMessage(const QString& text)
{
    auto* bubble = new QLabel(text.toHtmlEscaped());
    bubble->setObjectName("userBubble");
    bubble->setWordWrap(true);
    bubble->setTextFormat(Qt::RichText);
    bubble->setTextInteractionFlags(Qt::TextSelectableByMouse);
    bubble->setMaximumWidth(s_bubbleWidth);
    addRow(bubble, false);
}

void AgentPage::setReplies(const QStringList& replies)
{
    while (auto* item = m_repliesLayout->takeAt(0)) {
        // a reply may be the button that was just clicked: delete it once its click is fully handled
        if (auto* widget = item->widget()) {
            widget->hide();
            widget->deleteLater();
        }
        delete item;
    }
    for (int i = 0; i < replies.size(); i++) {
        auto* chip = new QPushButton(replies.at(i), m_repliesHost);
        chip->setObjectName("quickChip");
        chip->setCursor(Qt::PointingHandCursor);
        chip->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        const auto text = replies.at(i);
        connect(chip, &QPushButton::clicked, this, [this, text] { userSaid(text); });
        m_repliesLayout->addWidget(chip, i / 2, i % 2);
        Animations::fadeIn(chip, 200 + 60 * i);
    }
    m_repliesHost->setVisible(!replies.isEmpty());
}

void AgentPage::showTyping(const QString& status)
{
    hideTyping();
    m_typingStatus = status;
    auto* bubble = new QWidget;
    bubble->setObjectName("agentBubble");
    bubble->setAttribute(Qt::WA_StyledBackground, true);
    bubble->setMinimumWidth(280);
    bubble->setMaximumWidth(s_bubbleWidth);
    auto* bubbleLayout = new QVBoxLayout(bubble);
    bubbleLayout->setContentsMargins(12, 8, 12, 10);
    m_typingLabel = new QLabel(bubble);
    m_typingLabel->setWordWrap(true);
    m_typingLabel->setTextFormat(Qt::RichText);
    m_typingLabel->setText(QString("<b>%1</b><br>%2").arg(tr("The Warden is working"), status.toHtmlEscaped()));
    bubbleLayout->addWidget(m_typingLabel);
    m_typingProgress = new QProgressBar(bubble);
    m_typingProgress->setTextVisible(false);
    m_typingProgress->setMaximumHeight(8);
    m_typingProgress->setMaximum(0);
    bubbleLayout->addWidget(m_typingProgress);
    m_typingRow = addRow(bubble, true);
    m_typingTimer.start();
    m_warden->setTalking(true);
}

void AgentPage::hideTyping()
{
    m_typingTimer.stop();
    m_warden->setTalking(false);
    if (m_typingRow) {
        m_chatLayout->removeWidget(m_typingRow);
        m_typingRow->deleteLater();
        m_typingRow = nullptr;
        m_typingLabel = nullptr;
        m_typingProgress = nullptr;
    }
}

void AgentPage::scrollToBottom()
{
    // new rows grow in over a moment; follow them down until they are fully there
    m_followUntil.start();
}

void AgentPage::setBusy(bool busy)
{
    m_useModrinth->setEnabled(!busy);
    m_useCurseForge->setEnabled(!busy);
    m_repliesHost->setEnabled(!busy);
}

/* ---------------------------------------------------------------- conversation */

void AgentPage::reset()
{
    m_planner->abort();
    m_builder->abort();
    hideTyping();
    setBusy(false);
    m_stage = Stage::Topic;
    m_topic.clear();
    m_loader.clear();
    m_version.clear();
    m_resultPanel->setVisible(false);
    m_modTree->clear();
    m_log->clear();
    agentSay(tr("Hi! I'm the Warden and I build mod packs. <b>What should your pack be about?</b> "
                "Tell me the theme and what must be in it, for example: <i>a zombie apocalypse with cities, zombies and weapons</i>."),
             translated(s_topicReplies, std::size(s_topicReplies), true));
    suggestCurrent();
}

void AgentPage::sendInput()
{
    const auto text = m_input->text().trimmed();
    if (text.isEmpty()) {
        return;
    }
    m_input->clear();
    userSaid(text);
}

void AgentPage::userSaid(const QString& text)
{
    addUserMessage(text);
    const auto lower = text.toLower();

    if (isReset(lower)) {
        // a new conversation, the old messages stay visible above
        reset();
        return;
    }
    if (m_stage == Stage::Building) {
        agentSay(tr("I'm still assembling the pack, give me a moment."));
        return;
    }

    const auto parsed = ModpackAgent::parseLocally(text);
    bool setupChanged = false;
    if (parsed.versionGiven && parsed.minecraftVersion != m_version) {
        m_version = parsed.minecraftVersion;
        setupChanged = true;
    }
    if (parsed.loaderGiven && parsed.loader != m_loader) {
        m_loader = parsed.loader;
        setupChanged = true;
    }
    const bool topical = !parsed.themes.isEmpty() || ModpackAgent::wantsOnlyPerformance(text) || ModpackAgent::requestWantsShaders(text) ||
                         ModpackAgent::requestWantsMap(text);
    if (topical) {
        m_topic += (m_topic.isEmpty() ? QString() : QStringLiteral(", ")) + text;
    }

    if (m_stage == Stage::Ready) {
        if (topical || setupChanged) {
            startBuild();
        } else {
            agentSay(tr("Tell me what to change: a theme to add (\"add magic\"), another loader or version (\"Fabric 1.20.1\"), "
                        "or \"start over\". Or press OK below to create the instance."),
                     translated(s_afterReplies, std::size(s_afterReplies), true));
        }
        return;
    }
    if (!topical && !setupChanged) {
        agentSay(tr("I didn't catch the theme. Tell me what should be in the pack: zombies, weapons, magic, technology, "
                    "cities, adventure, horror, space..."),
                 translated(s_topicReplies, std::size(s_topicReplies), true));
        return;
    }
    continueConversation();
}

QString AgentPage::themeList(const ModpackAgent::Plan& plan) const
{
    QStringList titles;
    for (const auto& theme : plan.themes) {
        titles << theme.title.toLower();
    }
    return titles.isEmpty() ? tr("optimization only") : titles.join(", ");
}

void AgentPage::continueConversation()
{
    if (m_topic.isEmpty()) {
        m_stage = Stage::Topic;
        agentSay(tr("Good. And what should the pack be about?"), translated(s_topicReplies, std::size(s_topicReplies), true));
        return;
    }
    const auto plan = ModpackAgent::planFor(m_topic, m_loader, m_version);
    if (m_loader.isEmpty() && m_version.isEmpty()) {
        m_stage = Stage::Setup;
        agentSay(tr("Got it: <b>%1</b>. Now write the loader and the Minecraft version. Any version that has the loader works, "
                    "for example <i>NeoForge 1.21.1</i>, <i>Forge 1.12.2</i> or <i>Fabric 1.20.1</i>.")
                     .arg(themeList(plan).toHtmlEscaped()),
                 translated(s_setupReplies, std::size(s_setupReplies), false));
        return;
    }
    if (m_loader.isEmpty()) {
        m_stage = Stage::Setup;
        agentSay(tr("Minecraft %1. Which loader: NeoForge, Fabric, Forge or Quilt?").arg(m_version),
                 translated(s_loaderReplies, std::size(s_loaderReplies), false));
        return;
    }
    if (m_version.isEmpty()) {
        m_stage = Stage::Setup;
        agentSay(tr("%1 it is. Which Minecraft version? Write any version, for example 1.20.1 or 1.12.2.")
                     .arg(ModpackAgent::loaderDisplayName(m_loader)),
                 translated(s_versionReplies, std::size(s_versionReplies), false));
        return;
    }
    startBuild();
}

void AgentPage::startBuild()
{
    m_stage = Stage::Building;
    setBusy(true);
    setReplies({});
    suggestCurrent();  // no instance can be created while the pack is being rebuilt
    m_log->clear();
    m_pendingPlan = ModpackAgent::planFor(m_topic, m_loader, m_version);

    QStringList sources;
    if (m_useModrinth->isChecked())
        sources << "Modrinth";
    if (m_useCurseForge->isChecked())
        sources << "CurseForge";
    agentSay(tr("Understood! Theme: <b>%1</b>. Minecraft <b>%2</b> with <b>%3</b>. "
                "I'm looking for mods on %4 and adding optimization and the libraries they need.")
                 .arg(themeList(m_pendingPlan).toHtmlEscaped(), m_pendingPlan.minecraftVersion,
                      ModpackAgent::loaderDisplayName(m_pendingPlan.loader), sources.join(" + ")));
    showTyping(tr("Planning the pack..."));

    const auto apiKey = m_apiKey->text().trimmed();
    APPLICATION->settings()->set("AgentClaudeApiKey", apiKey);
    if (!apiKey.isEmpty()) {
        m_typingStatus = tr("Claude is reading your request...");
        m_planner->plan(QString("%1. %2 %3").arg(m_topic, ModpackAgent::loaderDisplayName(m_loader), m_version), apiKey);
    } else {
        startBuilder(m_pendingPlan);
    }
}

void AgentPage::startBuilder(const ModpackAgent::Plan& plan)
{
    m_builder->build(plan, m_useModrinth->isChecked(), m_useCurseForge->isChecked());
}

void AgentPage::builderFailed(const QString& reason)
{
    hideTyping();
    setBusy(false);
    m_stage = Stage::Setup;
    agentSay(tr("Something went wrong: %1<br>Let's try another loader or version.").arg(reason.toHtmlEscaped()),
             translated(s_setupReplies, std::size(s_setupReplies), false));
}

void AgentPage::builderFinished()
{
    hideTyping();
    setBusy(false);
    m_stage = Stage::Ready;
    fillModTree();
    if (!m_resultPanel->isVisible()) {
        m_resultPanel->setVisible(true);
        // the chat stays the bigger part, the pack list gets the rest
        const int total = m_splitter->width();
        m_splitter->setSizes({ total * 55 / 100, total * 45 / 100 });
        Animations::fadeIn(m_resultPanel, 300);
    }

    const auto& mods = m_builder->mods();
    int themed = 0;
    int libraries = 0;
    QStringList curatedGroups;
    for (const auto& curated : m_builder->plan().curated) {
        curatedGroups << curated.group;
    }
    for (const auto& mod : mods) {
        if (mod.dependency) {
            libraries++;
        } else if (!curatedGroups.contains(mod.group)) {
            themed++;
        }
    }
    const int essentials = mods.size() - themed - libraries;
    agentSay(tr("Done! <b>%1</b>: %2. For your themes: %3, optimization and comfort: %4, libraries the mods need: %5. "
                "The list is on the right, untick what you don't want.<br>Press <b>OK</b> below to create the instance, "
                "or write me what to change.")
                 .arg(m_builder->plan().name.toHtmlEscaped(), tr("%n mod(s)", "", mods.size()), QString::number(themed),
                      QString::number(essentials), QString::number(libraries)),
             translated(s_afterReplies, std::size(s_afterReplies), true));
    suggestCurrent();
}

/* ---------------------------------------------------------------- result */

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
        const auto source = mod.platform == "curseforge" ? QStringLiteral("CurseForge") : QStringLiteral("Modrinth");
        auto* item = new QTreeWidgetItem(group, { mod.title, source, mod.description });
        item->setData(0, s_modIndexRole, i);
        item->setToolTip(0, QString("%1 %2\n%3").arg(mod.title, mod.versionNumber, mod.fileName));
        item->setToolTip(2, mod.description);
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

    m_summary->setText(QString("<b>%1</b><br>Minecraft %2, %3 %4. %5")
                           .arg(plan.name.toHtmlEscaped(), plan.minecraftVersion, ModpackAgent::loaderDisplayName(plan.loader),
                                m_builder->loaderVersion(), tr("%n mod(s).", "", mods.size())));
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
    if (m_stage != Stage::Ready) {
        m_dialog->setSuggestedPack();
        return;
    }
    m_dialog->setSuggestedPack(m_builder->plan().name, new ModpackAgent::AgentImportTask(m_builder->makeIndex(), this));
}
