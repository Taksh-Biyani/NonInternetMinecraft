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
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

#include <offline/LaunchCompleteness.h>

using namespace OfflineBundle;

class LaunchCompletenessTest : public QObject {
    Q_OBJECT

    static void touch(const QString& path)
    {
        QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
    }

   private slots:
    void readsPackComponents()
    {
        QTemporaryDir dir;
        const QString pack = dir.filePath("mmc-pack.json");
        QFile file(pack);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(R"({"components":[
            {"uid":"org.lwjgl3","version":"3.4.3","cachedName":"LWJGL 3","dependencyOnly":true},
            {"uid":"net.minecraft","version":"26.3","cachedName":"Minecraft","important":true},
            {"uid":"net.fabricmc.fabric-loader","version":"0.19.5"},
            {"uid":"net.fabricmc.intermediary","version":"26.3","disabled":true},
            {"uid":"custom.thing"}
        ],"formatVersion":1})");
        file.close();

        const auto components = readPackComponents(pack);
        QCOMPARE(components.size(), 3);
        QCOMPARE(components[0].name, QString("LWJGL 3"));
        QCOMPARE(components[1].uid, QString("net.minecraft"));
        QCOMPARE(components[1].version, QString("26.3"));
        QCOMPARE(components[2].name, QString("net.fabricmc.fabric-loader"));  // no cachedName: the uid
        QVERIFY(readPackComponents(dir.filePath("missing.json")).isEmpty());
    }

    void findsMissingComponentMeta()
    {
        QTemporaryDir dir;
        touch(dir.filePath("meta/net.minecraft/26.3.json"));
        touch(dir.filePath("instance/patches/my.patch.json"));
        const QList<PackComponent> components = { { "net.minecraft", "26.3", "Minecraft" },
                                                  { "net.fabricmc.fabric-loader", "0.19.5", "Fabric Loader" },
                                                  { "my.patch", "1", "My patch" } };
        QCOMPARE(missingComponentMeta(dir.filePath("meta"), dir.filePath("instance/patches"), components),
                 QStringList{ "Fabric Loader 0.19.5" });
    }

    void checksLaunchFiles()
    {
        QTemporaryDir dir;
        const QString present = dir.filePath("libraries/a/present.jar");
        touch(present);
        const QString absent = dir.filePath("libraries/b/absent.jar");
        const QString game = dir.filePath("libraries/com/mojang/minecraft/26.3/minecraft-26.3-client.jar");
        const QString assets = dir.filePath("assets");
        QVERIFY(QDir().mkpath(assets + "/indexes"));
        QFile index(assets + "/indexes/34.json");
        QVERIFY(index.open(QIODevice::WriteOnly));
        index.write(R"({"objects":{"a.ogg":{"hash":"aa11","size":1},"b.png":{"hash":"bb22","size":1},"c.png":{"hash":"bb22","size":1}}})");
        index.close();
        touch(assets + "/objects/aa/aa11");

        MissingReport report;
        checkLaunchFiles(report, { present, absent, absent, game }, game, assets, "34");
        QCOMPARE(report.gameJar, game);
        QCOMPARE(report.libraryFiles, QStringList{ absent });  // once, and the game jar isn't counted as a library
        QVERIFY(report.assetIndex.isEmpty());
        QCOMPARE(report.assetObjects, QStringList{ QDir(assets).absoluteFilePath("objects/bb/bb22") });

        MissingReport noIndex;
        checkLaunchFiles(noIndex, {}, QString(), assets, "35");
        QCOMPARE(noIndex.assetIndex, QDir(assets).absoluteFilePath("indexes/35.json"));
        QVERIFY(!noIndex.isEmpty());

        MissingReport complete;
        checkLaunchFiles(complete, { present }, QString(), assets, QString());
        QVERIFY(complete.isEmpty());
    }

    void describesMissingThingsPlainly()
    {
        MissingReport report;
        report.minecraftVersion = "26.3";
        report.components = { "Fabric Loader 0.19.5" };
        report.gameJar = "C:/x/client.jar";
        report.libraryFiles = { "a", "b", "c" };
        report.assetObjects = { "o" };
        report.javaMajors = { 25 };
        QCOMPARE(describeMissing(report), (QStringList{ "Fabric Loader 0.19.5", "The Minecraft 26.3 game file", "3 game library files",
                                                        "Minecraft 26.3 sounds and textures", "Java 25" }));

        MissingReport other;
        other.libraryFiles = { "a" };
        other.assetIndex = "i";
        other.javaMajors = { 17, 21 };
        QCOMPARE(describeMissing(other), (QStringList{ "1 game library file", "Minecraft sounds and textures", "Java 17 or 21" }));

        const QString html = missingMessage(report, "My <Pack>");
        QVERIFY(html.contains("My &lt;Pack&gt;"));
        QVERIFY(html.contains("<li>Java 25</li>"));
        QVERIFY(html.contains("Export Bundle"));
        const QString details = missingDetails(report);
        QVERIFY(details.contains("C:/x/client.jar"));
        QVERIFY(details.contains("Fabric Loader 0.19.5"));
    }

    void storesOneReportPerInstance()
    {
        MissingReport report;
        report.javaMajors = { 25 };
        storeLaunchReport("inst", report);
        const auto taken = takeLaunchReport("inst");
        QVERIFY(taken);
        QCOMPARE(taken->javaMajors, QList<int>{ 25 });
        QVERIFY(!takeLaunchReport("inst"));
    }
};

QTEST_GUILESS_MAIN(LaunchCompletenessTest)

#include "LaunchCompleteness_test.moc"
