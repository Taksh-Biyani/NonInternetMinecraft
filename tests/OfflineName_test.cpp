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

#include <QTest>

#include <offline/OfflineName.h>

class OfflineNameTest : public QObject {
    Q_OBJECT
   private slots:
    void acceptsMinecraftNames()
    {
        QVERIFY(OfflineName::problem("Steve").isEmpty());
        QVERIFY(OfflineName::problem("a_1").isEmpty());
        QVERIFY(OfflineName::problem("ABCDEFGHIJKLMNOP").isEmpty());  // 16
    }
    void explainsProblems()
    {
        QVERIFY(OfflineName::problem("").contains("3 to 16"));
        QVERIFY(OfflineName::problem("ab").contains("too short"));
        QVERIFY(OfflineName::problem("ABCDEFGHIJKLMNOPQ").contains("too long"));  // 17
        QVERIFY(OfflineName::problem("Bob Smith").contains("letters"));
        QVERIFY(OfflineName::problem("Tést").contains("letters"));
    }
};

QTEST_GUILESS_MAIN(OfflineNameTest)

#include "OfflineName_test.moc"
