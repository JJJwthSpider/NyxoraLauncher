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
#include "ChangeNameDialog.h"

#include <QDialogButtonBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

#include "Application.h"
#include "minecraft/auth/AccountList.h"

namespace {
const QString s_profileApi = QStringLiteral("https://api.minecraftservices.com/minecraft/profile/name/");
// Minecraft names: 3 to 16 letters, digits and underscores
const QRegularExpression s_validName(QStringLiteral("^[A-Za-z0-9_]{3,16}$"));
}  // namespace

ChangeNameDialog::ChangeNameDialog(MinecraftAccountPtr account, QWidget* parent) : QDialog(parent), m_account(std::move(account))
{
    setWindowTitle(tr("Change nickname"));
    setMinimumWidth(440);

    auto* layout = new QVBoxLayout(this);
    auto* current = new QLabel(tr("Current nickname: <b>%1</b>").arg(m_account->profileName().toHtmlEscaped()), this);
    layout->addWidget(current);

    auto* hint =
        new QLabel(tr("3 to 16 characters: Latin letters, digits and _. Mojang allows changing the name once every 30 days."), this);
    hint->setObjectName("hintLabel");
    hint->setWordWrap(true);
    layout->addWidget(hint);

    m_name = new QLineEdit(this);
    m_name->setMaxLength(16);
    m_name->setPlaceholderText(tr("New nickname"));
    m_name->setValidator(new QRegularExpressionValidator(QRegularExpression("[A-Za-z0-9_]{0,16}"), m_name));
    m_name->setMinimumHeight(34);
    layout->addWidget(m_name);

    m_status = new QLabel(this);
    m_status->setWordWrap(true);
    m_status->setTextFormat(Qt::RichText);
    layout->addWidget(m_status);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    m_applyButton = buttons->addButton(tr("Change nickname"), QDialogButtonBox::AcceptRole);
    m_applyButton->setObjectName("accentButton");
    m_applyButton->setEnabled(false);
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_applyButton, &QPushButton::clicked, this, &ChangeNameDialog::apply);
    connect(m_name, &QLineEdit::textEdited, this, &ChangeNameDialog::nameEdited);

    // ask the server only when the user stops typing for a moment
    m_checkTimer.setSingleShot(true);
    m_checkTimer.setInterval(500);
    connect(&m_checkTimer, &QTimer::timeout, this, &ChangeNameDialog::checkAvailability);
}

QNetworkRequest ChangeNameDialog::request(const QString& url) const
{
    QNetworkRequest req{ QUrl(url) };
    req.setRawHeader("Authorization", QString("Bearer %1").arg(m_account->accessToken()).toUtf8());
    req.setRawHeader("Accept", "application/json");
    req.setHeader(QNetworkRequest::UserAgentHeader, APPLICATION->getUserAgent());
    return req;
}

void ChangeNameDialog::setStatus(const QString& html, bool canApply)
{
    m_status->setText(html);
    m_applyButton->setEnabled(canApply);
}

void ChangeNameDialog::nameEdited()
{
    if (m_checkReply) {
        m_checkReply->abort();
    }
    const auto name = m_name->text();
    if (name.isEmpty()) {
        setStatus({}, false);
        return;
    }
    if (!s_validName.match(name).hasMatch()) {
        setStatus(tr("The name must be 3 to 16 characters long."), false);
        return;
    }
    if (name == m_account->profileName()) {
        setStatus(tr("This is already your nickname."), false);
        return;
    }
    setStatus(tr("Checking whether the name is free..."), false);
    m_checkTimer.start();
}

void ChangeNameDialog::checkAvailability()
{
    const auto name = m_name->text();
    m_checkReply = APPLICATION->network()->get(request(s_profileApi + name + "/available"));
    connect(m_checkReply.data(), &QNetworkReply::finished, this, [this, reply = m_checkReply.data(), name] {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::OperationCanceledError || name != m_name->text()) {
            return;  // the user typed on
        }
        const auto status = QJsonDocument::fromJson(reply->readAll()).object().value("status").toString();
        if (status == "AVAILABLE") {
            setStatus(tr("<b>%1</b> is free.").arg(name), true);
        } else if (status == "DUPLICATE") {
            setStatus(tr("<b>%1</b> is already taken.").arg(name), false);
        } else if (status == "NOT_ALLOWED") {
            setStatus(tr("<b>%1</b> is not allowed.").arg(name), false);
        } else if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 401) {
            setStatus(tr("The login has expired. Refresh the account in the account settings and try again."), false);
        } else {
            setStatus(tr("Could not check the name: %1").arg(reply->errorString().toHtmlEscaped()), false);
        }
    });
}

void ChangeNameDialog::apply()
{
    const auto name = m_name->text();
    m_checkTimer.stop();
    setStatus(tr("Changing the nickname..."), false);
    m_name->setEnabled(false);
    m_changeReply = APPLICATION->network()->put(request(s_profileApi + name), QByteArray());
    connect(m_changeReply.data(), &QNetworkReply::finished, this, [this, reply = m_changeReply.data()] {
        reply->deleteLater();
        m_name->setEnabled(true);
        const int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (code == 200) {
            // load the new profile name into the launcher
            APPLICATION->accounts()->requestRefresh(m_account->internalId());
            accept();
            return;
        }
        QString reason;
        switch (code) {
            case 400:
                reason = tr("The name is not valid.");
                break;
            case 401:
                reason = tr("The login has expired. Refresh the account in the account settings and try again.");
                break;
            case 403:
                reason = tr("The name cannot be changed right now. Mojang allows one change every 30 days.");
                break;
            case 429:
                reason = tr("Too many attempts. Wait a minute and try again.");
                break;
            default:
                reason = reply->errorString().toHtmlEscaped();
        }
        setStatus(tr("<b>The nickname was not changed.</b> %1").arg(reason), true);
    });
}
