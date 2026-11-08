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

#include <algorithm>

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QDateTime>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMessageBox>
#include <QSaveFile>
#include <QUuid>
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
#include "ui/widgets/MoonyWidget.h"

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

QJsonArray toJson(const QStringList& list)
{
    return QJsonArray::fromStringList(list);
}

QStringList fromJson(const QJsonValue& value)
{
    QStringList list;
    for (const auto& item : value.toArray()) {
        list << item.toString();
    }
    return list;
}

QJsonObject modToJson(const ModpackAgent::AgentMod& mod)
{
    return QJsonObject{
        { "platform", mod.platform },
        { "projectId", mod.projectId },
        { "aliases", toJson(mod.aliases) },
        { "slug", mod.slug },
        { "title", mod.title },
        { "description", mod.description },
        { "group", mod.group },
        { "kind", mod.kind },
        { "versionNumber", mod.versionNumber },
        { "fileName", mod.fileName },
        { "url", mod.url },
        { "sha1", mod.sha1 },
        { "sha512", mod.sha512 },
        { "size", mod.size },
        { "dependency", mod.dependency },
        { "requested", mod.requested },
        { "enabled", mod.enabled },
        { "neededBy", toJson(mod.neededBy) },
        { "requiredProjects", toJson(mod.requiredProjects) },
        { "incompatibleProjects", toJson(mod.incompatibleProjects) },
    };
}

ModpackAgent::AgentMod modFromJson(const QJsonObject& obj)
{
    ModpackAgent::AgentMod mod;
    mod.platform = obj["platform"].toString();
    mod.projectId = obj["projectId"].toString();
    mod.aliases = fromJson(obj["aliases"]);
    mod.slug = obj["slug"].toString();
    mod.title = obj["title"].toString();
    mod.description = obj["description"].toString();
    mod.group = obj["group"].toString();
    mod.kind = obj["kind"].toString("mod");
    mod.versionNumber = obj["versionNumber"].toString();
    mod.fileName = obj["fileName"].toString();
    mod.url = obj["url"].toString();
    mod.sha1 = obj["sha1"].toString();
    mod.sha512 = obj["sha512"].toString();
    mod.size = obj["size"].toInteger();
    mod.dependency = obj["dependency"].toBool();
    mod.requested = obj["requested"].toBool();
    mod.enabled = obj["enabled"].toBool(true);
    mod.neededBy = fromJson(obj["neededBy"]);
    mod.requiredProjects = fromJson(obj["requiredProjects"]);
    mod.incompatibleProjects = fromJson(obj["incompatibleProjects"]);
    return mod;
}

QJsonObject readJson(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}

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

    // header: Moony itself, who it is, where it takes mods from
    auto* header = new QHBoxLayout;
    m_moony = new MoonyWidget(this);
    m_moony->setFixedSize(100, 70);
    m_moony->setBubbleVisible(true);
    header->addWidget(m_moony);
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

    // saved chats: one per pack, the newest on top
    m_chatsPanel = new QWidget(splitter);
    auto* chatsLayout = new QVBoxLayout(m_chatsPanel);
    chatsLayout->setContentsMargins(0, 0, 0, 0);
    chatsLayout->setSpacing(6);
    m_chatsTitle = new QLabel(m_chatsPanel);
    auto chatsFont = m_chatsTitle->font();
    chatsFont.setBold(true);
    m_chatsTitle->setFont(chatsFont);
    chatsLayout->addWidget(m_chatsTitle);
    m_newChatButton = new QPushButton(m_chatsPanel);
    m_newChatButton->setObjectName("accentButton");
    chatsLayout->addWidget(m_newChatButton);
    m_chatList = new QListWidget(m_chatsPanel);
    m_chatList->setTextElideMode(Qt::ElideRight);
    m_chatList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_chatList->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    chatsLayout->addWidget(m_chatList, 1);
    m_deleteChatButton = new QToolButton(m_chatsPanel);
    m_deleteChatButton->setIcon(QIcon::fromTheme("delete"));
    m_deleteChatButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_deleteChatButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_deleteChatButton->setEnabled(false);
    chatsLayout->addWidget(m_deleteChatButton);
    m_chatsPanel->setMinimumWidth(160);
    m_chatsPanel->setMaximumWidth(240);
    connect(m_newChatButton, &QPushButton::clicked, this, &AgentPage::newChat);
    connect(m_chatList, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        const auto id = item->data(Qt::UserRole).toString();
        if (id != m_chatId) {
            loadChat(id);
        }
    });
    connect(m_chatList, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem* item) { m_deleteChatButton->setEnabled(item != nullptr); });
    connect(m_chatList, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) {
        // renamed with a double click
        if (m_fillingChatList) {
            return;
        }
        const auto id = item->data(Qt::UserRole).toString();
        const auto title = item->text().trimmed();
        if (title.isEmpty()) {
            return;
        }
        if (id == m_chatId) {
            m_chatTitle = title;
            m_chatTitleEdited = true;
            saveChat();
        } else {
            auto obj = readJson(chatFile(id));
            obj["title"] = title;
            obj["titleEdited"] = true;
            QSaveFile file(chatFile(id));
            if (file.open(QIODevice::WriteOnly)) {
                file.write(QJsonDocument(obj).toJson());
                file.commit();
            }
        }
    });
    connect(m_deleteChatButton, &QToolButton::clicked, this, [this] {
        if (auto* item = m_chatList->currentItem()) {
            deleteChat(item->data(Qt::UserRole).toString());
        }
    });

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
    m_chatArea->viewport()->installEventFilter(this);
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
    splitter->setStretchFactor(0, 0);
    splitter->setSizes({ 190, 800, 0 });
    splitter->setStretchFactor(1, 3);
    splitter->setStretchFactor(2, 2);
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
                    .arg(tr("Arkan is working"), QString(".").repeated(m_typingDots), m_typingStatus.toHtmlEscaped()));
        }
    });

    connect(m_planner, &ModpackAgent::ClaudePlanner::planned, this, [this](const ModpackAgent::Plan& plan) {
        m_log->appendPlainText(tr("Claude understood the request: %1").arg(plan.summary));
        // the mods the user named and what they want left out are kept exactly as the built-in rules read them
        auto merged = plan;
        for (const auto& name : m_pendingPlan.requestedMods) {
            if (!merged.requestedMods.contains(name, Qt::CaseInsensitive))
                merged.requestedMods << name;
        }
        merged.excluded = m_pendingPlan.excluded;
        startBuilder(merged);
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
    refreshChatList();
    reset();
}

AgentPage::~AgentPage() = default;

void AgentPage::retranslate()
{
    m_title->setText(tr("Arkan the wizard, the pack builder"));
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
    m_input->setPlaceholderText(tr("Write to Arkan, e.g.: a zombie apocalypse pack with cities, zombies and weapons"));
    m_sendButton->setText(tr("Send"));
    m_modTree->setHeaderLabels({ tr("Mod"), tr("Source"), tr("What it does") });
    m_logButton->setText(tr("Show the work log"));
    m_chatsTitle->setText(tr("My packs"));
    m_newChatButton->setText(tr("New chat"));
    m_newChatButton->setToolTip(tr("Start a conversation about another pack; this one stays saved"));
    m_deleteChatButton->setText(tr("Delete chat"));
    m_chatList->setToolTip(tr("Your conversations with Arkan. Click one to continue it, double click to rename."));
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
        label->setProperty("naturalWidth", natural);
        fitBubble(label);
    }
    if (fromAgent) {
        auto* avatar = new QLabel(row);
        avatar->setPixmap(QIcon(":/sculk_theme/wizard.png").pixmap(36, 36));
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
    if (!m_replaying) {
        Animations::growIn(row);
    }
    scrollToBottom();
    return row;
}

int AgentPage::bubbleWidth() const
{
    // a comfortable reading width, but never wider than the chat column (minus the avatar)
    return std::clamp(m_chatArea->viewport()->width() - 80, 120, s_bubbleWidth);
}

void AgentPage::fitBubble(QLabel* label)
{
    const int width = bubbleWidth();
    label->setMaximumWidth(width);
    label->setMinimumWidth(std::clamp(label->property("naturalWidth").toInt(), 60, width));
}

bool AgentPage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_chatArea->viewport() && event->type() == QEvent::Resize) {
        for (auto* label : m_chatHost->findChildren<QLabel*>()) {
            if (label->property("naturalWidth").isValid()) {
                fitBubble(label);
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void AgentPage::agentSay(const QString& html, const QStringList& replies)
{
    showAgentBubble(html);
    m_messages << std::make_pair(true, html);
    setReplies(replies);
    saveChat();
}

void AgentPage::addUserMessage(const QString& text)
{
    showUserBubble(text);
    m_messages << std::make_pair(false, text);
    if (m_chatTitle.isEmpty()) {
        // until a pack is built the chat is named after the first wish
        m_chatTitle = text.size() > 40 ? text.left(38).trimmed() + QStringLiteral("…") : text;
    }
    saveChat();
}

void AgentPage::showAgentBubble(const QString& html)
{
    auto* bubble = new QLabel(html);
    bubble->setObjectName("agentBubble");
    bubble->setWordWrap(true);
    bubble->setTextFormat(Qt::RichText);
    bubble->setTextInteractionFlags(Qt::TextSelectableByMouse);
    bubble->setMaximumWidth(s_bubbleWidth);
    addRow(bubble, true);
}

void AgentPage::showUserBubble(const QString& text)
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
    m_currentReplies = replies;
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
    bubble->setMaximumWidth(bubbleWidth());
    auto* bubbleLayout = new QVBoxLayout(bubble);
    bubbleLayout->setContentsMargins(12, 8, 12, 10);
    m_typingLabel = new QLabel(bubble);
    m_typingLabel->setWordWrap(true);
    m_typingLabel->setTextFormat(Qt::RichText);
    m_typingLabel->setText(QString("<b>%1</b><br>%2").arg(tr("Arkan is working"), status.toHtmlEscaped()));
    bubbleLayout->addWidget(m_typingLabel);
    m_typingProgress = new QProgressBar(bubble);
    m_typingProgress->setTextVisible(false);
    m_typingProgress->setMaximumHeight(8);
    m_typingProgress->setMaximum(0);
    bubbleLayout->addWidget(m_typingProgress);
    m_typingRow = addRow(bubble, true);
    m_typingTimer.start();
    m_moony->setTalking(true);
}

void AgentPage::hideTyping()
{
    m_typingTimer.stop();
    m_moony->setTalking(false);
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
    clearChatView();
    m_chatId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_chatTitle.clear();
    m_chatTitleEdited = false;
    m_messages.clear();
    m_chatList->clearSelection();
    m_chatList->setCurrentItem(nullptr);
    m_stage = Stage::Topic;
    m_topic.clear();
    m_unknownWords.clear();
    m_loader.clear();
    m_version.clear();
    m_resultPanel->setVisible(false);
    m_modTree->clear();
    m_log->clear();
    agentSay(tr("Hi! I'm Arkan and I build mod packs. <b>What should your pack be about?</b> "
                "Tell me the theme and what must be in it, for example: <i>a zombie apocalypse with cities, zombies and weapons</i>."),
             translated(s_topicReplies, std::size(s_topicReplies), true));
    suggestCurrent();
}

void AgentPage::ask(const QString& text)
{
    if (text.trimmed().isEmpty()) {
        return;
    }
    newChat();
    userSaid(text.trimmed());
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
    const auto lower = text.toLower();
    if (isReset(lower)) {
        // a new chat; the old one stays saved in the list
        reset();
        return;
    }
    addUserMessage(text);
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
    const bool topical = !parsed.themes.isEmpty() || !parsed.requestedMods.isEmpty() || !parsed.excluded.isEmpty() ||
                         ModpackAgent::wantsOnlyPerformance(text) || ModpackAgent::requestWantsShaders(text) ||
                         ModpackAgent::requestWantsMap(text);
    if (topical) {
        // every message is kept, so later wishes ("убери зомби", "добавь Create") change the earlier ones
        m_topic += (m_topic.isEmpty() ? QString() : QStringLiteral(". ")) + text;
    }
    m_unknownWords = parsed.unknownWords;

    if (m_stage == Stage::Ready) {
        if (topical || setupChanged) {
            startBuild();
        } else {
            agentSay(unknownWordsNote() +
                         tr("Tell me what to change: a theme or a mod to add (\"add magic\", \"add Create\"), something to remove "
                            "(\"remove zombies\"), another loader or version (\"Fabric 1.20.1\"), or \"start over\". "
                            "Or press OK below to create the instance."),
                     translated(s_afterReplies, std::size(s_afterReplies), true));
        }
        return;
    }
    if (!topical && !setupChanged) {
        agentSay(unknownWordsNote() + tr("I didn't catch the theme. Tell me what should be in the pack: zombies, weapons, magic, "
                                         "technology, cities, adventure, horror, space... You can also name mods in English, "
                                         "for example <i>Create</i> or <i>Farmer's Delight</i>."),
                 translated(s_topicReplies, std::size(s_topicReplies), true));
        return;
    }
    continueConversation();
}

QString AgentPage::unknownWordsNote() const
{
    if (m_unknownWords.isEmpty()) {
        return {};
    }
    return tr("I don't know these words: <i>%1</i>. Say it differently, or write the mod's name in English as on Modrinth.")
               .arg(m_unknownWords.join(", ").toHtmlEscaped()) +
           "<br>";
}

QString AgentPage::wishesNote(const ModpackAgent::Plan& plan) const
{
    QString note;
    if (!plan.requestedMods.isEmpty()) {
        note += " " + tr("Mods you named: <b>%1</b>.").arg(plan.requestedMods.join(", ").toHtmlEscaped());
    }
    if (!plan.excluded.isEmpty()) {
        note += " " + tr("Leaving out: <b>%1</b>.").arg(plan.excluded.join(", ").toHtmlEscaped());
    }
    return note;
}

QString AgentPage::themeList(const ModpackAgent::Plan& plan) const
{
    QStringList titles;
    for (const auto& theme : plan.themes) {
        titles << theme.title.toLower();
    }
    if (titles.isEmpty() && !plan.requestedMods.isEmpty()) {
        return tr("the mods you named");
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
        agentSay(unknownWordsNote() +
                     tr("Got it: <b>%1</b>.").arg(themeList(plan).toHtmlEscaped()) + wishesNote(plan) + " " +
                     tr("Now write the loader and the Minecraft version. Any version that has the loader works, "
                        "for example <i>NeoForge 1.21.1</i>, <i>Forge 1.12.2</i> or <i>Fabric 1.20.1</i>."),
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
    agentSay(unknownWordsNote() +
             tr("Understood! Theme: <b>%1</b>. Minecraft <b>%2</b> with <b>%3</b>.")
                 .arg(themeList(m_pendingPlan).toHtmlEscaped(), m_pendingPlan.minecraftVersion,
                      ModpackAgent::loaderDisplayName(m_pendingPlan.loader)) +
             wishesNote(m_pendingPlan) + " " +
             tr("I'm looking for mods on %1 and adding optimization and every library the mods need.").arg(sources.join(" + ")));
    showTyping(tr("Planning the pack..."));

    const auto apiKey = m_apiKey->text().trimmed();
    APPLICATION->settings()->set("AgentClaudeApiKey", apiKey);
    if (!apiKey.isEmpty()) {
        m_typingStatus = tr("Claude is reading your request...");
        m_planner->plan(QString("%1. %2 %3").arg(m_topic, ModpackAgent::loaderDisplayName(m_loader), m_version), apiKey,
                        m_pendingPlan.requestedMods, m_pendingPlan.excluded);
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
    if (!m_chatTitleEdited) {
        m_chatTitle = m_builder->plan().name;
    }
    fillModTree();
    if (!m_resultPanel->isVisible()) {
        m_resultPanel->setVisible(true);
        // the chat stays the bigger part, the pack list gets the rest
        const int total = m_splitter->width();
        const int chats = m_chatsPanel->minimumWidth() + 30;
        m_splitter->setSizes({ chats, (total - chats) * 60 / 100, (total - chats) * 40 / 100 });
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
    // name the mods of every theme, so the user sees right away that the pack is about what they asked for
    QString details;
    for (const auto& theme : m_builder->plan().themes) {
        QStringList titles;
        for (const auto& mod : mods) {
            if (!mod.dependency && mod.group == theme.title) {
                titles << mod.title;
            }
        }
        if (!titles.isEmpty()) {
            details += QString("<br><b>%1</b>: %2.").arg(theme.title.toHtmlEscaped(), titles.join(", ").toHtmlEscaped());
        }
    }
    if (!m_builder->foundRequested().isEmpty()) {
        details += "<br>" + tr("Added the mods you asked for: <b>%1</b>.").arg(m_builder->foundRequested().join(", ").toHtmlEscaped());
    }
    if (!m_builder->missingRequested().isEmpty()) {
        details += "<br>" + tr("Not found for Minecraft %1 with %2: <b>%3</b>. Check the name or try another version.")
                                .arg(m_builder->plan().minecraftVersion, ModpackAgent::loaderDisplayName(m_builder->plan().loader),
                                     m_builder->missingRequested().join(", ").toHtmlEscaped());
    }
    auto replies = translated(s_afterReplies, std::size(s_afterReplies), true);
    if (!m_builder->emptyThemes().isEmpty()) {
        QString warning = tr("<b>For Minecraft %1 with %2 there are no mods about: %3.</b>")
                              .arg(m_builder->plan().minecraftVersion, ModpackAgent::loaderDisplayName(m_builder->plan().loader),
                                   m_builder->emptyThemes().join(", ").toHtmlEscaped());
        if (!m_builder->suggestedSetup().isEmpty()) {
            warning += " " + tr("Most of them are made for <b>%1</b> (%n mod(s) found), write it or press the button below and "
                                "I'll rebuild the pack.",
                                "", m_builder->suggestedSetupMods())
                                 .arg(m_builder->suggestedSetup());
            replies.prepend(m_builder->suggestedSetup());
        } else {
            warning += " " + tr("Try another Minecraft version or loader.");
        }
        details += "<br>" + warning;
    }
    QStringList libraryNotes;
    for (const auto& mod : mods) {
        if (mod.dependency) {
            libraryNotes << (mod.neededBy.isEmpty() ? mod.title : tr("%1 (for %2)").arg(mod.title, mod.neededBy.join(", ")));
        }
    }
    if (!libraryNotes.isEmpty()) {
        details += "<br>" + tr("Libraries downloaded automatically: %1.").arg(libraryNotes.join("; ").toHtmlEscaped());
    }
    agentSay(tr("Done! <b>%1</b>: %2. For your themes: %3, optimization and comfort: %4, libraries the mods need: %5.")
                     .arg(m_builder->plan().name.toHtmlEscaped(), tr("%n mod(s)", "", mods.size()), QString::number(themed),
                          QString::number(essentials), QString::number(libraries)) +
                 details + "<br>" +
                 tr("The list is on the right, untick what you don't want: libraries nobody needs any more are left out by "
                    "themselves.<br>Press <b>OK</b> below to create the instance, or write me what to change."),
             replies);
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
            item->setToolTip(0, mod.neededBy.isEmpty() ? tr("Needed by other mods in the pack")
                                                        : tr("Needed by: %1").arg(mod.neededBy.join(", ")));
            if (!mod.neededBy.isEmpty()) {
                item->setText(2, tr("Needed by: %1").arg(mod.neededBy.join(", ")));
            }
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
    saveChat();
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

/* ---------------------------------------------------------------- saved chats */

QString AgentPage::chatFile(const QString& id) const
{
    return QDir(APPLICATION->dataRoot()).filePath("agent-chats/" + id + ".json");
}

void AgentPage::newChat()
{
    if (m_stage == Stage::Building) {
        // the pack being built stays in its chat and is built again when the chat is opened
        saveChat();
    }
    reset();
}

void AgentPage::clearChatView()
{
    hideTyping();
    while (auto* item = m_chatLayout->takeAt(0)) {
        if (auto* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
    m_chatLayout->addStretch(1);
}

void AgentPage::saveChat()
{
    // a chat with only the greeting is not worth keeping
    if (m_replaying || m_chatId.isEmpty() ||
        std::none_of(m_messages.cbegin(), m_messages.cend(), [](const auto& message) { return !message.first; })) {
        return;
    }
    QJsonArray messages;
    for (const auto& [fromAgent, text] : m_messages) {
        // "warden" is the name the agent had before; saved chats keep using it
        messages.append(QJsonObject{ { "from", fromAgent ? "warden" : "user" }, { "text", text } });
    }
    QJsonObject obj{
        { "formatVersion", 1 },
        { "id", m_chatId },
        { "title", m_chatTitle },
        { "titleEdited", m_chatTitleEdited },
        { "updated", QDateTime::currentDateTimeUtc().toString(Qt::ISODate) },
        { "stage", static_cast<int>(m_stage) },
        { "topic", m_topic },
        { "loader", m_loader },
        { "version", m_version },
        { "messages", messages },
        { "replies", toJson(m_currentReplies) },
    };
    if (m_stage == Stage::Ready) {
        const auto& plan = m_builder->plan();
        QJsonArray mods;
        for (const auto& mod : m_builder->mods()) {
            mods.append(modToJson(mod));
        }
        obj["pack"] = QJsonObject{
            { "name", plan.name },
            { "summary", plan.summary },
            { "minecraftVersion", plan.minecraftVersion },
            { "loader", plan.loader },
            { "loaderVersion", m_builder->loaderVersion() },
            { "mods", mods },
            { "log", m_log->toPlainText() },
        };
    }

    QDir().mkpath(QFileInfo(chatFile(m_chatId)).absolutePath());
    QSaveFile file(chatFile(m_chatId));
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    file.write(QJsonDocument(obj).toJson());
    if (file.commit()) {
        refreshChatList();
    }
}

void AgentPage::refreshChatList()
{
    struct Entry {
        QString id;
        QString title;
        QString details;
        QDateTime updated;
    };
    QList<Entry> entries;
    const QDir dir(QDir(APPLICATION->dataRoot()).filePath("agent-chats"));
    for (const auto& name : dir.entryList({ "*.json" }, QDir::Files)) {
        const auto obj = readJson(dir.filePath(name));
        if (obj["id"].toString().isEmpty()) {
            continue;
        }
        Entry entry{ obj["id"].toString(), obj["title"].toString(), {},
                     QDateTime::fromString(obj["updated"].toString(), Qt::ISODate).toLocalTime() };
        const auto pack = obj["pack"].toObject();
        entry.details = pack.isEmpty() ? tr("Not built yet")
                                       : tr("Minecraft %1, %2, %n mod(s)", "", pack["mods"].toArray().size())
                                             .arg(pack["minecraftVersion"].toString(),
                                                  ModpackAgent::loaderDisplayName(pack["loader"].toString()));
        if (entry.title.isEmpty()) {
            entry.title = tr("Chat");
        }
        entries << entry;
    }
    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.updated > b.updated; });

    m_fillingChatList = true;
    m_chatList->clear();
    for (const auto& entry : entries) {
        auto* item = new QListWidgetItem(entry.title, m_chatList);
        item->setData(Qt::UserRole, entry.id);
        item->setToolTip(QString("%1\n%2\n%3").arg(entry.title, entry.details, QLocale().toString(entry.updated, QLocale::ShortFormat)));
        item->setIcon(QIcon(":/sculk_theme/wizard.png"));
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        if (entry.id == m_chatId) {
            m_chatList->setCurrentItem(item);
        }
    }
    m_fillingChatList = false;
    m_deleteChatButton->setEnabled(m_chatList->currentItem() != nullptr);
}

void AgentPage::loadChat(const QString& id)
{
    const auto obj = readJson(chatFile(id));
    if (obj.isEmpty()) {
        refreshChatList();
        return;
    }
    m_planner->abort();
    m_builder->abort();
    setBusy(false);
    clearChatView();

    m_chatId = id;
    m_chatTitle = obj["title"].toString();
    m_chatTitleEdited = obj["titleEdited"].toBool();
    m_topic = obj["topic"].toString();
    m_loader = obj["loader"].toString();
    m_version = obj["version"].toString();
    m_unknownWords.clear();
    m_messages.clear();

    // show the conversation as it was, without animations
    m_replaying = true;
    for (const auto& value : obj["messages"].toArray()) {
        const auto message = value.toObject();
        const bool fromAgent = message["from"].toString() == "warden";
        const auto text = message["text"].toString();
        if (fromAgent) {
            showAgentBubble(text);
        } else {
            showUserBubble(text);
        }
        m_messages << std::make_pair(fromAgent, text);
    }
    m_replaying = false;
    setReplies(fromJson(obj["replies"]));

    const auto pack = obj["pack"].toObject();
    if (!pack.isEmpty()) {
        ModpackAgent::Plan plan;
        plan.name = pack["name"].toString();
        plan.summary = pack["summary"].toString();
        plan.minecraftVersion = pack["minecraftVersion"].toString();
        plan.loader = pack["loader"].toString();
        QList<ModpackAgent::AgentMod> mods;
        for (const auto& value : pack["mods"].toArray()) {
            mods << modFromJson(value.toObject());
        }
        m_builder->restore(plan, mods, pack["loaderVersion"].toString());
        m_log->setPlainText(pack["log"].toString());
        fillModTree();
        if (!m_resultPanel->isVisible()) {
            m_resultPanel->setVisible(true);
            const int total = m_splitter->width();
            const int chats = m_chatsPanel->minimumWidth() + 30;
            m_splitter->setSizes({ chats, (total - chats) * 60 / 100, (total - chats) * 40 / 100 });
        }
        m_stage = Stage::Ready;
    } else {
        m_resultPanel->setVisible(false);
        m_modTree->clear();
        m_log->clear();
        m_stage = static_cast<Stage>(std::clamp(obj["stage"].toInt(), 0, static_cast<int>(Stage::Ready)));
        if (m_stage == Stage::Ready) {
            m_stage = Stage::Setup;
        }
    }
    refreshChatList();
    m_chatArea->verticalScrollBar()->setValue(m_chatArea->verticalScrollBar()->maximum());
    scrollToBottom();

    if (m_stage == Stage::Building) {
        // the chat was closed while the pack was being assembled: finish the job
        if (!m_topic.isEmpty() && !m_loader.isEmpty() && !m_version.isEmpty()) {
            startBuild();
        } else {
            m_stage = Stage::Setup;
        }
    }
    suggestCurrent();
    m_input->setFocus();
}

void AgentPage::deleteChat(const QString& id)
{
    const auto obj = readJson(chatFile(id));
    const auto title = obj["title"].toString();
    const auto answer = QMessageBox::question(this, tr("Delete chat"),
                                              tr("Delete the chat \"%1\"? The instances you created stay.").arg(title),
                                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }
    QFile::remove(chatFile(id));
    if (id == m_chatId) {
        reset();
    }
    refreshChatList();
}
