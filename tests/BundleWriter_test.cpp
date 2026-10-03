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
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <atomic>

#include <offline/BundleImportTask.h>
#include <offline/BundleManifest.h>
#include <offline/BundleWriter.h>
#include <offline/ExportCollector.h>

using namespace OfflineBundle;

static void put(const QString& path, const QByteArray& data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(data);
}

static QByteArray readFile(const QString& path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray("<missing>");
}

static QByteArray json(const QJsonObject& obj)
{
    return QJsonDocument(obj).toJson();
}

// A small but valid versions bundle source: metadata the importer can merge, a library, an asset, a Java file.
static void makeSource(const QString& root)
{
    put(root + "/meta/index.json",
        json({ { "formatVersion", 1 }, { "packages", QJsonArray{ QJsonObject{ { "uid", "net.minecraft" }, { "name", "Minecraft" } } } } }));
    put(root + "/meta/net.minecraft/index.json",
        json({ { "formatVersion", 1 }, { "uid", "net.minecraft" }, { "name", "Minecraft" },
               { "versions", QJsonArray{ QJsonObject{ { "version", "26.3" }, { "sha256", "x" } } } } }));
    put(root + "/meta/net.minecraft/26.3.json", json({ { "uid", "net.minecraft" }, { "version", "26.3" } }));
    put(root + "/libraries/org/example/lib/1.0/lib-1.0.jar", QByteArray(100000, 'j'));
    put(root + "/assets/objects/ab/abcdef", "asset bytes");
    put(root + "/java/rt/bin/java.exe", "java bytes");
}

static FileSet sourceFiles(const QString& root)
{
    FileSet fs;
    for (const QString& top : { "meta", "libraries", "assets", "java" })
        addTree(fs, root + '/' + top, top);
    return fs;
}

static Manifest versionsManifest()
{
    Manifest m;
    m.formatVersion = 1;
    m.kind = Kind::Versions;
    m.name = "Test bundle";
    m.components = { { "net.minecraft", "26.3", "Minecraft" } };
    m.java = { { "Java 25", 25, "rt" } };
    return m;
}

class BundleWriterTest : public QObject {
    Q_OBJECT
   private slots:
    void roundTripThroughImporter()
    {
        QTemporaryDir source, live, out;
        makeSource(source.path());
        FileSet fs = sourceFiles(source.path());
        QVERIFY(fs.addData("meta/net.minecraft/extra.txt", "in-memory data"));
        const QString zip = out.filePath("bundle.zip");
        std::atomic_bool cancelled = false;
        int progressCalls = 0;
        const auto result = writeBundle(zip, versionsManifest(), fs, [&](const QString&, qint64, qint64) { ++progressCalls; }, cancelled);
        QVERIFY2(result.ok, qPrintable(result.details));
        QVERIFY(result.zipSize > 0);
        QVERIFY(progressCalls > 0);
        QVERIFY(!QFile::exists(zip + ".part"));

        const auto read = readManifestFromZip(zip);
        QVERIFY2(read.manifest, qPrintable(read.details));
        QCOMPARE(read.manifest->files.size(), fs.entries().size());

        ImportTask task(zip, *read.manifest, live.path(), live.filePath("java"));
        QSignalSpy finished(&task, &Task::finished);
        task.start();
        QVERIFY(finished.wait(30000));
        QVERIFY2(task.wasSuccessful(), qPrintable(task.failReason() + " " + task.errorDetails()));
        QCOMPARE(readFile(live.filePath("libraries/org/example/lib/1.0/lib-1.0.jar")), QByteArray(100000, 'j'));
        QCOMPARE(readFile(live.filePath("assets/objects/ab/abcdef")), QByteArray("asset bytes"));
        QCOMPARE(readFile(live.filePath("java/rt/bin/java.exe")), QByteArray("java bytes"));
        QCOMPARE(readFile(live.filePath("meta/net.minecraft/26.3.json")), readFile(source.filePath("meta/net.minecraft/26.3.json")));
    }

    void missingSourceLeavesNothing()
    {
        QTemporaryDir source, out;
        makeSource(source.path());
        FileSet fs = sourceFiles(source.path());
        QFile::remove(source.filePath("assets/objects/ab/abcdef"));
        const QString zip = out.filePath("bundle.zip");
        put(zip, "an older bundle");
        std::atomic_bool cancelled = false;
        const auto result = writeBundle(zip, versionsManifest(), fs, {}, cancelled);
        QVERIFY(!result.ok);
        QVERIFY(!result.cancelled);
        QVERIFY(!result.message.isEmpty());
        QVERIFY(!QFile::exists(zip + ".part"));
        QCOMPARE(readFile(zip), QByteArray("an older bundle"));  // an existing file is only replaced on success
    }

    void cancelLeavesNothing()
    {
        QTemporaryDir source, out;
        makeSource(source.path());
        const QString zip = out.filePath("bundle.zip");
        std::atomic_bool cancelled = true;
        const auto result = writeBundle(zip, versionsManifest(), sourceFiles(source.path()), {}, cancelled);
        QVERIFY(!result.ok);
        QVERIFY(result.cancelled);
        QVERIFY(!QFile::exists(zip));
        QVERIFY(!QFile::exists(zip + ".part"));
    }

    void compressesTheZip()
    {
        QTemporaryDir source, out;
        makeSource(source.path());
        const QString zip = out.filePath("bundle.zip");
        std::atomic_bool cancelled = false;
        QVERIFY(writeBundle(zip, versionsManifest(), sourceFiles(source.path()), {}, cancelled).ok);
        // The 100000-byte jar of repeated 'j' deflates to almost nothing.
        QVERIFY(QFileInfo(zip).size() < 50000);
    }
};

QTEST_GUILESS_MAIN(BundleWriterTest)

#include "BundleWriter_test.moc"
