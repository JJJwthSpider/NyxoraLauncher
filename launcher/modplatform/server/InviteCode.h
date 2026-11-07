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

#include <QString>

/// Invite codes for Lullaby servers, like "LL-8N4Q-2ZKD-1X" (codes of the older SculkShoot Client start with "SC-"). The code carries the address and the port itself,
/// so no service is needed to look it up: the friend types it into the launcher and gets the IP and the port back.
///
/// Inside: a type byte (IPv4, the bore.pub tunnel or any host name), the address, the port and a check byte so that
/// a mistyped code is noticed. Written in Crockford's base32, which has no I, L, O and U: they can't be confused
/// with 1 and 0, and typing them anyway still works.
namespace InviteCode {

QString encode(const QString& host, quint16 port);
bool decode(const QString& code, QString& host, quint16& port);

}  // namespace InviteCode
