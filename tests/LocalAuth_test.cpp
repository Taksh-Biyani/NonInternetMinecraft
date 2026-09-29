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

#include <QHostAddress>
#include <QTcpServer>
#include <QTest>

#include <offline/LocalAuth.h>

class LocalAuthTest : public QObject {
    Q_OBJECT

   private slots:
    void test_jvmArguments_stubFirstThenInjector()
    {
        const QStringList expected = {
            "-javaagent:C:/app/jars/pinecone-offline-auth.jar=25601",
            "-javaagent:C:/app/jars/authlib-injector.jar=http://127.0.0.1:25601",
            "-Dauthlibinjector.noLogFile",
            "-Dauthlibinjector.noShowServerName",
        };
        QCOMPARE(LocalAuth::jvmArguments("C:/app/jars/pinecone-offline-auth.jar", "C:/app/jars/authlib-injector.jar", 25601), expected);
    }

    void test_pickFreePort_returnsABindablePort()
    {
        const quint16 port = LocalAuth::pickFreePort();
        QVERIFY(port != 0);
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, port));
    }
};

QTEST_GUILESS_MAIN(LocalAuthTest)

#include "LocalAuth_test.moc"
