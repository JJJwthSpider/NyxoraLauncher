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

#include <QByteArray>
#include <QObject>
#include <QPointer>
#include <QTcpSocket>

/// A client for bore (https://github.com/ekzhang/bore), a free TCP tunnel: the public bore server gives the local
/// Minecraft server a port on its own address, e.g. "bore.pub:41234", and forwards every player connection to it.
/// No account, no port forwarding on the router.
///
/// The protocol: JSON messages separated by a zero byte on a control connection to port 7835. The client says
/// {"Hello":0} and gets {"Hello":port}. For every player the server sends {"Connection":"<uuid>"}; the client opens a
/// new connection, answers {"Accept":"<uuid>"} and from then on copies bytes between it and the local server.
class BoreTunnel : public QObject {
    Q_OBJECT
   public:
    explicit BoreTunnel(QObject* parent = nullptr);
    ~BoreTunnel() override;

    void open(quint16 localPort, const QString& server = "bore.pub", quint16 controlPort = 7835);
    void close();
    QString publicHost() const { return m_server; }
    quint16 publicPort() const { return m_publicPort; }

   signals:
    void opened(QString host, quint16 port);
    void failed(QString reason);

   private:
    void readControl();
    void handleMessage(const QByteArray& json);
    void acceptConnection(const QString& id);
    static void send(QTcpSocket* socket, const QByteArray& json);

    QPointer<QTcpSocket> m_control;
    QByteArray m_buffer;
    QString m_server;
    quint16 m_controlPort = 7835;
    quint16 m_localPort = 0;
    quint16 m_publicPort = 0;
};
