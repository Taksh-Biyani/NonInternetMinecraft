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

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include <offline/MetaIndexMerger.h>

static bool writeJson(const QString& path, const QJsonObject& obj)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(QJsonDocument(obj).toJson()) > 0;
}

static QByteArray readBytes(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

static QJsonObject readJson(const QString& path)
{
    return QJsonDocument::fromJson(readBytes(path)).object();
}

static QString sha256Of(const QString& path)
{
    return QString::fromLatin1(QCryptographicHash::hash(readBytes(path), QCryptographicHash::Sha256).toHex());
}

static QJsonObject versionEntry(const QString& version, const QString& sha)
{
    return { { "version", version }, { "sha256", sha }, { "type", "release" }, { "releaseTime", "2026-09-15T00:00:00+00:00" } };
}

static QJsonObject packageIndex(const QJsonArray& versions)
{
    return { { "formatVersion", 1 }, { "name", "Minecraft" }, { "uid", "net.minecraft" }, { "versions", versions } };
}

static QJsonObject rootIndex(const QJsonArray& packages)
{
    return { { "formatVersion", 1 }, { "packages", packages } };
}

// A staged meta folder with Minecraft 26.3 (and a 26.2 index entry whose file is NOT in the bundle).
static bool stageMinecraft263(const QString& staged)
{
    return writeJson(staged + "/net.minecraft/26.3.json", { { "uid", "net.minecraft" }, { "version", "26.3" } }) &&
           writeJson(staged + "/net.minecraft/index.json", packageIndex({ versionEntry("26.3", "stale"), versionEntry("26.2", "x") })) &&
           writeJson(staged + "/index.json", rootIndex({ QJsonObject{ { "uid", "net.minecraft" }, { "name", "Minecraft" }, { "sha256", "y" } } }));
}

static QStringList versionsIn(const QJsonObject& index)
{
    QStringList result;
    for (const QJsonValue& v : index.value("versions").toArray())
        result << v.toObject().value("version").toString();
    return result;
}

class MetaIndexMergerTest : public QObject {
    Q_OBJECT

   private slots:
    void test_freshDataFolder()
    {
        QTemporaryDir dir;
        const QString staged = dir.filePath("staged"), live = dir.filePath("live");
        QVERIFY(stageMinecraft263(staged));

        const auto result = OfflineBundle::mergeMeta(staged, live);
        QVERIFY2(result.ok, qPrintable(result.details));
        QCOMPARE(result.changedUids, QStringList{ "net.minecraft" });
        QCOMPARE(readBytes(live + "/net.minecraft/26.3.json"), readBytes(staged + "/net.minecraft/26.3.json"));

        const QJsonObject index = readJson(live + "/net.minecraft/index.json");
        QCOMPARE(versionsIn(index), QStringList{ "26.3" });  // 26.2's file isn't in the bundle, so it isn't added
        QCOMPARE(index["versions"].toArray().at(0).toObject()["sha256"].toString(), sha256Of(live + "/net.minecraft/26.3.json"));

        const QJsonArray packages = readJson(live + "/index.json")["packages"].toArray();
        QCOMPARE(packages.size(), 1);
        QCOMPARE(packages.at(0).toObject()["sha256"].toString(), sha256Of(live + "/net.minecraft/index.json"));
    }

    void test_keepsExistingVersionsAndPackages()
    {
        QTemporaryDir dir;
        const QString staged = dir.filePath("staged"), live = dir.filePath("live");
        QVERIFY(stageMinecraft263(staged));
        QVERIFY(writeJson(live + "/net.minecraft/26.1.json", { { "version", "26.1" } }));
        QVERIFY(writeJson(live + "/net.minecraft/index.json", packageIndex({ versionEntry("26.1", sha256Of(live + "/net.minecraft/26.1.json")) })));
        const QJsonObject lwjgl{ { "uid", "org.lwjgl3" }, { "name", "LWJGL 3" }, { "sha256", "keepme" } };
        QVERIFY(writeJson(live + "/index.json", rootIndex({ lwjgl, QJsonObject{ { "uid", "net.minecraft" }, { "name", "Minecraft" }, { "sha256", "old" } } })));

        const auto result = OfflineBundle::mergeMeta(staged, live);
        QVERIFY2(result.ok, qPrintable(result.details));
        QCOMPARE(versionsIn(readJson(live + "/net.minecraft/index.json")), (QStringList{ "26.1", "26.3" }));
        const QJsonArray packages = readJson(live + "/index.json")["packages"].toArray();
        QCOMPARE(packages.size(), 2);
        QCOMPARE(packages.at(0).toObject(), lwjgl);
        QCOMPARE(packages.at(1).toObject()["sha256"].toString(), sha256Of(live + "/net.minecraft/index.json"));
    }

    void test_replacesAnExistingEntryForTheSameVersion()
    {
        QTemporaryDir dir;
        const QString staged = dir.filePath("staged"), live = dir.filePath("live");
        QVERIFY(stageMinecraft263(staged));
        QVERIFY(writeJson(live + "/net.minecraft/index.json", packageIndex({ versionEntry("26.3", "outdated") })));

        QVERIFY(OfflineBundle::mergeMeta(staged, live).ok);
        const QJsonArray versions = readJson(live + "/net.minecraft/index.json")["versions"].toArray();
        QCOMPARE(versions.size(), 1);
        QCOMPARE(versions.at(0).toObject()["sha256"].toString(), sha256Of(live + "/net.minecraft/26.3.json"));
    }

    void test_reimportIsIdempotent()
    {
        QTemporaryDir dir;
        const QString staged = dir.filePath("staged"), live = dir.filePath("live");
        QVERIFY(stageMinecraft263(staged));
        QVERIFY(OfflineBundle::mergeMeta(staged, live).ok);
        const QByteArray index = readBytes(live + "/net.minecraft/index.json");
        const QByteArray root = readBytes(live + "/index.json");
        QVERIFY(OfflineBundle::mergeMeta(staged, live).ok);
        QCOMPARE(readBytes(live + "/net.minecraft/index.json"), index);
        QCOMPARE(readBytes(live + "/index.json"), root);
    }

    void test_packageWithoutIndexFails()
    {
        QTemporaryDir dir;
        const QString staged = dir.filePath("staged"), live = dir.filePath("live");
        QVERIFY(writeJson(staged + "/net.minecraft/26.3.json", { { "version", "26.3" } }));
        const auto result = OfflineBundle::mergeMeta(staged, live);
        QVERIFY(!result.ok);
        QVERIFY(result.details.contains("index.json"));
    }
};

QTEST_GUILESS_MAIN(MetaIndexMergerTest)

#include "MetaIndexMerger_test.moc"
