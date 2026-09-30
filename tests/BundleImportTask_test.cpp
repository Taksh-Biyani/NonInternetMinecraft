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
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <archive/ArchiveWriter.h>
#include <offline/BundleImportTask.h>
#include <offline/BundleManifest.h>
#include <offline/BundleMessages.h>
#include <offline/BundlePaths.h>

struct BundleFile {
    QString path;
    QByteArray data;
};

static QString sha1Hex(const QByteArray& data)
{
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex());
}

static QByteArray json(const QJsonObject& obj)
{
    return QJsonDocument(obj).toJson();
}

static QList<BundleFile> sampleFiles()
{
    return {
        { "meta/index.json", json({ { "formatVersion", 1 }, { "packages", QJsonArray{ QJsonObject{ { "uid", "net.minecraft" }, { "name", "Minecraft" } } } } }) },
        { "meta/net.minecraft/index.json",
          json({ { "formatVersion", 1 }, { "uid", "net.minecraft" }, { "name", "Minecraft" },
                 { "versions", QJsonArray{ QJsonObject{ { "version", "26.3" }, { "sha256", "x" } } } } }) },
        { "meta/net.minecraft/26.3.json", json({ { "uid", "net.minecraft" }, { "version", "26.3" } }) },
        { "libraries/org/example/lib/1.0/lib-1.0.jar", QByteArray("library bytes") },
        { "assets/objects/ab/abcdef", QByteArray("asset bytes") },
        { "java/rt/bin/java.exe", QByteArray("java bytes") },
    };
}

// `listed` go into the manifest; `zipped` into the zip. Normally the same list.
static bool writeBundle(const QString& zipPath, const QList<BundleFile>& listed, const QList<BundleFile>& zipped)
{
    QJsonArray files;
    for (const BundleFile& f : listed)
        files.append(QJsonObject{ { "path", f.path }, { "sha1", sha1Hex(f.data) }, { "size", f.data.size() } });
    const QJsonObject manifest{ { "formatVersion", 1 }, { "kind", "versions" }, { "name", "Test bundle" },
                                { "contents", QJsonObject{ { "components", QJsonArray{ QJsonObject{ { "uid", "net.minecraft" }, { "version", "26.3" } } } } } },
                                { "files", files } };
    MMCZip::ArchiveWriter zip(zipPath);
    if (!zip.open() || !zip.addFile(QString(OfflineBundle::ManifestFileName), json(manifest)))
        return false;
    for (const BundleFile& f : zipped) {
        if (!zip.addFile(f.path, f.data))
            return false;
    }
    return zip.close();
}

static QByteArray readBytes(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

class BundleImportTaskTest : public QObject {
    Q_OBJECT

    struct Run {
        bool succeeded = false;
        QString failReason;
        QStringList changedUids;
        QStringList javaFolders;
    };

    static Run import(const QString& zip, const QString& dataRoot, const QString& javaDir)
    {
        const auto read = OfflineBundle::readManifestFromZip(zip);
        if (!read.manifest)
            return { false, read.message, {}, {} };
        OfflineBundle::ImportTask task(zip, *read.manifest, dataRoot, javaDir);
        QSignalSpy finished(&task, &Task::finished);
        task.start();
        if (!finished.wait(20000))
            return { false, "timeout", {}, {} };
        return { task.wasSuccessful(), task.failReason(), task.changedMetaUids(), task.installedJavaFolders() };
    }

   private slots:
    void test_importsEverythingAndCleansUp()
    {
        QTemporaryDir dir;
        const QString zip = dir.filePath("bundle.zip"), data = dir.filePath("data"), javaDir = dir.filePath("data/java");
        QVERIFY(writeBundle(zip, sampleFiles(), sampleFiles()));

        const Run run = import(zip, data, javaDir);
        QVERIFY2(run.succeeded, qPrintable(run.failReason));
        QCOMPARE(readBytes(data + "/libraries/org/example/lib/1.0/lib-1.0.jar"), QByteArray("library bytes"));
        QCOMPARE(readBytes(data + "/assets/objects/ab/abcdef"), QByteArray("asset bytes"));
        QCOMPARE(readBytes(javaDir + "/rt/bin/java.exe"), QByteArray("java bytes"));
        QVERIFY(QFile::exists(data + "/meta/net.minecraft/26.3.json"));
        QCOMPARE(run.changedUids, QStringList{ "net.minecraft" });
        QCOMPARE(run.javaFolders, QStringList{ "rt" });
        QVERIFY(QDir(OfflineBundle::ImportTask::stagingRoot(data)).isEmpty());
    }

    void test_reimportChangesNothing()
    {
        QTemporaryDir dir;
        const QString zip = dir.filePath("bundle.zip"), data = dir.filePath("data"), javaDir = dir.filePath("data/java");
        QVERIFY(writeBundle(zip, sampleFiles(), sampleFiles()));
        QVERIFY(import(zip, data, javaDir).succeeded);
        const Run again = import(zip, data, javaDir);
        QVERIFY2(again.succeeded, qPrintable(again.failReason));
        QCOMPARE(again.javaFolders, QStringList{ "rt" });  // identical runtime isn't copied a second time
        QVERIFY(!QFile::exists(javaDir + "/rt-2"));
    }

    void test_differentJavaWithSameFolderNameGetsSuffix()
    {
        QTemporaryDir dir;
        const QString zip = dir.filePath("bundle.zip"), data = dir.filePath("data"), javaDir = dir.filePath("data/java");
        QVERIFY(writeBundle(zip, sampleFiles(), sampleFiles()));
        QVERIFY(QDir().mkpath(javaDir + "/rt/bin"));
        QFile other(javaDir + "/rt/bin/java.exe");
        QVERIFY(other.open(QIODevice::WriteOnly));
        other.write("someone else's java");
        other.close();

        const Run run = import(zip, data, javaDir);
        QVERIFY2(run.succeeded, qPrintable(run.failReason));
        QCOMPARE(readBytes(javaDir + "/rt/bin/java.exe"), QByteArray("someone else's java"));  // never overwritten
        QCOMPARE(readBytes(javaDir + "/rt-2/bin/java.exe"), QByteArray("java bytes"));
        QCOMPARE(run.javaFolders, QStringList{ "rt-2" });
    }

    void test_corruptFileChangesNothing()
    {
        QTemporaryDir dir;
        const QString zip = dir.filePath("bundle.zip"), data = dir.filePath("data"), javaDir = dir.filePath("data/java");
        QList<BundleFile> zipped = sampleFiles();
        zipped[3].data = "library bytez";  // same size, different content
        QVERIFY(writeBundle(zip, sampleFiles(), zipped));

        const Run run = import(zip, data, javaDir);
        QVERIFY(!run.succeeded);
        QCOMPARE(run.failReason, OfflineBundle::Messages::damaged());
        QVERIFY(!QFile::exists(data + "/libraries"));
        QVERIFY(!QFile::exists(data + "/meta"));
        QVERIFY(!QFile::exists(javaDir));
        QVERIFY(QDir(OfflineBundle::ImportTask::stagingRoot(data)).isEmpty());
    }

    void test_unlistedFileIsRejected()
    {
        QTemporaryDir dir;
        const QString zip = dir.filePath("bundle.zip"), data = dir.filePath("data");
        QList<BundleFile> zipped = sampleFiles();
        zipped.append({ "libraries/extra.jar", QByteArray("sneaky") });
        QVERIFY(writeBundle(zip, sampleFiles(), zipped));
        const Run run = import(zip, data, data + "/java");
        QVERIFY(!run.succeeded);
        QCOMPARE(run.failReason, OfflineBundle::Messages::damaged());
        QVERIFY(!QFile::exists(data + "/libraries"));
    }

    void test_missingFileIsRejected()
    {
        QTemporaryDir dir;
        const QString zip = dir.filePath("bundle.zip"), data = dir.filePath("data");
        QList<BundleFile> zipped = sampleFiles();
        zipped.removeLast();
        QVERIFY(writeBundle(zip, sampleFiles(), zipped));
        const Run run = import(zip, data, data + "/java");
        QVERIFY(!run.succeeded);
        QCOMPARE(run.failReason, OfflineBundle::Messages::damaged());
    }

    void test_unsafeEntryIsRejected()
    {
        QTemporaryDir dir;
        const QString zip = dir.filePath("bundle.zip"), data = dir.filePath("data");
        QList<BundleFile> zipped = sampleFiles();
        zipped.append({ "saves/evil.txt", QByteArray("x") });
        QVERIFY(writeBundle(zip, sampleFiles(), zipped));
        const Run run = import(zip, data, data + "/java");
        QVERIFY(!run.succeeded);
        QCOMPARE(run.failReason, OfflineBundle::Messages::unsafe());
        QVERIFY(!QFile::exists(dir.filePath("saves")));
        QVERIFY(!QFile::exists(data + "/saves"));
    }

    void test_changedLibraryIsReplaced()
    {
        QTemporaryDir dir;
        const QString zip = dir.filePath("bundle.zip"), data = dir.filePath("data");
        QVERIFY(writeBundle(zip, sampleFiles(), sampleFiles()));
        QVERIFY(QDir().mkpath(data + "/libraries/org/example/lib/1.0"));
        QFile old(data + "/libraries/org/example/lib/1.0/lib-1.0.jar");
        QVERIFY(old.open(QIODevice::WriteOnly));
        old.write("old library");
        old.close();
        QVERIFY(import(zip, data, data + "/java").succeeded);
        QCOMPARE(readBytes(data + "/libraries/org/example/lib/1.0/lib-1.0.jar"), QByteArray("library bytes"));
    }
};

QTEST_GUILESS_MAIN(BundleImportTaskTest)

#include "BundleImportTask_test.moc"
