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

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <offline/BundleSummary.h>

class BundleSummaryTest : public QObject {
    Q_OBJECT

   private slots:
    void test_marksWhatIsAlreadyInstalled()
    {
        QTemporaryDir dir;
        const QString data = dir.filePath("data"), javaDir = dir.filePath("data/java");
        QVERIFY(QDir().mkpath(data + "/meta/net.minecraft"));
        QFile existing(data + "/meta/net.minecraft/26.3.json");
        QVERIFY(existing.open(QIODevice::WriteOnly));
        existing.close();
        QVERIFY(QDir().mkpath(javaDir + "/java-runtime-epsilon"));

        OfflineBundle::Manifest m;
        m.components = { { "net.minecraft", "26.3", "Minecraft" }, { "net.fabricmc.fabric-loader", "0.19.5", "Fabric Loader" } };
        m.java = { { "Java 25", 25, "java-runtime-epsilon" }, { "Java 21", 21, "java-runtime-delta" } };
        m.instance = OfflineBundle::InstanceEntry{ "Cobblemon Pack", "instance", {} };

        const auto lines = OfflineBundle::summarize(m, data, javaDir);
        QCOMPARE(lines.size(), 5);
        QCOMPARE(lines.at(0).text, QString("Minecraft 26.3"));
        QVERIFY(lines.at(0).alreadyInstalled);
        QCOMPARE(lines.at(1).text, QString("Fabric Loader 0.19.5"));
        QVERIFY(!lines.at(1).alreadyInstalled);
        QCOMPARE(lines.at(2).text, QString("Java 25"));
        QVERIFY(lines.at(2).alreadyInstalled);
        QVERIFY(!lines.at(3).alreadyInstalled);
        QCOMPARE(lines.at(4).text, QString("Instance: Cobblemon Pack"));
    }
};

QTEST_GUILESS_MAIN(BundleSummaryTest)

#include "BundleSummary_test.moc"
