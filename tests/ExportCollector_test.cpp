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

#include <minecraft/MojangVersionFormat.h>
#include <offline/ExportCollector.h>
#include <offline/ExportRules.h>

using namespace OfflineBundle;

static void put(const QString& path, const QByteArray& data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(data);
}

static QStringList sortedPaths(const FileSet& fs)
{
    QStringList paths;
    for (const auto& e : fs.entries())
        paths << e.bundlePath;
    paths.sort();
    return paths;
}

class ExportCollectorTest : public QObject {
    Q_OBJECT
   private slots:
    void fileSetRejectsCaseClashes()
    {
        QTemporaryDir dir;
        put(dir.filePath("a.txt"), "a");
        put(dir.filePath("b.txt"), "b");
        FileSet fs;
        QVERIFY(fs.addFile("libraries/A.txt", dir.filePath("a.txt")));
        QVERIFY(fs.addFile("libraries/A.txt", dir.filePath("a.txt")));  // same source again: fine, not duplicated
        QVERIFY(!fs.addFile("libraries/a.TXT", dir.filePath("b.txt")));  // different source, same path ignoring case
        QVERIFY(fs.addData("instance/instance.cfg", "name=x"));
        QCOMPARE(fs.entries().size(), 2);
        QCOMPARE(fs.conflicts().size(), 1);
        QCOMPARE(fs.totalSize(), qint64(1 + 6));
    }

    void addFileUnderRecordsMissingAndOutside()
    {
        QTemporaryDir dir;
        put(dir.filePath("libraries/org/x/1/x-1.jar"), "x");
        FileSet fs;
        addFileUnder(fs, dir.filePath("libraries/org/x/1/x-1.jar"), dir.filePath("libraries"), "libraries");
        addFileUnder(fs, dir.filePath("libraries/org/y/1/y-1.jar"), dir.filePath("libraries"), "libraries");
        addFileUnder(fs, dir.filePath("elsewhere/z.jar"), dir.filePath("libraries"), "libraries");
        QCOMPARE(sortedPaths(fs), QStringList{ "libraries/org/x/1/x-1.jar" });
        QCOMPARE(fs.missing().size(), 1);
        QCOMPARE(fs.outside().size(), 1);
    }

    void addTreeAppliesFilter()
    {
        QTemporaryDir dir;
        put(dir.filePath("inst/instance.cfg"), "c");
        put(dir.filePath("inst/minecraft/mods/a.jar"), "m");
        put(dir.filePath("inst/minecraft/logs/latest.log"), "l");
        FileSet fs;
        QVERIFY(addTree(fs, dir.filePath("inst"), "instance", [](const QString& rel) { return shouldExportInstanceFile(rel, false); }));
        QCOMPARE(sortedPaths(fs), (QStringList{ "instance/instance.cfg", "instance/minecraft/mods/a.jar" }));
        QVERIFY(!addTree(fs, dir.filePath("nope"), "x"));
    }

    void addMetaTakesVersionAndIndexes()
    {
        QTemporaryDir dir;
        put(dir.filePath("meta/index.json"), "{}");
        put(dir.filePath("meta/net.minecraft/index.json"), "{}");
        put(dir.filePath("meta/net.minecraft/26.3.json"), "{}");
        put(dir.filePath("meta/net.minecraft/26.2.json"), "{}");
        FileSet fs;
        addMeta(fs, dir.filePath("meta"), "net.minecraft", "26.3");
        QCOMPARE(sortedPaths(fs), (QStringList{ "meta/index.json", "meta/net.minecraft/26.3.json", "meta/net.minecraft/index.json" }));
        QVERIFY(fs.missing().isEmpty());
    }

    void addAssetsTakesIndexAndObjects()
    {
        QTemporaryDir dir;
        const QJsonObject index{ { "objects", QJsonObject{ { "minecraft/sounds/a.ogg", QJsonObject{ { "hash", "ab12" }, { "size", 1 } } },
                                                           { "icons/icon.png", QJsonObject{ { "hash", "cd34" }, { "size", 1 } } } } } };
        put(dir.filePath("assets/indexes/29.json"), QJsonDocument(index).toJson());
        put(dir.filePath("assets/objects/ab/ab12"), "a");
        put(dir.filePath("assets/objects/cd/cd34"), "c");
        FileSet fs;
        QVERIFY(addAssets(fs, dir.filePath("assets"), "29"));
        QCOMPARE(sortedPaths(fs), (QStringList{ "assets/indexes/29.json", "assets/objects/ab/ab12", "assets/objects/cd/cd34" }));
        QVERIFY(fs.missing().isEmpty());

        FileSet broken;
        QVERIFY(!addAssets(broken, dir.filePath("assets"), "30"));  // no such index
    }

    void addLibrariesUsesWindowsNatives()
    {
        QTemporaryDir dir;
        const QString oldCwd = QDir::currentPath();
        QDir::setCurrent(dir.path());
        put("libraries/org/lwjgl/lwjgl/3.3.3/lwjgl-3.3.3.jar", "j");
        put("libraries/org/lwjgl/lwjgl/3.3.3/lwjgl-3.3.3-natives-windows.jar", "w");
        ProblemContainer problems;
        auto plain = MojangVersionFormat::libraryFromJson(problems, QJsonObject{ { "name", "org.lwjgl:lwjgl:3.3.3" } }, "t");
        auto windows = MojangVersionFormat::libraryFromJson(
            problems,
            QJsonObject{ { "name", "org.lwjgl:lwjgl:3.3.3:natives-windows" },
                         { "rules", QJsonArray{ QJsonObject{ { "action", "allow" }, { "os", QJsonObject{ { "name", "windows" } } } } } } },
            "t");
        auto linux = MojangVersionFormat::libraryFromJson(
            problems,
            QJsonObject{ { "name", "org.lwjgl:lwjgl:3.3.3:natives-linux" },
                         { "rules", QJsonArray{ QJsonObject{ { "action", "allow" }, { "os", QJsonObject{ { "name", "linux" } } } } } } },
            "t");
        FileSet fs;
        addLibraries(fs, { plain, windows, linux }, windowsX64Context(), QString(), QDir(dir.path()).absoluteFilePath("libraries"));
        QDir::setCurrent(oldCwd);
        QCOMPARE(sortedPaths(fs), (QStringList{ "libraries/org/lwjgl/lwjgl/3.3.3/lwjgl-3.3.3-natives-windows.jar",
                                                "libraries/org/lwjgl/lwjgl/3.3.3/lwjgl-3.3.3.jar" }));
        QVERIFY(fs.missing().isEmpty());
    }

    void snapshotFindsNewAndChangedFiles()
    {
        QTemporaryDir dir;
        put(dir.filePath("libraries/a.jar"), "a");
        put(dir.filePath("libraries/b.jar"), "b");
        const auto before = snapshotTree(dir.filePath("libraries"));
        put(dir.filePath("libraries/b.jar"), "bb");   // size changed
        put(dir.filePath("libraries/c/c.jar"), "c");  // new
        QStringList changed = changedFiles(dir.filePath("libraries"), before);
        for (QString& path : changed)
            path = QDir(dir.filePath("libraries")).relativeFilePath(path);
        changed.sort();
        QCOMPARE(changed, (QStringList{ "b.jar", "c/c.jar" }));
    }
};

QTEST_GUILESS_MAIN(ExportCollectorTest)

#include "ExportCollector_test.moc"
