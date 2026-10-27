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
#include "InviteCode.h"

#include <QByteArray>
#include <QHostAddress>

namespace InviteCode {

namespace {
const char* const s_alphabet = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
const char* const s_boreHost = "bore.pub";

enum Type : quint8 { IPv4 = 1, Bore = 2, HostName = 3 };

quint8 check(const QByteArray& bytes)
{
    quint8 sum = 0x5A;
    for (const char c : bytes) {
        sum = quint8((sum << 1 | sum >> 7) ^ quint8(c));
    }
    return sum;
}

QString toBase32(const QByteArray& bytes)
{
    QString out;
    quint32 buffer = 0;
    int bits = 0;
    for (const char c : bytes) {
        buffer = (buffer << 8) | quint8(c);
        bits += 8;
        while (bits >= 5) {
            out += QLatin1Char(s_alphabet[(buffer >> (bits - 5)) & 31]);
            bits -= 5;
        }
    }
    if (bits > 0) {
        out += QLatin1Char(s_alphabet[(buffer << (5 - bits)) & 31]);
    }
    return out;
}

bool fromBase32(const QString& text, QByteArray& out)
{
    quint32 buffer = 0;
    int bits = 0;
    for (QChar ch : text) {
        ch = ch.toUpper();
        if (ch == 'O') {
            ch = '0';
        } else if (ch == 'I' || ch == 'L') {
            ch = '1';
        }
        const char* found = strchr(s_alphabet, ch.toLatin1());
        if (!found || ch.isNull()) {
            return false;
        }
        buffer = (buffer << 5) | quint32(found - s_alphabet);
        bits += 5;
        if (bits >= 8) {
            out += char((buffer >> (bits - 8)) & 0xFF);
            bits -= 8;
        }
    }
    return true;
}
}  // namespace

QString encode(const QString& host, quint16 port)
{
    QByteArray bytes;
    QHostAddress address;
    if (host == s_boreHost) {
        bytes += char(Bore);
    } else if (address.setAddress(host) && address.protocol() == QAbstractSocket::IPv4Protocol) {
        bytes += char(IPv4);
        const quint32 ip = address.toIPv4Address();
        for (int shift = 24; shift >= 0; shift -= 8) {
            bytes += char((ip >> shift) & 0xFF);
        }
    } else {
        const QByteArray name = host.toUtf8().left(200);
        bytes += char(HostName);
        bytes += char(name.size());
        bytes += name;
    }
    bytes += char(port >> 8);
    bytes += char(port & 0xFF);
    bytes += char(check(bytes));

    const QString body = toBase32(bytes);
    QStringList groups;
    for (int i = 0; i < body.size(); i += 4) {
        groups << body.mid(i, 4);
    }
    return "SC-" + groups.join('-');
}

bool decode(const QString& code, QString& host, quint16& port)
{
    QString text = code.trimmed().toUpper();
    if (text.startsWith("SC-") || text.startsWith("SC ")) {
        text = text.mid(3);
    }
    text.remove('-').remove(' ');
    QByteArray bytes;
    if (text.size() < 4 || !fromBase32(text, bytes) || bytes.size() < 4) {
        return false;
    }
    // base32 may add a few padding bits that make an extra zero byte at the end
    auto valid = [](const QByteArray& b) { return b.size() >= 4 && check(b.left(b.size() - 1)) == quint8(b.back()); };
    while (!valid(bytes) && !bytes.isEmpty() && bytes.back() == '\0') {
        bytes.chop(1);
    }
    if (!valid(bytes)) {
        return false;
    }
    bytes.chop(1);

    const auto type = quint8(bytes[0]);
    int at = 1;
    switch (type) {
        case Bore:
            host = s_boreHost;
            break;
        case IPv4:
            if (bytes.size() < 7) {
                return false;
            }
            host = QString("%1.%2.%3.%4").arg(quint8(bytes[1])).arg(quint8(bytes[2])).arg(quint8(bytes[3])).arg(quint8(bytes[4]));
            at = 5;
            break;
        case HostName: {
            const int length = quint8(bytes[1]);
            if (bytes.size() < 2 + length + 2) {
                return false;
            }
            host = QString::fromUtf8(bytes.mid(2, length));
            at = 2 + length;
            break;
        }
        default:
            return false;
    }
    if (bytes.size() != at + 2) {
        return false;
    }
    port = quint16((quint8(bytes[at]) << 8) | quint8(bytes[at + 1]));
    return port != 0;
}

}  // namespace InviteCode
