// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PineconeMC Offline - Minecraft Launcher
 *  Copyright (C) 2026 PineconeMC Offline Contributors
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
#include <QStringList>

// Offline accounts sign in through a small server inside the game process, so LAN works without internet.
// pinecone-offline-auth.jar is that server (a Java agent); authlib-injector.jar redirects Minecraft's auth calls to it.
namespace LocalAuth {
inline constexpr auto StubJarName = "pinecone-offline-auth.jar";
inline constexpr auto InjectorJarName = "authlib-injector.jar";

// JVM options for one launch. The stub agent comes first, so it's listening before authlib-injector starts.
QStringList jvmArguments(const QString& stubJar, const QString& injectorJar, quint16 port);

// A TCP port on 127.0.0.1 that's free right now, or 0 if none could be found.
quint16 pickFreePort();
}  // namespace LocalAuth
