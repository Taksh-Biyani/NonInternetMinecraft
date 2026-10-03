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

#include "ExportJavaTask.h"

#include <QDir>
#include <QFileInfo>

#include "Application.h"
#include "FileSystem.h"
#include "java/JavaMetadata.h"
#include "java/JavaUtils.h"
#include "java/download/ArchiveDownloadTask.h"
#include "java/download/ManifestDownloadTask.h"
#include "meta/Index.h"
#include "meta/VersionList.h"
#include "net/Mode.h"

namespace OfflineBundle {

static const QString s_windowsX64 = "windows-x64";

ExportJavaTask::ExportJavaTask(QString javaName, QList<int> majors, QString javaRoot)
    : m_javaName(std::move(javaName)), m_majors(std::move(majors)), m_javaRoot(std::move(javaRoot))
{}

QString ExportJavaTask::javaFolder() const
{
    return QDir(m_javaRoot).absoluteFilePath(m_javaName);
}

bool ExportJavaTask::installed() const
{
    return QFileInfo::exists(FS::PathCombine(javaFolder(), "bin", JavaUtils::javaExecutable));
}

void ExportJavaTask::executeTask()
{
    setStatus(tr("Getting Java..."));
    if (m_javaName.isEmpty()) {
        emitFailed(tr("The metadata for this Minecraft version doesn't say which Java it needs."));
        return;
    }
    if (installed()) {
        emitSucceeded();
        return;
    }
    auto versionList = APPLICATION->metadataIndex()->get("net.minecraft.java");
    auto load = versionList->loadTask(Net::Mode::Online);
    connect(load.get(), &Task::succeeded, this, &ExportJavaTask::tryNextMajor);
    follow(load);
}

void ExportJavaTask::follow(Task::Ptr task)
{
    m_current = task;
    connect(task.get(), &Task::failed, this, [this](const QString& reason) {
        // A failed metadata load or download: try the next major version, if any.
        if (m_next < m_majors.size())
            tryNextMajor();
        else
            emitFailed(reason);
    });
    connect(task.get(), &Task::progress, this, &ExportJavaTask::setProgress);
    connect(task.get(), &Task::status, this, &ExportJavaTask::setDetails);
    if (!task->isRunning())
        task->start();
}

void ExportJavaTask::tryNextMajor()
{
    if (!isRunning())
        return;
    if (m_next >= m_majors.size()) {
        emitFailed(tr("No %1 build of Java %2 was found.").arg(s_windowsX64, m_javaName));
        return;
    }
    const int major = m_majors.at(m_next++);
    auto versionList = APPLICATION->metadataIndex()->get("net.minecraft.java");
    auto version = versionList->getVersion(QString("java%1").arg(major));
    if (version->isLoaded()) {
        download(version);
        return;
    }
    auto load = APPLICATION->metadataIndex()->loadVersion("net.minecraft.java", version->version(), Net::Mode::Online);
    connect(load.get(), &Task::succeeded, this, [this, version] { download(version); });
    follow(load);
}

void ExportJavaTask::download(Meta::Version::Ptr version)
{
    for (const auto& java : version->data()->runtimes) {
        if (java->runtimeOS != s_windowsX64 || java->name() != m_javaName)
            continue;
        const QString target = javaFolder();
        Task::Ptr task;
        if (java->downloadType == Java::DownloadType::Manifest)
            task = makeShared<Java::ManifestDownloadTask>(java->url, target, java->checksumType, java->checksumHash);
        else if (java->downloadType == Java::DownloadType::Archive)
            task = makeShared<Java::ArchiveDownloadTask>(java->url, target, java->checksumType, java->checksumHash);
        else
            continue;
        setStatus(tr("Downloading %1...").arg(m_javaName));
        connect(task.get(), &Task::succeeded, this, [this] {
            if (installed())
                emitSucceeded();
            else
                emitFailed(tr("The downloaded Java has no %1.").arg(JavaUtils::javaExecutable));
        });
        connect(task.get(), &Task::failed, this, [target] { FS::deletePath(target); });
        follow(task);
        return;
    }
    tryNextMajor();
}

bool ExportJavaTask::abort()
{
    if (m_current && m_current->isRunning())
        m_current->abort();
    emitAborted();
    return true;
}

}  // namespace OfflineBundle
