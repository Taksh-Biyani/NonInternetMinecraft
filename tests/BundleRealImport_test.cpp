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
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <memory>

#include <meta/Index.h>
#include <meta/Version.h>
#include <meta/VersionList.h>
#include <offline/BundleImportTask.h>
#include <offline/BundleManifest.h>
#include <offline/BundleMessages.h>

// Opt-in check against real bundles (e.g. from scripts/offline/make-test-bundle.ps1). Skipped unless the environment
// variables are set: PINECONE_TEST_BUNDLE (a good bundle) and PINECONE_TEST_DAMAGED_BUNDLE (a corrupted copy).
class BundleRealImportTest : public QObject {
    Q_OBJECT

    static bool runImport(const QString& zip, const OfflineBundle::Manifest& manifest, const QString& data, OfflineBundle::ImportTask*& out)
    {
        out = new OfflineBundle::ImportTask(zip, manifest, data, QDir(data).filePath("java"));
        QSignalSpy finished(out, &Task::finished);
        out->start();
        return finished.wait(15 * 60 * 1000);
    }

   private slots:
    void test_realBundleImportsAndLoadsOffline()
    {
        const QString zip = qEnvironmentVariable("PINECONE_TEST_BUNDLE");
        if (zip.isEmpty())
            QSKIP("set PINECONE_TEST_BUNDLE to a bundle zip to run this");
        const auto read = OfflineBundle::readManifestFromZip(zip);
        QVERIFY2(read.manifest.has_value(), qPrintable(read.details));

        QTemporaryDir dir;
        OfflineBundle::ImportTask* task = nullptr;
        QVERIFY(runImport(zip, *read.manifest, dir.path(), task));
        std::unique_ptr<OfflineBundle::ImportTask> owner(task);
        QVERIFY2(task->wasSuccessful(), qPrintable(task->failReason() + " | " + task->errorDetails()));
        QVERIFY(QDir(OfflineBundle::ImportTask::stagingRoot(dir.path())).isEmpty());

        const QString oldCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(dir.path()));
        Meta::Index index;
        index.loadTask(Net::Mode::Offline)->start();
        for (const auto& component : read.manifest->components) {
            auto list = index.get(component.uid);
            QVERIFY2(list, qPrintable(component.uid));
            list->loadTask(Net::Mode::Offline)->start();
            auto version = list->getVersion(component.version);
            QVERIFY2(version, qPrintable(component.uid + " " + component.version));
            version->loadTask(Net::Mode::Offline)->start();
            QVERIFY2(version->isLoaded(), qPrintable(component.uid + " " + component.version + " didn't load offline"));
        }
        QVERIFY(QDir::setCurrent(oldCwd));
    }

    void test_damagedBundleIsRejected()
    {
        const QString zip = qEnvironmentVariable("PINECONE_TEST_DAMAGED_BUNDLE");
        if (zip.isEmpty())
            QSKIP("set PINECONE_TEST_DAMAGED_BUNDLE to a corrupted bundle zip to run this");
        const auto read = OfflineBundle::readManifestFromZip(zip);
        QVERIFY2(read.manifest.has_value(), qPrintable(read.details));

        QTemporaryDir dir;
        OfflineBundle::ImportTask* task = nullptr;
        QVERIFY(runImport(zip, *read.manifest, dir.path(), task));
        std::unique_ptr<OfflineBundle::ImportTask> owner(task);
        QVERIFY(!task->wasSuccessful());
        QCOMPARE(task->failReason(), OfflineBundle::Messages::damaged());
        qInfo() << "details:" << task->errorDetails();
        QVERIFY(!QFile::exists(dir.filePath("libraries")));
        QVERIFY(!QFile::exists(dir.filePath("meta")));
        QVERIFY(QDir(OfflineBundle::ImportTask::stagingRoot(dir.path())).isEmpty());
    }
};

QTEST_GUILESS_MAIN(BundleRealImportTest)

#include "BundleRealImport_test.moc"
