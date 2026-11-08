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

#include <QElapsedTimer>
#include <QTimer>
#include <QWidget>

#include "modplatform/agent/ClaudePlanner.h"
#include "modplatform/agent/ModpackBuilder.h"
#include "ui/pages/BasePage.h"

class NewInstanceDialog;
class QCheckBox;
class QGridLayout;
class QGroupBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QScrollArea;
class QSplitter;
class QToolButton;
class QTreeWidget;
class QTreeWidgetItem;
class QVBoxLayout;
class MoonyWidget;

/// "AI pack" page of the new instance dialog: a chat with Moony.
/// Moony asks what the pack should be about, then for the loader and the Minecraft version,
/// finds fitting mods (plus performance mods and every library they need) on Modrinth and CurseForge
/// and prepares the instance. Afterwards the pack can be changed by just writing ("add magic").
class AgentPage : public QWidget, public BasePage {
    Q_OBJECT

   public:
    explicit AgentPage(NewInstanceDialog* dialog, QWidget* parent = nullptr);
    ~AgentPage() override;

    QString displayName() const override { return tr("AI pack"); }
    QIcon icon() const override { return QIcon(":/sculk_theme/wizard.png"); }
    QString id() const override { return "agent"; }
    QString helpPage() const override { return QString(); }
    bool shouldDisplay() const override { return true; }
    void openedImpl() override;
    void retranslate() override;

    /// a new chat that starts from these words ("Начни с идеи" in the "Create" tab)
    void ask(const QString& text);

   private:
    enum class Stage { Topic, Setup, Building, Ready };

    // conversation
    void sendInput();
    void userSaid(const QString& text);
    void continueConversation();
    void reset();
    void startBuild();
    void startBuilder(const ModpackAgent::Plan& plan);
    void builderFinished();
    void builderFailed(const QString& reason);
    QString themeList(const ModpackAgent::Plan& plan) const;
    QString unknownWordsNote() const;
    QString wishesNote(const ModpackAgent::Plan& plan) const;

    // saved chats: every conversation is kept in <data>/agent-chats/<id>.json with its messages and its pack
    void newChat();
    void saveChat();
    void loadChat(const QString& id);
    void deleteChat(const QString& id);
    void refreshChatList();
    void clearChatView();
    QString chatFile(const QString& id) const;

    // chat widgets
    void agentSay(const QString& html, const QStringList& replies = {});
    void addUserMessage(const QString& text);
    void showAgentBubble(const QString& html);
    void showUserBubble(const QString& text);
    int bubbleWidth() const;
    void fitBubble(QLabel* label);
    bool eventFilter(QObject* watched, QEvent* event) override;
    QWidget* addRow(QWidget* bubble, bool fromAgent);
    void setReplies(const QStringList& replies);
    void showTyping(const QString& status);
    void hideTyping();
    void scrollToBottom();
    void setBusy(bool busy);

    // result
    void fillModTree();
    void modItemChanged(QTreeWidgetItem* item, int column);
    void suggestCurrent();

    NewInstanceDialog* m_dialog = nullptr;
    ModpackAgent::ClaudePlanner* m_planner = nullptr;
    ModpackAgent::ModpackBuilder* m_builder = nullptr;
    ModpackAgent::Plan m_pendingPlan;

    Stage m_stage = Stage::Topic;
    QString m_topic;
    QStringList m_unknownWords;  // from the last message
    QString m_loader;
    QString m_version;
    bool m_fillingTree = false;

    QString m_chatId;
    QString m_chatTitle;
    bool m_chatTitleEdited = false;           // the user renamed the chat, the pack name no longer replaces it
    QList<std::pair<bool, QString>> m_messages;  // (from Moony, html or plain text)
    QStringList m_currentReplies;
    bool m_replaying = false;  // a saved chat is being shown: no animations, nothing is saved again
    bool m_fillingChatList = false;
    QWidget* m_chatsPanel = nullptr;
    QLabel* m_chatsTitle = nullptr;
    QListWidget* m_chatList = nullptr;
    QPushButton* m_newChatButton = nullptr;
    QToolButton* m_deleteChatButton = nullptr;

    MoonyWidget* m_moony = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_subtitle = nullptr;
    QLabel* m_sourcesLabel = nullptr;
    QCheckBox* m_useModrinth = nullptr;
    QCheckBox* m_useCurseForge = nullptr;
    QToolButton* m_settingsButton = nullptr;
    QGroupBox* m_aiBox = nullptr;
    QLabel* m_aiHint = nullptr;
    QLineEdit* m_apiKey = nullptr;

    QScrollArea* m_chatArea = nullptr;
    QWidget* m_chatHost = nullptr;
    QVBoxLayout* m_chatLayout = nullptr;
    QWidget* m_typingRow = nullptr;
    QLabel* m_typingLabel = nullptr;
    QProgressBar* m_typingProgress = nullptr;
    QTimer m_typingTimer;
    QElapsedTimer m_followUntil;
    int m_typingDots = 0;
    QString m_typingStatus;

    QWidget* m_repliesHost = nullptr;
    QGridLayout* m_repliesLayout = nullptr;
    QLineEdit* m_input = nullptr;
    QPushButton* m_sendButton = nullptr;

    QSplitter* m_splitter = nullptr;
    QWidget* m_resultPanel = nullptr;
    QLabel* m_summary = nullptr;
    QTreeWidget* m_modTree = nullptr;
    QToolButton* m_logButton = nullptr;
    QPlainTextEdit* m_log = nullptr;
};
