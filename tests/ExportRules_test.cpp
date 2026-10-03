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

#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

#include <minecraft/Library.h>
#include <minecraft/MojangVersionFormat.h>
#include <offline/BundleMessages.h>
#include <offline/ExportRules.h>

using namespace OfflineBundle;

class ExportRulesTest : public QObject {
    Q_OBJECT
   private slots:
    void parsesExportSets()
    {
        auto plain = parseExportSet("26.3");
        QVERIFY(plain);
        QCOMPARE(plain->minecraft, QString("26.3"));
        QVERIFY(plain->loaderUid.isEmpty());

        auto withLoader = parseExportSet(" 26.3 , net.neoforged = 26.3.0.33-beta ");
        QVERIFY(withLoader);
        QCOMPARE(withLoader->minecraft, QString("26.3"));
        QCOMPARE(withLoader->loaderUid, QString("net.neoforged"));
        QCOMPARE(withLoader->loaderVersion, QString("26.3.0.33-beta"));

        QVERIFY(!parseExportSet(""));
        QVERIFY(!parseExportSet("26.3,net.neoforged"));
        QVERIFY(!parseExportSet("26.3,=1"));
    }

    void filtersInstanceFiles_data()
    {
        QTest::addColumn<QString>("path");
        QTest::addColumn<bool>("worlds");
        QTest::addColumn<bool>("expected");
        QTest::newRow("cfg") << "instance.cfg" << false << true;
        QTest::newRow("pack") << "mmc-pack.json" << false << true;
        QTest::newRow("mod") << "minecraft/mods/sodium.jar" << false << true;
        QTest::newRow("mod index") << "minecraft/mods/.index/sodium.pw.toml" << false << true;
        QTest::newRow("dot-minecraft root") << ".minecraft/config/a.toml" << false << true;
        QTest::newRow("options") << "minecraft/options.txt" << false << true;
        QTest::newRow("logs") << "minecraft/logs/latest.log" << false << false;
        QTest::newRow("crash") << "minecraft/crash-reports/c.txt" << false << false;
        QTest::newRow("saves off") << "minecraft/saves/World/level.dat" << false << false;
        QTest::newRow("saves on") << "minecraft/saves/World/level.dat" << true << true;
        QTest::newRow("cache") << "minecraft/.cache/x" << false << false;
        QTest::newRow("fabric cache") << "minecraft/.fabric/remappedJars/a.jar" << false << false;
        QTest::newRow("hidden tool folder") << ".toolcache/state.json" << false << false;
        QTest::newRow("natives") << "natives/lwjgl.dll" << false << false;
        QTest::newRow("dot-minecraft logs") << ".minecraft/logs/latest.log" << false << false;
    }
    void filtersInstanceFiles()
    {
        QFETCH(QString, path);
        QFETCH(bool, worlds);
        QFETCH(bool, expected);
        QCOMPARE(shouldExportInstanceFile(path, worlds), expected);
    }

    void sanitizesAutomaticJava()
    {
        const QByteArray cfg = "[General]\r\nAutomaticJava=true\r\nJavaPath=C:/Users/x/java.exe\r\nOverrideJavaLocation=true\r\nname=Pack\r\n";
        QCOMPARE(sanitizeInstanceCfg(cfg), QByteArray("[General]\r\nname=Pack\r\n"));
    }

    void keepsManualJava()
    {
        const QByteArray cfg = "[General]\nJavaPath=java/custom/bin/javaw.exe\nOverrideJavaLocation=true\nname=Pack\n";
        QCOMPARE(sanitizeInstanceCfg(cfg), cfg);
    }

    void readsJavaMajor()
    {
        QCOMPARE(javaMajorFromRelease("IMPLEMENTOR=\"Microsoft\"\nJAVA_VERSION=\"25.0.1\"\n"), 25);
        QCOMPARE(javaMajorFromRelease("JAVA_VERSION=\"1.8.0_392\"\r\n"), 8);
        QCOMPARE(javaMajorFromRelease("nothing here"), 0);
    }

    void namesBundleFiles()
    {
        QCOMPARE(suggestedFileName({ { "net.minecraft", "26.3", "Minecraft" }, { "net.neoforged", "26.3.0.33-beta", "NeoForge" } }),
                 QString("MC-26.3-NeoForge-26.3.0.33-beta.zip"));
        QCOMPARE(suggestedFileName({ { "net.minecraft", "26.3", "Minecraft" },
                                     { "net.fabricmc.fabric-loader", "0.19.5", "Fabric Loader" },
                                     { "net.minecraft", "26.3", "Minecraft" },
                                     { "org.lwjgl3", "3.3.3", "LWJGL 3" } }),
                 QString("MC-26.3-Fabric-0.19.5.zip"));
        QCOMPARE(suggestedInstanceFileName("Cobblemon: Pack?"), QString("Cobblemon_ Pack_-offline.zip"));
    }

    void namesLoaders()
    {
        QCOMPARE(loaderDisplayName("net.minecraftforge"), QString("Forge"));
        QCOMPARE(loaderDisplayName("org.quiltmc.quilt-loader"), QString("Quilt Loader"));
        QVERIFY(loaderDisplayName("org.lwjgl3").isEmpty());
    }

    void forcesWindowsNatives()
    {
        const RuntimeContext ctx = windowsX64Context();
        QCOMPARE(ctx.system, QString("windows"));
        QCOMPARE(ctx.getClassifier(), QString("windows-x86_64"));

        ProblemContainer problems;
        auto windowsOnly = MojangVersionFormat::libraryFromJson(
            problems,
            QJsonObject{ { "name", "org.lwjgl:lwjgl:3.3.3:natives-windows" },
                         { "rules", QJsonArray{ QJsonObject{ { "action", "allow" }, { "os", QJsonObject{ { "name", "windows" } } } } } } },
            "test");
        auto linuxOnly = MojangVersionFormat::libraryFromJson(
            problems,
            QJsonObject{ { "name", "org.lwjgl:lwjgl:3.3.3:natives-linux" },
                         { "rules", QJsonArray{ QJsonObject{ { "action", "allow" }, { "os", QJsonObject{ { "name", "linux" } } } } } } },
            "test");
        QVERIFY(windowsOnly->isActive(ctx));
        QVERIFY(!linuxOnly->isActive(ctx));
    }

    void messages()
    {
        QVERIFY(Messages::downloadFailed("Minecraft 26.3").contains("Minecraft 26.3"));
        QVERIFY(Messages::downloadFailed("x").contains("internet"));
        QVERIFY(Messages::exportNeedsInternet().contains("internet"));
        QVERIFY(!Messages::couldNotWriteBundle().isEmpty());
    }
};

QTEST_GUILESS_MAIN(ExportRulesTest)

#include "ExportRules_test.moc"
