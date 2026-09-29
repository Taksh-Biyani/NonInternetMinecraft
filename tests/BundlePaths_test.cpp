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
};

QTEST_GUILESS_MAIN(BundlePathsTest)

#include "BundlePaths_test.moc"
