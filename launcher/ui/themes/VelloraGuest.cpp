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
#include "VelloraGuest.h"

#include <QDir>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSettings>

#include "Application.h"
#include "minecraft/auth/AccountList.h"
#include "ui/themes/VelloraSound.h"
#include "ui/widgets/VelloraSkinGen.h"

namespace {
QSettings settings()
{
    return QSettings(QDir(APPLICATION->dataRoot()).filePath("vellora-ui.ini"), QSettings::IniFormat);
}
}  // namespace

namespace VelloraGuest {
bool aiAllowed()
{
    auto* accounts = APPLICATION->accounts();
    for (int i = 0; i < accounts->count(); i++) {
        if (accounts->at(i)->accountType() == AccountType::MSA) {
            return true;
        }
    }
    return false;
}

QString name()
{
    return settings().value("guestName").toString();
}

void setName(const QString& name)
{
    settings().setValue("guestName", name);
}

QString randomName()
{
    return QString("Player%1").arg(1000 + QRandomGenerator::global()->bounded(9000));
}

bool validName(const QString& name)
{
    static const QRegularExpression nick("^[A-Za-z0-9_]{3,16}$");
    return nick.match(name).hasMatch();
}

QImage skin(bool* slim)
{
    const QString nick = name();
    if (nick.isEmpty()) {
        return {};
    }
    const auto own = SkinGen::skins(QString(), 1, qint64(qHash(nick)));
    if (own.isEmpty()) {
        return {};
    }
    if (slim) {
        *slim = own.first().slim;
    }
    return own.first().image;
}

bool askName(QWidget* parent)
{
    bool ok = false;
    const QString current = name().isEmpty() ? randomName() : name();
    const QString typed = QInputDialog::getText(parent, QObject::tr("Guest"),
                                                QObject::tr("The guest's nickname: 3 to 16 Latin letters, digits or _.\n"
                                                            "A guest has any number of instances, any mods and versions, the wardrobe "
                                                            "and the demo of the game, for free; the AI and the full game come with a "
                                                            "Microsoft account."),
                                                QLineEdit::Normal, current, &ok)
                              .trimmed();
    if (!ok) {
        return false;
    }
    if (!validName(typed)) {
        QMessageBox::warning(parent, QObject::tr("Guest"), QObject::tr("A nickname is 3 to 16 Latin letters, digits or _."));
        return false;
    }
    setName(typed);
    VelloraSound::play("open");
    return true;
}

bool requireAi(QWidget* parent)
{
    if (aiAllowed()) {
        return true;
    }
    VelloraSound::play("open");
    QMessageBox box(parent);
    box.setWindowTitle(QObject::tr("The AI needs a sign-in"));
    box.setText(QObject::tr("Arkan and the AI skin and cape generators work with a Microsoft account. A guest has everything else: "
                            "instances, versions, mods, the catalog and the wardrobe."));
    auto* signIn = box.addButton(QObject::tr("Sign in with Microsoft"), QMessageBox::AcceptRole);
    box.addButton(QObject::tr("Later"), QMessageBox::RejectRole);
    box.exec();
    if (box.clickedButton() == signIn) {
        APPLICATION->ShowGlobalSettings(parent, "accounts");
    }
    return aiAllowed();
}

bool playsDemo()
{
    if (name().isEmpty()) {
        return false;
    }
    auto* accounts = APPLICATION->accounts();
    for (int i = 0; i < accounts->count(); i++) {
        if (accounts->at(i)->ownsMinecraft()) {
            return false;  // an account that bought the game is here: Prism's own way (the full game)
        }
    }
    return true;
}

bool demoWorks(const QString& version)
{
    if (version.startsWith("1.")) {
        // 1.3.1 and newer, pre-releases included ("1.20.1-pre1")
        const QStringList parts = version.section('-', 0, 0).split('.');
        const int minor = parts.value(1).toInt(), patch = parts.value(2).toInt();
        return minor > 3 || (minor == 3 && patch >= 1);
    }
    static const QRegularExpression snapshot("^(\\d\\d)w\\d\\d[a-z]$");
    if (const auto match = snapshot.match(version); match.hasMatch()) {
        return match.captured(1).toInt() >= 13;
    }
    static const QRegularExpression yearBased("^\\d{2}\\.\\d+");  // 26.1 and newer
    return yearBased.match(version).hasMatch();
}

void noDemo(QWidget* parent, const QString& version)
{
    QMessageBox::information(parent, QObject::tr("No demo in this version"),
                             QObject::tr("Minecraft %1 has no demo mode (it came in 1.3.1), so a guest cannot start it. Choose an instance "
                                         "on a newer version or sign in with the Microsoft account that bought Minecraft.")
                                 .arg(version));
}

bool confirmDemo(QWidget* parent)
{
    if (settings().value("guestDemoExplained", false).toBool()) {
        return true;
    }
    VelloraSound::play("open");
    QMessageBox box(parent);
    box.setWindowTitle(QObject::tr("A guest plays the demo"));
    box.setText(QObject::tr("Without a Microsoft account there is the demo of Minecraft: Java Edition, the official free trial from Mojang: "
                            "a world for 5 game days (about 100 minutes), then a new one can be started. The instance starts as it is, with "
                            "its mods, in the demo mode and with your nickname, %1.\n\n"
                            "The full game comes after signing in with the Microsoft account that bought Minecraft.")
                    .arg(name()));
    auto* play = box.addButton(QObject::tr("Play the demo"), QMessageBox::AcceptRole);
    auto* signIn = box.addButton(QObject::tr("Sign in with Microsoft"), QMessageBox::ActionRole);
    box.addButton(QObject::tr("Cancel"), QMessageBox::RejectRole);
    box.setDefaultButton(play);
    box.exec();
    if (box.clickedButton() == signIn) {
        APPLICATION->ShowGlobalSettings(parent, "accounts");
        return false;
    }
    if (box.clickedButton() != play) {
        return false;
    }
    settings().setValue("guestDemoExplained", true);
    return true;
}
}  // namespace VelloraGuest
