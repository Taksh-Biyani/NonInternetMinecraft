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

#include <offline/BundleMessages.h>
#include <offline/BundlePaths.h>

class BundlePathsTest : public QObject {
    Q_OBJECT

   private slots:
    void test_formatSize_data()
    {
        QTest::addColumn<qint64>("bytes");
        QTest::addColumn<QString>("expected");
        QTest::newRow("tiny rounds up to 1 KB") << qint64(100) << "1 KB";
        QTest::newRow("kilobytes") << qint64(5 * 1024) << "5 KB";
        QTest::newRow("megabytes") << qint64(612) * 1024 * 1024 << "612 MB";
        QTest::newRow("gigabytes") << qint64(3) * 512 * 1024 * 1024 << "1.5 GB";
    }

    void test_formatSize()
    {
        QFETCH(qint64, bytes);
        QFETCH(QString, expected);
        QCOMPARE(OfflineBundle::Messages::formatSize(bytes), expected);
    }

    void test_notEnoughSpace_mentionsBothSizes()
    {
        const QString text = OfflineBundle::Messages::notEnoughSpace(qint64(2) * 1024 * 1024 * 1024, qint64(512) * 1024 * 1024);
        QVERIFY(text.contains("2.0 GB"));
        QVERIFY(text.contains("512 MB"));
    }

    void test_safePaths_data()
    {
        QTest::addColumn<QString>("path");
        QTest::newRow("manifest") << "pinecone-offline-bundle.json";
        QTest::newRow("root folder") << "meta";
        QTest::newRow("meta file") << "meta/net.minecraft/26.3.json";
        QTest::newRow("library") << "libraries/org/lwjgl/lwjgl/3.4.3/lwjgl-3.4.3.jar";
        QTest::newRow("asset object") << "assets/objects/ab/abcdef0123";
        QTest::newRow("java") << "java/java-runtime-epsilon/bin/javaw.exe";
        QTest::newRow("instance world with space") << "instance/minecraft/saves/My World/level.dat";
    }

    void test_safePaths()
    {
        QFETCH(QString, path);
        QCOMPARE(OfflineBundle::checkEntryPath(path), QString());
    }

    void test_unsafePaths_data()
    {
        QTest::addColumn<QString>("path");
        QTest::newRow("empty") << "";
        QTest::newRow("absolute") << "/etc/passwd";
        QTest::newRow("drive letter") << "C:/Windows/evil.dll";
        QTest::newRow("parent segment") << "meta/../../evil.txt";
        QTest::newRow("dot segment") << "./meta/x.json";
        QTest::newRow("empty segment") << "meta//x.json";
        QTest::newRow("top level not allowed") << "saves/world/level.dat";
        QTest::newRow("loose top-level file") << "readme.txt";
        QTest::newRow("alternate data stream") << "meta/a.json:hidden";
        QTest::newRow("reserved name") << "meta/CON";
        QTest::newRow("reserved name with extension") << "libraries/nul.txt";
        QTest::newRow("trailing dot") << "meta/name.";
        QTest::newRow("trailing space") << "meta/name ";
        QTest::newRow("control character") << "meta/a\tb";
    }

    void test_unsafePaths()
    {
        QFETCH(QString, path);
        QVERIFY(!OfflineBundle::checkEntryPath(path).isEmpty());
    }

    void test_normalizeEntryPath()
    {
        QCOMPARE(OfflineBundle::normalizeEntryPath("meta\\net.minecraft\\26.3.json"), QString("meta/net.minecraft/26.3.json"));
        QCOMPARE(OfflineBundle::normalizeEntryPath("meta/"), QString("meta"));
        QCOMPARE(OfflineBundle::normalizeEntryPath("assets/objects/ab/abc"), QString("assets/objects/ab/abc"));
    }
};

QTEST_GUILESS_MAIN(BundlePathsTest)

#include "BundlePaths_test.moc"
