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
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include <meta/Index.h>
#include <meta/Version.h>
#include <meta/VersionList.h>
#include <offline/MetaIndexMerger.h>
#include <offline/MetaReload.h>

static bool writeJson(const QString& path, const QJsonObject& obj)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(QJsonDocument(obj).toJson()) > 0;
}

static QJsonObject versionFile(const QString& version)
{
    return { { "formatVersion", 1 }, { "uid", "net.minecraft" }, { "version", version }, { "name", "Minecraft" },
             { "releaseTime", "2026-09-15T00:00:00+00:00" }, { "type", "release" } };
}

static QJsonObject packageIndex(const QString& version)
{
    return { { "formatVersion", 1 }, { "name", "Minecraft" }, { "uid", "net.minecraft" },
             { "versions", QJsonArray{ QJsonObject{ { "version", version }, { "sha256", "placeholder" },
                                                    { "releaseTime", "2026-09-15T00:00:00+00:00" }, { "type", "release" } } } } };
}

static QJsonObject rootIndex()
{
    return { { "formatVersion", 1 }, { "packages", QJsonArray{ QJsonObject{ { "uid", "net.minecraft" }, { "name", "Minecraft" }, { "sha256", "x" } } } } };
}

class MetaReloadTest : public QObject {
    Q_OBJECT

   private slots:
    void test_newVersionAppearsAndLoadsAfterReload()
    {
        QTemporaryDir dir;
        const QString oldCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(dir.path()));

        // A data folder that already knows Minecraft 26.2 ...
        QVERIFY(writeJson("staged1/net.minecraft/26.2.json", versionFile("26.2")));
        QVERIFY(writeJson("staged1/net.minecraft/index.json", packageIndex("26.2")));
        QVERIFY(writeJson("staged1/index.json", rootIndex()));
        QVERIFY(OfflineBundle::mergeMeta("staged1", "meta").ok);

        Meta::Index index;
        index.loadTask(Net::Mode::Offline)->start();  // loads from disk synchronously
        auto list = index.get("net.minecraft");
        list->loadTask(Net::Mode::Offline)->start();
        QCOMPARE(list->count(), 1);

        // ... then a bundle adds 26.3.
        QVERIFY(writeJson("staged2/net.minecraft/26.3.json", versionFile("26.3")));
        QVERIFY(writeJson("staged2/net.minecraft/index.json", packageIndex("26.3")));
        const auto merged = OfflineBundle::mergeMeta("staged2", "meta");
        QVERIFY(merged.ok);

        OfflineBundle::reloadMetadata(&index, merged.changedUids);
        QCOMPARE(list->count(), 2);
        auto version = list->getVersion("26.3");
        QVERIFY(version);
        version->loadTask(Net::Mode::Offline)->start();
        QVERIFY(version->isLoaded());  // the recomputed sha256 matches the file

        QVERIFY(QDir::setCurrent(oldCwd));
    }
};

QTEST_GUILESS_MAIN(MetaReloadTest)

#include "MetaReload_test.moc"
