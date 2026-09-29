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

#include "LocalAuth.h"

#include <QHostAddress>
#include <QTcpServer>

namespace LocalAuth {

QStringList jvmArguments(const QString& stubJar, const QString& injectorJar, quint16 port)
{
    const QString portString = QString::number(port);
    return {
        "-javaagent:" + stubJar + "=" + portString,
        "-javaagent:" + injectorJar + "=http://127.0.0.1:" + portString,
        "-Dauthlibinjector.noLogFile",
        "-Dauthlibinjector.noShowServerName",
    };
}

quint16 pickFreePort()
{
    QTcpServer probe;
    if (!probe.listen(QHostAddress::LocalHost, 0))
        return 0;
    const quint16 port = probe.serverPort();
    probe.close();
    return port;
}

}  // namespace LocalAuth
