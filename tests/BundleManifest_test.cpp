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
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include <archive/ArchiveWriter.h>
#include <offline/BundleManifest.h>
#include <offline/BundleMessages.h>
#include <offline/BundlePaths.h>

static const QString kSha1 = "0123456789abcdef0123456789abcdef01234567";

static QJsonObject fileEntry(const QString& path, qint64 size = 10)
{
    return { { "path", path }, { "sha1", kSha1 }, { "size", size } };
}

static QJsonObject versionsManifest()
{
    return QJsonObject{
        { "formatVersion", 1 },
        { "kind", "versions" },
        { "name", "Minecraft 26.3 + Fabric" },
        { "createdAt", "2026-09-29T12:00:00Z" },
        { "createdBy", "PineconeMC Offline 1.0.0" },
        { "contents", QJsonObject{ { "components", QJsonArray{ QJsonObject{ { "uid", "net.minecraft" }, { "version", "26.3" }, { "name", "Minecraft" } },
                                                              QJsonObject{ { "uid", "net.fabricmc.fabric-loader" }, { "version", "0.19.5" } } } },
                                   { "java", QJsonArray{ QJsonObject{ { "name", "Java 25" }, { "major", 25 }, { "folder", "java-runtime-epsilon" } } } },
                                   { "instance", QJsonValue::Null } } },
        { "files", QJsonArray{ fileEntry("meta/net.minecraft/index.json", 0), fileEntry("meta/net.minecraft/26.3.json", 100),
                               fileEntry("libraries/a/b.jar", 2000) } },
    };
}

static QByteArray toJson(const QJsonObject& obj)
{
    return QJsonDocument(obj).toJson();
}

class BundleManifestTest : public QObject {
    Q_OBJECT

   private slots:
    void test_parse_validVersionsBundle()
    {
        const auto result = OfflineBundle::parseManifest(toJson(versionsManifest()));
        QVERIFY2(result.manifest.has_value(), qPrintable(result.details));
        const auto& m = *result.manifest;
        QCOMPARE(m.kind, OfflineBundle::Kind::Versions);
        QCOMPARE(m.name, QString("Minecraft 26.3 + Fabric"));
        QCOMPARE(m.components.size(), 2);
        QCOMPARE(m.components.at(1).name, QString("net.fabricmc.fabric-loader"));  // name falls back to the uid
        QCOMPARE(m.java.at(0).major, 25);
        QCOMPARE(m.java.at(0).folder, QString("java-runtime-epsilon"));
        QVERIFY(!m.instance.has_value());
        QCOMPARE(m.files.size(), 3);
        QCOMPARE(m.totalSize(), qint64(2100));
    }

    void test_parse_validInstanceBundle()
    {
        QJsonObject obj = versionsManifest();
        obj["kind"] = "instance";
        QJsonObject contents = obj["contents"].toObject();
        contents["instance"] = QJsonObject{ { "name", "Cobblemon Pack" }, { "folder", "instance" }, { "group", "Modpacks" } };
        obj["contents"] = contents;
        QJsonArray files = obj["files"].toArray();
        files.append(fileEntry("instance/instance.cfg"));
        obj["files"] = files;
        const auto result = OfflineBundle::parseManifest(toJson(obj));
        QVERIFY2(result.manifest.has_value(), qPrintable(result.details));
        QCOMPARE(result.manifest->instance->name, QString("Cobblemon Pack"));
        QCOMPARE(result.manifest->instance->group, QString("Modpacks"));
    }

    void test_parse_newerFormatVersion()
    {
        QJsonObject obj = versionsManifest();
        obj["formatVersion"] = 2;
        const auto result = OfflineBundle::parseManifest(toJson(obj));
        QVERIFY(!result.manifest.has_value());
        QCOMPARE(result.message, OfflineBundle::Messages::newerFormat());
    }

    void test_parse_damaged_data()
    {
        QTest::addColumn<QByteArray>("json");
        QJsonObject noFormat = versionsManifest();
        noFormat.remove("formatVersion");
        QJsonObject badKind = versionsManifest();
        badKind["kind"] = "world";
        QJsonObject badSha = versionsManifest();
        badSha["files"] = QJsonArray{ QJsonObject{ { "path", "libraries/a.jar" }, { "sha1", "xyz" }, { "size", 1 } } };
        QJsonObject duplicate = versionsManifest();
        duplicate["files"] = QJsonArray{ fileEntry("libraries/A.jar"), fileEntry("libraries/a.jar") };
        QJsonObject instanceWithoutCfg = versionsManifest();
        instanceWithoutCfg["kind"] = "instance";
        QJsonObject contents = instanceWithoutCfg["contents"].toObject();
        contents["instance"] = QJsonObject{ { "name", "X" }, { "folder", "instance" } };
        instanceWithoutCfg["contents"] = contents;
        QJsonObject versionsWithInstanceFiles = versionsManifest();
        versionsWithInstanceFiles["files"] = QJsonArray{ fileEntry("instance/instance.cfg") };
        QJsonObject noFiles = versionsManifest();
        noFiles.remove("files");
        QJsonObject metaWithoutIndex = versionsManifest();
        metaWithoutIndex["files"] = QJsonArray{ fileEntry("meta/net.minecraft/26.3.json"), fileEntry("meta/.junk/notes.json") };

        QTest::newRow("not json") << QByteArray("{ nope");
        QTest::newRow("no formatVersion") << toJson(noFormat);
        QTest::newRow("unknown kind") << toJson(badKind);
        QTest::newRow("bad sha1") << toJson(badSha);
        QTest::newRow("duplicate path, different case") << toJson(duplicate);
        QTest::newRow("instance bundle without instance.cfg") << toJson(instanceWithoutCfg);
        QTest::newRow("versions bundle with instance files") << toJson(versionsWithInstanceFiles);
        QTest::newRow("no files list") << toJson(noFiles);
        QTest::newRow("meta package without index.json") << toJson(metaWithoutIndex);
    }

    void test_parse_damaged()
    {
        QFETCH(QByteArray, json);
        const auto result = OfflineBundle::parseManifest(json);
        QVERIFY(!result.manifest.has_value());
        QCOMPARE(result.message, OfflineBundle::Messages::damaged());
        QVERIFY(!result.details.isEmpty());
    }

    void test_parse_unsafePath()
    {
        QJsonObject obj = versionsManifest();
        obj["files"] = QJsonArray{ fileEntry("meta/../../evil.dll") };
        const auto result = OfflineBundle::parseManifest(toJson(obj));
        QVERIFY(!result.manifest.has_value());
        QCOMPARE(result.message, OfflineBundle::Messages::unsafe());
    }

    void test_readManifestFromZip()
    {
        QTemporaryDir dir;
        const QString bundle = dir.filePath("bundle.zip");
        {
            MMCZip::ArchiveWriter zip(bundle);
            QVERIFY(zip.open());
            QVERIFY(zip.addFile(QString(OfflineBundle::ManifestFileName), toJson(versionsManifest())));
            QVERIFY(zip.close());
        }
        const QString modpack = dir.filePath("modpack.zip");
        {
            MMCZip::ArchiveWriter zip(modpack);
            QVERIFY(zip.open());
            QVERIFY(zip.addFile(QString("manifest.json"), QByteArray("{}")));
            QVERIFY(zip.close());
        }
        QVERIFY(OfflineBundle::readManifestFromZip(bundle).manifest.has_value());
        QVERIFY(OfflineBundle::isBundle(bundle));

        const auto notBundle = OfflineBundle::readManifestFromZip(modpack);
        QVERIFY(!notBundle.manifest.has_value());
        QCOMPARE(notBundle.message, OfflineBundle::Messages::notABundle());
        QVERIFY(!OfflineBundle::isBundle(modpack));
        QVERIFY(!OfflineBundle::isBundle(dir.filePath("missing.zip")));
    }

    void serializeRoundTrip()
    {
        OfflineBundle::Manifest m;
        m.formatVersion = 1;
        m.kind = OfflineBundle::Kind::Instance;
        m.name = "Cobblemon Pack";
        m.createdAt = "2026-09-29T12:00:00Z";
        m.createdBy = "PineconeMC Offline 1.0.0";
        m.components = { { "net.minecraft", "26.3", "Minecraft" }, { "net.neoforged", "26.3.0.33-beta", "NeoForge" } };
        m.java = { { "Java 25", 25, "java-runtime-epsilon" } };
        m.instance = OfflineBundle::InstanceEntry{ "Cobblemon Pack", "instance", "Modpacks" };
        m.files = { { "instance/instance.cfg", QString(40, 'a'), 12 }, { "meta/net.minecraft/index.json", QString(40, 'b'), 3 } };

        const auto read = OfflineBundle::parseManifest(OfflineBundle::serializeManifest(m));
        QVERIFY2(read.manifest, qPrintable(read.details));
        const auto& r = *read.manifest;
        QCOMPARE(r.kind, OfflineBundle::Kind::Instance);
        QCOMPARE(r.name, m.name);
        QCOMPARE(r.createdAt, m.createdAt);
        QCOMPARE(r.createdBy, m.createdBy);
        QCOMPARE(r.components.size(), 2);
        QCOMPARE(r.components.at(1).uid, QString("net.neoforged"));
        QCOMPARE(r.components.at(1).version, QString("26.3.0.33-beta"));
        QCOMPARE(r.components.at(1).name, QString("NeoForge"));
        QCOMPARE(r.java.size(), 1);
        QCOMPARE(r.java.at(0).major, 25);
        QCOMPARE(r.java.at(0).folder, QString("java-runtime-epsilon"));
        QVERIFY(r.instance);
        QCOMPARE(r.instance->group, QString("Modpacks"));
        QCOMPARE(r.files.size(), 2);
        QCOMPARE(r.files.at(0).size, qint64(12));
        QCOMPARE(r.totalSize(), qint64(15));
    }

    void serializeVersionsBundleHasNullInstance()
    {
        OfflineBundle::Manifest m;
        m.formatVersion = 1;
        m.kind = OfflineBundle::Kind::Versions;
        m.name = "Minecraft 26.3";
        m.components = { { "net.minecraft", "26.3", "Minecraft" } };
        const QJsonObject root = QJsonDocument::fromJson(OfflineBundle::serializeManifest(m)).object();
        QCOMPARE(root.value("kind").toString(), QString("versions"));
        QVERIFY(root.value("contents").toObject().value("instance").isNull());
        QVERIFY(root.value("files").isArray());
        QVERIFY(OfflineBundle::parseManifest(OfflineBundle::serializeManifest(m)).manifest);
    }
};

QTEST_GUILESS_MAIN(BundleManifestTest)

#include "BundleManifest_test.moc"
