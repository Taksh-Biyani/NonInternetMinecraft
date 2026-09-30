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

#pragma once

#include <QFuture>
#include <QFutureWatcher>
#include <QStringList>
#include <atomic>
#include <optional>

#include "offline/BundleManifest.h"
#include "tasks/Task.h"

namespace OfflineBundle {

// Imports an offline bundle into the data folder (spec §3.2 steps 3-5 and 8): checks free space, extracts every file
// into <data>/.import-staging/<id> while verifying its SHA-1 and size, and only then merges into the live folders.
// The staging folder is always deleted. Cancelling is possible until merging starts. The instance of an instance
// bundle is created afterwards by the caller (InstanceImportTask on the same zip).
class ImportTask : public Task {
    Q_OBJECT
   public:
    ImportTask(QString bundlePath, Manifest manifest, QString dataRoot, QString javaDir);
    ~ImportTask() override;

    static QString stagingRoot(const QString& dataRoot);

    QString errorDetails() const { return m_errorDetails; }
    QStringList changedMetaUids() const { return m_changedMetaUids; }
    QStringList installedJavaFolders() const { return m_installedJavaFolders; }

    bool abort() override;

   protected:
    void executeTask() override;

   private:
    struct Failure {
        QString message;
        QString details;
    };
    using Outcome = std::optional<Failure>;

    Outcome doImport();
    Outcome extractAndVerify(const QString& staging);
    Outcome mergeFiles(const QString& staging);
    Outcome mergeJava(const QString& staging);
    void finish();

    QString m_bundlePath;
    Manifest m_manifest;
    QString m_dataRoot;
    QString m_javaDir;
    std::atomic_bool m_cancelled = false;
    std::atomic_bool m_merging = false;
    QString m_errorDetails;
    QStringList m_changedMetaUids;
    QStringList m_installedJavaFolders;
    QFuture<Outcome> m_future;
    QFutureWatcher<Outcome> m_watcher;
};

}  // namespace OfflineBundle
