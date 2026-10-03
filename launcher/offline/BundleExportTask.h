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
#include <QHash>
#include <QString>
#include <QStringList>
#include <atomic>
#include <functional>
#include <memory>
#include <vector>

#include "RuntimeContext.h"
#include "offline/BundleManifest.h"
#include "offline/BundleWriter.h"
#include "offline/ExportCollector.h"
#include "offline/ExportRules.h"
#include "tasks/Task.h"

class MinecraftInstance;

namespace OfflineBundle {

struct ExportRequest {
    Kind kind = Kind::Versions;
    QList<ExportSet> sets;         // versions bundles
    QString instanceId;            // instance bundles
    bool includeWorlds = false;    // instance bundles
    QStringList extraJavaFolders;  // more folders under java/ to include
    QString outputPath;            // the .zip to write
};

// Builds an offline bundle (spec §5.3). For every set (or the instance) it makes a throwaway instance under
// <data>/.export-staging/<id>/, resolves and downloads everything with the launcher's own tasks for Windows x64, runs
// the Forge/NeoForge processors if needed, collects the files and finally writes and verifies the zip on a worker
// thread. The staging folder is always deleted. Nothing in the user's instances changes.
class ExportTask : public Task {
    Q_OBJECT
   public:
    explicit ExportTask(ExportRequest request);
    ~ExportTask() override;

    static QString stagingRoot(const QString& dataRoot);

    QString errorDetails() const { return m_errorDetails; }
    qint64 bundleSize() const { return m_bundleSize; }
    Manifest manifest() const { return m_manifest; }
    bool abort() override;

   protected:
    void executeTask() override;

   private:
    struct Job {
        QString label;
        std::unique_ptr<MinecraftInstance> instance;
    };

    bool addSetJob(int index, const ExportSet& set);
    bool addInstanceJob();
    std::unique_ptr<MinecraftInstance> makeInstance(const QString& dir);
    void startJob();
    void afterJava(MinecraftInstance* inst, const QString& javaFolderName, const QList<int>& majors);
    void collectJob(MinecraftInstance* inst, const QStringList& forgeOutputs);
    void addElyPatch(MinecraftInstance* inst);
    void nextJob();
    bool addJavaFolder(const QString& folderName, int fallbackMajor);
    void startWriting();
    void finishWriting();
    void runTask(Task::Ptr task, const QString& item, std::function<void()> next);
    void fail(const QString& message, const QString& details);
    void finishCancelled();
    void cleanUp();

    ExportRequest m_request;
    QString m_dataRoot;
    QString m_staging;
    std::vector<Job> m_jobs;
    size_t m_jobIndex = 0;
    RuntimeContext m_context;
    FileSet m_files;
    Manifest m_manifest;
    QStringList m_javaFolders;
    Task::Ptr m_current;
    QList<Task::Ptr> m_keepAlive;  // every started step, so none is freed while it is still emitting a signal
    std::atomic_bool m_cancelled = false;
    bool m_done = false;
    QString m_errorDetails;
    qint64 m_bundleSize = 0;
    QFuture<WriteResult> m_future;
    QFutureWatcher<WriteResult> m_watcher;
};

}  // namespace OfflineBundle
