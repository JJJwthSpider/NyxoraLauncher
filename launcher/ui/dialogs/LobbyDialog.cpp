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
#include "LobbyDialog.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkInterface>
#include <QPushButton>
#include <QRegularExpression>
#include <QTabWidget>
#include <QVBoxLayout>

#include "Application.h"
#include "BaseInstance.h"
#include "InstanceList.h"
#include "icons/IconList.h"
#include "launch/LaunchTask.h"
#include "launch/LogModel.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/launch/MinecraftTarget.h"
#include "modplatform/lobby/TunnelModInstaller.h"
#include "modplatform/server/InviteCode.h"

namespace {
// the address e4mc gives out, e.g. "brave-creeper.eu.e4mc.link"
const QRegularExpression s_tunnelAddress(R"(([a-z0-9-]+(?:\.[a-z0-9-]+)*\.e4mc\.link))");
// "Started serving on 51234" (integrated server) or the chat line "Local game hosted on port 51234"
const QRegularExpression s_lanPort(R"((?:Started serving on|hosted on port) (\d{2,5}))");

QString localAddress()
{
    for (const auto& address : QNetworkInterface::allAddresses()) {
        const bool privateUse = address.isInSubnet(QHostAddress("10.0.0.0"), 8) || address.isInSubnet(QHostAddress("172.16.0.0"), 12) ||
                                address.isInSubnet(QHostAddress("192.168.0.0"), 16);
        if (address.protocol() == QAbstractSocket::IPv4Protocol && privateUse) {
            return address.toString();
        }
    }
    return {};
}

QLabel* heading(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    QFont font = label->font();
    font.setBold(true);
    font.setPointSizeF(font.pointSizeF() * 1.15);
    label->setFont(font);
    return label;
}

QLabel* paragraph(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setTextFormat(Qt::RichText);
    return label;
}
}  // namespace

LobbyDialog::LobbyDialog(BaseInstance* preselected, QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Lobby: play with friends"));
    setAttribute(Qt::WA_DeleteOnClose);
    resize(720, 560);

    auto* layout = new QVBoxLayout(this);
    auto* tabs = new QTabWidget(this);
    tabs->addTab(buildHostTab(preselected), tr("Create a lobby"));
    tabs->addTab(buildJoinTab(preselected), tr("Join a friend"));
    layout->addWidget(tabs);

    auto* close = new QPushButton(tr("Close"), this);
    connect(close, &QPushButton::clicked, this, &QDialog::close);
    auto* bottom = new QHBoxLayout;
    bottom->addStretch(1);
    bottom->addWidget(close);
    layout->addLayout(bottom);
}

void LobbyDialog::fillInstances(QComboBox* combo, BaseInstance* preselected)
{
    auto* instances = APPLICATION->instances();
    for (int i = 0; i < instances->count(); i++) {
        auto* instance = instances->at(i);
        if (!dynamic_cast<MinecraftInstance*>(instance)) {
            continue;
        }
        combo->addItem(APPLICATION->icons()->getIcon(instance->iconKey()), instance->name(), instance->id());
        if (instance == preselected) {
            combo->setCurrentIndex(combo->count() - 1);
        }
    }
}

BaseInstance* LobbyDialog::instanceOf(const QComboBox* combo)
{
    return APPLICATION->instances()->getInstanceById(combo->currentData().toString());
}

QWidget* LobbyDialog::buildHostTab(BaseInstance* preselected)
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(10);

    layout->addWidget(heading(tr("Your world, your friends"), page));
    layout->addWidget(paragraph(tr("Choose the instance you want to play. The launcher prepares it, starts the game and shows the address "
                                   "for your friends as soon as you open your world."),
                                page));

    auto* row = new QHBoxLayout;
    m_hostInstance = new QComboBox(page);
    fillInstances(m_hostInstance, preselected);
    row->addWidget(m_hostInstance, 1);
    m_createButton = new QPushButton(tr("Create lobby"), page);
    m_createButton->setObjectName("accentButton");
    row->addWidget(m_createButton);
    layout->addLayout(row);

    m_hostInfo = paragraph(QString(), page);
    layout->addWidget(m_hostInfo);

    m_hostStatus = paragraph(QString(), page);
    m_hostStatus->setObjectName("lobbyStatus");
    layout->addWidget(m_hostStatus);

    // the address card, shown once the game reported it
    m_addressBox = new QFrame(page);
    m_addressBox->setObjectName("lobbyAddress");
    m_addressBox->setFrameShape(QFrame::StyledPanel);
    auto* card = new QVBoxLayout(m_addressBox);
    card->addWidget(new QLabel(tr("Address for your friends:"), m_addressBox));
    m_addressLabel = new QLabel(m_addressBox);
    m_addressLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QFont big = m_addressLabel->font();
    big.setPointSizeF(big.pointSizeF() * 1.6);
    big.setBold(true);
    m_addressLabel->setFont(big);
    card->addWidget(m_addressLabel);
    m_lanLabel = paragraph(QString(), m_addressBox);
    card->addWidget(m_lanLabel);
    auto* buttons = new QHBoxLayout;
    m_copyAddress = new QPushButton(tr("Copy address"), m_addressBox);
    m_copyAddress->setObjectName("accentButton");
    m_copyInvite = new QPushButton(tr("Copy invitation"), m_addressBox);
    buttons->addWidget(m_copyAddress);
    buttons->addWidget(m_copyInvite);
    buttons->addStretch(1);
    card->addLayout(buttons);
    m_addressBox->hide();
    layout->addWidget(m_addressBox);

    auto* exportRow = new QHBoxLayout;
    auto* exportHint = paragraph(tr("Friends need the same mods. Send them your pack:"), page);
    auto* exportButton = new QPushButton(tr("Save the pack for friends (.mrpack)..."), page);
    exportRow->addWidget(exportHint, 1);
    exportRow->addWidget(exportButton);
    layout->addLayout(exportRow);

    auto* serverRow = new QHBoxLayout;
    serverRow->addWidget(paragraph(tr("Want a real server with settings like PvP, game mode and a white list, and an invite code for friends?"),
                                   page),
                         1);
    auto* serverButton = new QPushButton(tr("My server..."), page);
    serverRow->addWidget(serverButton);
    layout->addLayout(serverRow);
    connect(serverButton, &QPushButton::clicked, this, [this] {
        emit serverRequested();
        close();
    });
    layout->addStretch(1);

    connect(m_hostInstance, &QComboBox::currentIndexChanged, this, &LobbyDialog::updateHostInfo);
    connect(m_createButton, &QPushButton::clicked, this, &LobbyDialog::createLobby);
    connect(m_copyAddress, &QPushButton::clicked, this, [this] {
        QApplication::clipboard()->setText(m_internetAddress.isEmpty() ? m_lanAddress : m_internetAddress);
        m_copyAddress->setText(tr("Copied!"));
    });
    connect(m_copyInvite, &QPushButton::clicked, this, [this] {
        QApplication::clipboard()->setText(invitation());
        m_copyInvite->setText(tr("Copied!"));
    });
    connect(exportButton, &QPushButton::clicked, this, [this] {
        if (auto* instance = instanceOf(m_hostInstance)) {
            emit exportRequested(instance);
        }
    });

    m_createButton->setEnabled(m_hostInstance->count() > 0);
    if (m_hostInstance->count() == 0) {
        m_hostInfo->setText(tr("Create an instance first, then come back here."));
    } else {
        updateHostInfo();
    }
    return page;
}

void LobbyDialog::updateHostInfo()
{
    auto* instance = dynamic_cast<MinecraftInstance*>(instanceOf(m_hostInstance));
    if (!instance) {
        return;
    }
    const auto info = Lobby::describe(instance);
    if (info.loader.isEmpty()) {
        m_hostInfo->setText(tr("<b>Minecraft %1 without mods:</b> friends can join only from the same network (the same Wi-Fi). "
                               "To play over the internet choose an instance with Fabric, Quilt, Forge or NeoForge.")
                                .arg(info.minecraftVersion));
    } else if (Lobby::hasTunnelMod(instance)) {
        m_hostInfo->setText(tr("<b>Minecraft %1 with %2:</b> ready for playing over the internet (e4mc is installed).")
                                .arg(info.minecraftVersion, info.loaderName));
    } else {
        m_hostInfo->setText(tr("<b>Minecraft %1 with %2:</b> the launcher will add the small e4mc mod. It gives your world an "
                               "internet address, no port forwarding and no account needed. Only you need it, not your friends.")
                                .arg(info.minecraftVersion, info.loaderName));
    }
}

void LobbyDialog::setStatus(const QString& text)
{
    m_hostStatus->setText(text);
}

void LobbyDialog::createLobby()
{
    auto* instance = dynamic_cast<MinecraftInstance*>(instanceOf(m_hostInstance));
    if (!instance) {
        return;
    }
    m_internetAddress.clear();
    m_lanAddress.clear();
    m_addressBox->hide();
    m_createButton->setEnabled(false);
    m_hostInstance->setEnabled(false);

    const auto info = Lobby::describe(instance);
    if (info.loader.isEmpty() || Lobby::hasTunnelMod(instance)) {
        startGame();
        return;
    }
    if (!m_installer) {
        m_installer = new Lobby::TunnelModInstaller(this);
        connect(m_installer, &Lobby::TunnelModInstaller::status, this, &LobbyDialog::setStatus);
        connect(m_installer, &Lobby::TunnelModInstaller::finished, this, [this](bool ok, const QString& message) {
            if (ok) {
                updateHostInfo();
                startGame();
                return;
            }
            setStatus(tr("<b>The internet lobby could not be prepared:</b> %1<br>You can still play with friends in the same network: "
                         "press \"Create lobby\" again.")
                          .arg(message));
            m_createButton->setEnabled(true);
            m_hostInstance->setEnabled(true);
        });
    }
    m_installer->install(instance);
}

void LobbyDialog::startGame()
{
    auto* instance = instanceOf(m_hostInstance);
    if (!instance) {
        return;
    }
    m_running = instance;
    connect(instance, &BaseInstance::launchTaskChanged, this, &LobbyDialog::attachLog, Qt::UniqueConnection);
    disconnect(m_runningConnection);
    m_runningConnection = connect(instance, &BaseInstance::runningStatusChanged, this, [this](bool running) {
        if (!running) {
            setStatus(tr("The game is closed. The lobby ended."));
            m_createButton->setEnabled(true);
            m_hostInstance->setEnabled(true);
        }
    });
    if (!instance->isRunning()) {
        APPLICATION->launch(instance);
    }
    // the launch can still be cancelled (account, demo question...), so the lobby can always be started again
    m_createButton->setEnabled(true);
    m_hostInstance->setEnabled(true);
    attachLog(instance->getLaunchTask());
    setStatus(tr("<b>The game is starting.</b> Then:<ol><li>open your world (or create a new one);</li>"
                 "<li>press <b>Esc</b> and choose <b>Open to LAN</b>;</li><li>press <b>Start LAN World</b>.</li></ol>"
                 "The address for your friends appears here by itself."));
}

void LobbyDialog::attachLog(LaunchTask* task)
{
    disconnect(m_logConnection);
    if (!task) {
        return;
    }
    auto model = task->getLogModel();
    if (!model) {
        return;
    }
    for (int row = 0; row < model->rowCount(); row++) {
        scanLine(model->data(model->index(row, 0), Qt::DisplayRole).toString());
    }
    auto* raw = model.get();
    m_logConnection = connect(raw, &QAbstractItemModel::rowsInserted, this, [this, raw](const QModelIndex&, int first, int last) {
        for (int row = first; row <= last; row++) {
            scanLine(raw->data(raw->index(row, 0), Qt::DisplayRole).toString());
        }
    });
}

void LobbyDialog::scanLine(const QString& line)
{
    bool changed = false;
    if (const auto match = s_tunnelAddress.match(line); match.hasMatch() && m_internetAddress != match.captured(1)) {
        m_internetAddress = match.captured(1);
        changed = true;
    }
    if (const auto match = s_lanPort.match(line); match.hasMatch()) {
        const QString ip = localAddress();
        const QString lan = (ip.isEmpty() ? QStringLiteral("localhost") : ip) + ":" + match.captured(1);
        if (lan != m_lanAddress) {
            m_lanAddress = lan;
            changed = true;
        }
    }
    if (!changed) {
        return;
    }
    m_addressLabel->setText(m_internetAddress.isEmpty() ? m_lanAddress : m_internetAddress);
    m_lanLabel->setText(m_internetAddress.isEmpty()
                            ? tr("This address works for friends in the same network (Wi-Fi).")
                            : (m_lanAddress.isEmpty() ? tr("Works over the internet.")
                                                      : tr("Works over the internet. In the same network friends can also use %1.").arg(m_lanAddress)));
    m_copyAddress->setText(tr("Copy address"));
    m_copyInvite->setText(tr("Copy invitation"));
    m_addressBox->show();
    setStatus(tr("<b>The lobby is open!</b> Send the address to your friends. They join with <i>Lobby → Join a friend</i> "
                 "or in Minecraft with <i>Multiplayer → Direct Connection</i>."));
}

QString LobbyDialog::invitation() const
{
    QString version;
    if (auto* instance = dynamic_cast<MinecraftInstance*>(m_running.data())) {
        const auto info = Lobby::describe(instance);
        version = info.loader.isEmpty() ? info.minecraftVersion : info.minecraftVersion + " " + info.loaderName;
    }
    return tr("Join me in Minecraft %1!\nAddress: %2\nIn Vellora Client: Lobby → Join a friend, paste the address. "
              "In Minecraft: Multiplayer → Direct Connection.")
        .arg(version, m_internetAddress.isEmpty() ? m_lanAddress : m_internetAddress);
}

QWidget* LobbyDialog::buildJoinTab(BaseInstance* preselected)
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setSpacing(10);

    layout->addWidget(heading(tr("Join a friend's world"), page));
    layout->addWidget(paragraph(tr("Enter the invite code or the address your friend sent you and choose the instance with the same pack. "
                                   "The game starts and connects right away."),
                                page));

    m_joinAddress = new QLineEdit(page);
    m_joinAddress->setPlaceholderText(tr("Invite code like LL-8N4Q-2ZKD-1X, or an address like brave-creeper.e4mc.link"));
    m_joinAddress->setClearButtonEnabled(true);
    layout->addWidget(m_joinAddress);
    m_joinInfo = paragraph(QString(), page);
    layout->addWidget(m_joinInfo);
    connect(m_joinAddress, &QLineEdit::textChanged, this, [this] {
        QString explanation;
        targetAddress(&explanation);
        m_joinInfo->setText(explanation);
    });

    auto* row = new QHBoxLayout;
    m_joinInstance = new QComboBox(page);
    fillInstances(m_joinInstance, preselected);
    row->addWidget(m_joinInstance, 1);
    auto* joinButton = new QPushButton(tr("Connect"), page);
    joinButton->setObjectName("accentButton");
    row->addWidget(joinButton);
    layout->addLayout(row);

    auto* importRow = new QHBoxLayout;
    importRow->addWidget(paragraph(tr("No such pack yet? Import the .mrpack file your friend sent:"), page), 1);
    auto* importButton = new QPushButton(tr("Import a pack from a friend..."), page);
    importRow->addWidget(importButton);
    layout->addLayout(importRow);
    layout->addStretch(1);

    // an address on the clipboard is pasted for the user
    const QString clip = QApplication::clipboard()->text().trimmed();
    QString codeHost;
    quint16 codePort = 0;
    if (s_tunnelAddress.match(clip).hasMatch() || QRegularExpression(R"(^\d{1,3}(\.\d{1,3}){3}:\d{2,5}$)").match(clip).hasMatch() ||
        InviteCode::decode(clip, codeHost, codePort)) {
        m_joinAddress->setText(clip);
    }

    connect(joinButton, &QPushButton::clicked, this, &LobbyDialog::join);
    connect(m_joinAddress, &QLineEdit::returnPressed, this, &LobbyDialog::join);
    connect(importButton, &QPushButton::clicked, this, &LobbyDialog::importRequested);
    joinButton->setEnabled(m_joinInstance->count() > 0);
    return page;
}

QString LobbyDialog::targetAddress(QString* explanation) const
{
    const QString text = m_joinAddress->text().trimmed();
    QString host;
    quint16 port = 0;
    // an invite code, maybe inside a pasted invitation
    static const QRegularExpression code(R"((?:LL|SC)-[0-9A-Za-z]{1,4}(?:-[0-9A-Za-z]{1,4})+)", QRegularExpression::CaseInsensitiveOption);
    const auto codeMatch = code.match(text);
    if (InviteCode::decode(codeMatch.hasMatch() ? codeMatch.captured(0) : text, host, port)) {
        if (explanation) {
            *explanation = tr("Invite code accepted. Server: <b>%1</b>, port: <b>%2</b>").arg(host).arg(port);
        }
        return QString("%1:%2").arg(host).arg(port);
    }
    if (text.startsWith("LL-", Qt::CaseInsensitive) || text.startsWith("SC-", Qt::CaseInsensitive)) {
        if (explanation) {
            *explanation = tr("This invite code is not complete or has a typo.");
        }
        return {};
    }
    // the whole invitation may have been pasted: take the address out of it
    if (const auto match = s_tunnelAddress.match(text); match.hasMatch()) {
        return match.captured(1);
    }
    if (explanation) {
        explanation->clear();
    }
    return text;
}

void LobbyDialog::join()
{
    const QString address = targetAddress();
    auto* instance = instanceOf(m_joinInstance);
    if (address.isEmpty() || !instance) {
        m_joinAddress->setFocus();
        return;
    }
    auto target = std::make_shared<MinecraftTarget>(MinecraftTarget::parse(address, false));
    APPLICATION->launch(instance, LaunchMode::Normal, target);
    close();
}
