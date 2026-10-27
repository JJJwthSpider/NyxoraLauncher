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
#include "BoreTunnel.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

BoreTunnel::BoreTunnel(QObject* parent) : QObject(parent) {}

BoreTunnel::~BoreTunnel()
{
    close();
}

void BoreTunnel::send(QTcpSocket* socket, const QByteArray& json)
{
    socket->write(json);
    socket->write(QByteArray(1, '\0'));
}

void BoreTunnel::open(quint16 localPort, const QString& server, quint16 controlPort)
{
    close();
    m_server = server;
    m_controlPort = controlPort;
    m_localPort = localPort;
    m_publicPort = 0;
    m_buffer.clear();

    m_control = new QTcpSocket(this);
    connect(m_control, &QTcpSocket::connected, this, [this] { send(m_control, R"({"Hello":0})"); });
    connect(m_control, &QTcpSocket::readyRead, this, &BoreTunnel::readControl);
    connect(m_control, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        emit failed(tr("The tunnel server %1 can't be reached: %2").arg(m_server, m_control->errorString()));
    });
    connect(m_control, &QTcpSocket::disconnected, this, [this] {
        if (m_publicPort != 0) {
            emit failed(tr("The tunnel was closed by %1.").arg(m_server));
        }
        m_publicPort = 0;
    });
    m_control->connectToHost(m_server, m_controlPort);
}

void BoreTunnel::close()
{
    if (m_control) {
        m_control->disconnect(this);
        m_control->abort();
        m_control->deleteLater();
    }
    m_publicPort = 0;
}

void BoreTunnel::readControl()
{
    m_buffer += m_control->readAll();
    int end;
    while ((end = m_buffer.indexOf('\0')) >= 0) {
        const QByteArray message = m_buffer.left(end);
        m_buffer.remove(0, end + 1);
        handleMessage(message);
    }
}

void BoreTunnel::handleMessage(const QByteArray& json)
{
    const auto document = QJsonDocument::fromJson(json);
    if (!document.isObject()) {
        return;  // "Heartbeat" comes as a plain string
    }
    const auto object = document.object();
    if (object.contains("Hello")) {
        m_publicPort = quint16(object.value("Hello").toInt());
        emit opened(m_server, m_publicPort);
    } else if (object.contains("Connection")) {
        acceptConnection(object.value("Connection").toString());
    } else if (object.contains("Error")) {
        emit failed(object.value("Error").toString());
    } else if (object.contains("Challenge")) {
        emit failed(tr("The tunnel server %1 needs a secret.").arg(m_server));
    }
}

void BoreTunnel::acceptConnection(const QString& id)
{
    // one player: a connection to the tunnel server and one to the local Minecraft server, bytes copied both ways
    auto* remote = new QTcpSocket(this);
    auto* local = new QTcpSocket(this);
    auto pending = std::make_shared<QByteArray>();  // what the player sent before the local server answered

    auto closeBoth = [remote, local] {
        remote->disconnect();
        local->disconnect();
        remote->disconnectFromHost();
        local->disconnectFromHost();
        remote->deleteLater();
        local->deleteLater();
    };
    connect(remote, &QTcpSocket::connected, this, [remote, local, id, this] {
        send(remote, QJsonDocument(QJsonObject{ { "Accept", id } }).toJson(QJsonDocument::Compact));
        local->connectToHost(QStringLiteral("127.0.0.1"), m_localPort);
    });
    connect(local, &QTcpSocket::connected, this, [local, pending] {
        if (!pending->isEmpty()) {
            local->write(*pending);
            pending->clear();
        }
    });
    connect(remote, &QTcpSocket::readyRead, this, [remote, local, pending] {
        const QByteArray data = remote->readAll();
        if (local->state() == QAbstractSocket::ConnectedState) {
            local->write(data);
        } else {
            *pending += data;
        }
    });
    connect(local, &QTcpSocket::readyRead, this, [remote, local] { remote->write(local->readAll()); });
    connect(remote, &QTcpSocket::disconnected, this, closeBoth);
    connect(local, &QTcpSocket::disconnected, this, closeBoth);
    connect(remote, &QTcpSocket::errorOccurred, this, closeBoth);
    connect(local, &QTcpSocket::errorOccurred, this, closeBoth);
    remote->connectToHost(m_server, m_controlPort);
}
