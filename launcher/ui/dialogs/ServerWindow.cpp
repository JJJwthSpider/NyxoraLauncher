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
#include "ServerWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
#include <QVBoxLayout>

#include "Application.h"
#include "BaseInstance.h"
#include "DesktopServices.h"
#include "InstanceList.h"
#include "icons/IconList.h"
#include "minecraft/MinecraftInstance.h"

ServerWindow::ServerWindow(BaseInstance* preselected, QWidget* parent) : QDialog(parent), m_manager(ServerManager::instance())
{
    setWindowTitle(tr("My server"));
    setAttribute(Qt::WA_DeleteOnClose);
    resize(1040, 680);

    auto* layout = new QHBoxLayout(this);
    auto* splitter = new QSplitter(this);
    splitter->addWidget(buildSettings());
    splitter->addWidget(buildStatus());
    splitter->setStretchFactor(0, 4);
    splitter->setStretchFactor(1, 6);
    splitter->setSizes({ 470, 570 });
    layout->addWidget(splitter);

    // the instance the server runs, the selected one, or the first one
    const QString running = m_manager->isActive() ? m_manager->instanceId() : QString();
    for (int i = 0; i < m_instance->count(); i++) {
        const QString id = m_instance->itemData(i).toString();
        if ((!running.isEmpty() && id == running) || (running.isEmpty() && preselected && id == preselected->id())) {
            m_instance->setCurrentIndex(i);
        }
    }
    loadSettings();

    connect(m_manager, &ServerManager::stateChanged, this, &ServerWindow::updateState);
    connect(m_manager, &ServerManager::status, m_status, &QLabel::setText);
    connect(m_manager, &ServerManager::addressesChanged, this, &ServerWindow::updateAddresses);
    connect(m_manager, &ServerManager::logLine, this, [this](const QString& line) { m_console->appendPlainText(line); });
    connect(m_manager, &ServerManager::playersChanged, this, [this](const QStringList& players) {
        m_players->clear();
        m_players->addItems(players);
        if (players.isEmpty()) {
            m_players->addItem(tr("Nobody yet"));
        }
    });
    emit m_manager->playersChanged(m_manager->players());
    updateState();
    updateAddresses();
}

QWidget* ServerWindow::buildSettings()
{
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setMinimumWidth(440);
    m_settingsBox = new QWidget(scroll);
    auto* form = new QFormLayout(m_settingsBox);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);

    auto* title = new QLabel(tr("<b>Server settings</b>"), m_settingsBox);
    form->addRow(title);

    m_instance = new QComboBox(m_settingsBox);
    auto* instances = APPLICATION->instances();
    for (int i = 0; i < instances->count(); i++) {
        if (auto* instance = dynamic_cast<MinecraftInstance*>(instances->at(i))) {
            m_instance->addItem(APPLICATION->icons()->getIcon(instance->iconKey()), instance->name(), instance->id());
        }
    }
    form->addRow(tr("Instance:"), m_instance);
    m_instanceInfo = new QLabel(m_settingsBox);
    m_instanceInfo->setWordWrap(true);
    form->addRow(m_instanceInfo);

    m_gamemode = new QComboBox(m_settingsBox);
    m_gamemode->addItem(tr("Survival"), "survival");
    m_gamemode->addItem(tr("Creative"), "creative");
    m_gamemode->addItem(tr("Adventure"), "adventure");
    form->addRow(tr("Game mode:"), m_gamemode);

    m_difficulty = new QComboBox(m_settingsBox);
    m_difficulty->addItem(tr("Peaceful"), "peaceful");
    m_difficulty->addItem(tr("Easy"), "easy");
    m_difficulty->addItem(tr("Normal"), "normal");
    m_difficulty->addItem(tr("Hard"), "hard");
    form->addRow(tr("Difficulty:"), m_difficulty);

    m_pvp = new QCheckBox(tr("PvP between players"), m_settingsBox);
    m_hardcore = new QCheckBox(tr("Hardcore: one life"), m_settingsBox);
    m_allowFlight = new QCheckBox(tr("Allow flying"), m_settingsBox);
    m_allowFlight->setToolTip(tr("Some mods need it, otherwise the server kicks players for flying"));
    m_commandBlocks = new QCheckBox(tr("Command blocks"), m_settingsBox);
    form->addRow(m_pvp);
    form->addRow(m_hardcore);
    form->addRow(m_allowFlight);
    form->addRow(m_commandBlocks);

    m_maxPlayers = new QSpinBox(m_settingsBox);
    m_maxPlayers->setRange(2, 100);
    form->addRow(tr("Max players:"), m_maxPlayers);
    m_viewDistance = new QSpinBox(m_settingsBox);
    m_viewDistance->setRange(3, 32);
    m_viewDistance->setSuffix(tr(" chunks"));
    form->addRow(tr("View distance:"), m_viewDistance);
    m_memory = new QSpinBox(m_settingsBox);
    m_memory->setRange(512, 32768);
    m_memory->setSingleStep(512);
    m_memory->setSuffix(tr(" MB"));
    form->addRow(tr("Server memory:"), m_memory);

    m_motd = new QLineEdit(m_settingsBox);
    m_motd->setPlaceholderText(tr("Vellora server"));
    form->addRow(tr("Description:"), m_motd);
    m_seed = new QLineEdit(m_settingsBox);
    m_seed->setPlaceholderText(tr("random"));
    form->addRow(tr("World seed:"), m_seed);

    m_whitelist = new QCheckBox(tr("Only my friends (white list)"), m_settingsBox);
    form->addRow(m_whitelist);
    m_whitelistNames = new QLineEdit(m_settingsBox);
    m_whitelistNames->setPlaceholderText(tr("Nicknames separated by commas"));
    form->addRow(tr("Friends:"), m_whitelistNames);
    connect(m_whitelist, &QCheckBox::toggled, m_whitelistNames, &QWidget::setEnabled);

    m_internet = new QCheckBox(tr("Open to the internet"), m_settingsBox);
    m_internet->setToolTip(tr("Through the free bore.pub tunnel: no port forwarding and no account needed"));
    form->addRow(m_internet);
    m_eula = new QCheckBox(m_settingsBox);
    auto* eulaText = new QLabel(tr("I accept the <a href=\"https://aka.ms/MinecraftEULA\">Minecraft EULA</a>"), m_settingsBox);
    eulaText->setOpenExternalLinks(true);
    auto* eulaRow = new QHBoxLayout;
    eulaRow->addWidget(m_eula);
    eulaRow->addWidget(eulaText, 1);
    form->addRow(eulaRow);

    auto* note = new QLabel(tr("Changes apply on the next start of the server. Players need a licensed Minecraft, like in the launcher."),
                            m_settingsBox);
    note->setWordWrap(true);
    note->setObjectName("mutedLabel");
    form->addRow(note);

    connect(m_instance, &QComboBox::currentIndexChanged, this, &ServerWindow::loadSettings);
    scroll->setWidget(m_settingsBox);
    return scroll;
}

QWidget* ServerWindow::buildStatus()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);

    auto* top = new QHBoxLayout;
    m_startButton = new QPushButton(page);
    m_startButton->setObjectName("accentButton");
    m_startButton->setMinimumHeight(40);
    top->addWidget(m_startButton, 1);
    auto* folder = new QPushButton(tr("Server folder"), page);
    top->addWidget(folder);
    layout->addLayout(top);

    m_status = new QLabel(page);
    m_status->setWordWrap(true);
    m_status->setTextFormat(Qt::RichText);
    layout->addWidget(m_status);

    m_inviteBox = new QFrame(page);
    m_inviteBox->setObjectName("lobbyAddress");
    m_inviteBox->setFrameShape(QFrame::StyledPanel);
    auto* card = new QVBoxLayout(m_inviteBox);
    card->addWidget(new QLabel(tr("Invite code for your friends:"), m_inviteBox));
    m_code = new QLabel(m_inviteBox);
    m_code->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QFont big = m_code->font();
    big.setPointSizeF(big.pointSizeF() * 1.8);
    big.setBold(true);
    m_code->setFont(big);
    card->addWidget(m_code);
    m_addresses = new QLabel(m_inviteBox);
    m_addresses->setWordWrap(true);
    m_addresses->setTextInteractionFlags(Qt::TextSelectableByMouse);
    card->addWidget(m_addresses);
    auto* buttons = new QHBoxLayout;
    m_copyCode = new QPushButton(tr("Copy code"), m_inviteBox);
    m_copyCode->setObjectName("accentButton");
    m_copyInvite = new QPushButton(tr("Copy invitation"), m_inviteBox);
    buttons->addWidget(m_copyCode);
    buttons->addWidget(m_copyInvite);
    buttons->addStretch(1);
    card->addLayout(buttons);
    layout->addWidget(m_inviteBox);

    auto* lists = new QHBoxLayout;
    auto* playersColumn = new QVBoxLayout;
    playersColumn->addWidget(new QLabel(tr("Players online:"), page));
    m_players = new QListWidget(page);
    m_players->setMaximumWidth(220);
    playersColumn->addWidget(m_players);
    lists->addLayout(playersColumn);

    auto* consoleColumn = new QVBoxLayout;
    consoleColumn->addWidget(new QLabel(tr("Console:"), page));
    m_console = new QPlainTextEdit(page);
    m_console->setReadOnly(true);
    m_console->setMaximumBlockCount(3000);
    QFont mono("Monospace");
    mono.setStyleHint(QFont::TypeWriter);
    m_console->setFont(mono);
    consoleColumn->addWidget(m_console, 1);
    m_command = new QLineEdit(page);
    m_command->setPlaceholderText(tr("Server command, e.g. op Steve, time set day, gamerule keepInventory true"));
    consoleColumn->addWidget(m_command);
    lists->addLayout(consoleColumn, 1);
    layout->addLayout(lists, 1);

    connect(m_startButton, &QPushButton::clicked, this, &ServerWindow::startOrStop);
    connect(folder, &QPushButton::clicked, this, [this] {
        const QString dir = ServerManager::serverDir(m_instance->currentData().toString());
        QDir().mkpath(dir);
        DesktopServices::openPath(dir);
    });
    connect(m_command, &QLineEdit::returnPressed, this, [this] {
        m_manager->sendCommand(m_command->text());
        m_command->clear();
    });
    connect(m_copyCode, &QPushButton::clicked, this, [this] {
        QApplication::clipboard()->setText(m_manager->inviteCode());
        m_copyCode->setText(tr("Copied!"));
    });
    connect(m_copyInvite, &QPushButton::clicked, this, [this] {
        QApplication::clipboard()->setText(invitation());
        m_copyInvite->setText(tr("Copied!"));
    });
    return page;
}

void ServerWindow::loadSettings()
{
    const QString id = m_instance->currentData().toString();
    if (id.isEmpty()) {
        m_instanceInfo->setText(tr("Create an instance first, then come back here."));
        return;
    }
    const auto s = ServerManager::loadSettings(id);
    m_gamemode->setCurrentIndex(qMax(0, m_gamemode->findData(s.gamemode)));
    m_difficulty->setCurrentIndex(qMax(0, m_difficulty->findData(s.difficulty)));
    m_pvp->setChecked(s.pvp);
    m_hardcore->setChecked(s.hardcore);
    m_allowFlight->setChecked(s.allowFlight);
    m_commandBlocks->setChecked(s.commandBlocks);
    m_maxPlayers->setValue(s.maxPlayers);
    m_viewDistance->setValue(s.viewDistance);
    m_memory->setValue(s.memoryMb);
    m_motd->setText(s.motd);
    m_seed->setText(s.seed);
    m_whitelist->setChecked(s.whitelist);
    m_whitelistNames->setText(s.whitelistNames.join(", "));
    m_whitelistNames->setEnabled(s.whitelist);
    m_internet->setChecked(s.internet);
    m_eula->setChecked(QFile::exists(FS::PathCombine(ServerManager::serverDir(id), "eula.txt")));

    if (auto* instance = dynamic_cast<MinecraftInstance*>(APPLICATION->instances()->getInstanceById(id))) {
        const auto info = Lobby::describe(instance);
        m_instanceInfo->setText(info.loader.isEmpty()
                                    ? tr("Minecraft %1 without mods: the vanilla server.").arg(info.minecraftVersion)
                                    : tr("Minecraft %1 with %2 %3: the server gets the same loader and the mods of the instance "
                                         "(client-only mods like Sodium are left out).")
                                          .arg(info.minecraftVersion, info.loaderName, info.loaderVersion));
    }
}

ServerSettings ServerWindow::collectSettings() const
{
    ServerSettings s;
    s.instanceId = m_instance->currentData().toString();
    s.gamemode = m_gamemode->currentData().toString();
    s.difficulty = m_difficulty->currentData().toString();
    s.pvp = m_pvp->isChecked();
    s.hardcore = m_hardcore->isChecked();
    s.allowFlight = m_allowFlight->isChecked();
    s.commandBlocks = m_commandBlocks->isChecked();
    s.maxPlayers = m_maxPlayers->value();
    s.viewDistance = m_viewDistance->value();
    s.memoryMb = m_memory->value();
    s.motd = m_motd->text().trimmed();
    s.seed = m_seed->text().trimmed();
    s.whitelist = m_whitelist->isChecked();
    for (const auto& name : m_whitelistNames->text().split(QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts)) {
        s.whitelistNames << name;
    }
    s.internet = m_internet->isChecked();
    return s;
}

void ServerWindow::startOrStop()
{
    if (m_manager->isActive()) {
        m_manager->stop();
        return;
    }
    if (m_instance->currentData().toString().isEmpty()) {
        return;
    }
    if (!m_eula->isChecked()) {
        QMessageBox::information(this, tr("Minecraft EULA"), tr("A Minecraft server can only be started after accepting the Minecraft EULA."));
        return;
    }
    m_console->clear();
    m_manager->start(collectSettings());
}

void ServerWindow::updateState()
{
    const auto state = m_manager->state();
    const bool active = m_manager->isActive();
    m_settingsBox->setEnabled(!active);
    switch (state) {
        case ServerManager::State::Stopped:
            m_startButton->setText(tr("Start the server"));
            break;
        case ServerManager::State::Preparing:
        case ServerManager::State::Starting:
            m_startButton->setText(tr("Cancel"));
            break;
        case ServerManager::State::Running:
            m_startButton->setText(tr("Stop the server"));
            break;
        case ServerManager::State::Stopping:
            m_startButton->setText(tr("Stopping..."));
            break;
    }
    m_startButton->setEnabled(state != ServerManager::State::Stopping);
    m_command->setEnabled(state == ServerManager::State::Running);
    if (!active && m_status->text().isEmpty()) {
        m_status->setText(tr("Set up the server on the left and press <b>Start the server</b>. You get an invite code: send it to your friends, "
                             "they enter it in <i>Lobby → Join a friend</i> and connect."));
    }
}

void ServerWindow::updateAddresses()
{
    const QString code = m_manager->inviteCode();
    m_inviteBox->setVisible(!code.isEmpty());
    m_code->setText(code);
    QStringList lines;
    if (!m_manager->publicAddress().isEmpty()) {
        lines << tr("Over the internet: <b>%1</b>").arg(m_manager->publicAddress());
    }
    if (!m_manager->lanAddress().isEmpty()) {
        lines << tr("In the same network: <b>%1</b>").arg(m_manager->lanAddress());
    }
    m_addresses->setText(lines.join("<br>"));
    m_copyCode->setText(tr("Copy code"));
    m_copyInvite->setText(tr("Copy invitation"));
}

QString ServerWindow::invitation() const
{
    const QString address = m_manager->publicAddress().isEmpty() ? m_manager->lanAddress() : m_manager->publicAddress();
    return tr("Join my Minecraft server!\nInvite code: %1\nIn Vellora Client: Lobby → Join a friend, enter the code. "
              "Or in Minecraft: Multiplayer → Direct Connection → %2")
        .arg(m_manager->inviteCode(), address);
}
