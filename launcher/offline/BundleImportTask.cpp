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

#include "BundleImportTask.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QSet>
#include <QStorageInfo>
#include <QUuid>
#include <QtConcurrent>

#include "FileSystem.h"
#include "archive/ArchiveReader.h"
#include "offline/BundleMessages.h"
#include "offline/BundlePaths.h"
#include "offline/MetaIndexMerger.h"

namespace OfflineBundle {
namespace {

QString sha1Of(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(&file);
    return QString::fromLatin1(hash.result().toHex());
}

bool sameFile(const QString& path, const FileEntry& expected)
{
    const QFileInfo info(path);
    return info.isFile() && info.size() == expected.size && sha1Of(path) == expected.sha1;
}

// Moves a verified staged file into place (same drive, so a rename), replacing an older copy.
bool placeFile(const QString& from, const QString& to)
{
    QDir().mkpath(QFileInfo(to).absolutePath());
    if (QFileInfo::exists(to) && !QFile::remove(to))
        return false;
    return QFile::rename(from, to) || QFile::copy(from, to);
}

}  // namespace

ImportTask::ImportTask(QString bundlePath, Manifest manifest, QString dataRoot, QString javaDir)
    : m_bundlePath(std::move(bundlePath)), m_manifest(std::move(manifest)), m_dataRoot(std::move(dataRoot)), m_javaDir(std::move(javaDir))
{}

ImportTask::~ImportTask()
{
    m_cancelled = true;
    m_future.waitForFinished();
}

QString ImportTask::stagingRoot(const QString& dataRoot)
{
    return QDir(dataRoot).filePath(".import-staging");
}

void ImportTask::executeTask()
{
    setAbortable(true);
    connect(&m_watcher, &QFutureWatcher<Outcome>::finished, this, &ImportTask::finish);
    m_future = QtConcurrent::run(QThreadPool::globalInstance(), [this] { return doImport(); });
    m_watcher.setFuture(m_future);
}

bool ImportTask::abort()
{
    if (m_merging)
        return false;
    m_cancelled = true;
    return true;
}

void ImportTask::finish()
{
    const Outcome outcome = m_future.result();
    if (outcome) {
        m_errorDetails = outcome->details;
        emitFailed(outcome->message);
    } else if (m_cancelled && !m_merging) {
        emitAborted();
    } else {
        emitSucceeded();
    }
}

auto ImportTask::doImport() -> Outcome
{
    const QString staging = QDir(stagingRoot(m_dataRoot)).filePath(QUuid::createUuid().toString(QUuid::Id128));
    const Outcome outcome = [this, &staging]() -> Outcome {
        const qint64 total = m_manifest.totalSize();
        const qint64 needed = total + total / 10 + qint64(64) * 1024 * 1024;
        QDir().mkpath(m_dataRoot);
        const QStorageInfo storage(m_dataRoot);
        if (storage.isValid() && storage.bytesAvailable() < needed)
            return Failure{ Messages::notEnoughSpace(needed, storage.bytesAvailable()),
                            QString("need %1 bytes, %2 free on %3").arg(needed).arg(storage.bytesAvailable()).arg(storage.rootPath()) };
        if (!QDir().mkpath(staging))
            return Failure{ Messages::couldNotWrite(), QString("can't create %1").arg(staging) };

        if (Outcome failure = extractAndVerify(staging))
            return failure;
        if (m_cancelled)
            return std::nullopt;  // finish() reports the cancel

        m_merging = true;
        setAbortable(false);
        if (Outcome failure = mergeFiles(staging))
            return failure;
        if (Outcome failure = mergeJava(staging))
            return failure;
        setStatus(tr("Installing version information..."));
        const MetaMergeResult merged = mergeMeta(QDir(staging).filePath("meta"), QDir(m_dataRoot).filePath("meta"));
        if (!merged.ok)
            return Failure{ Messages::couldNotWrite(), merged.details };
        m_changedMetaUids = merged.changedUids;
        return std::nullopt;
    }();
    setStatus(tr("Cleaning up..."));
    FS::deletePath(staging);
    return outcome;
}

auto ImportTask::extractAndVerify(const QString& staging) -> Outcome
{
    QHash<QString, const FileEntry*> expected;
    for (const FileEntry& file : m_manifest.files)
        expected.insert(file.path, &file);
    QSet<QString> seen;
    Outcome failure;
    const qint64 total = m_manifest.files.size();
    qint64 done = 0;
    setStatus(tr("Checking and unpacking files..."));
    setProgress(0, total);

    MMCZip::ArchiveReader zip(m_bundlePath);
    const bool parsed = zip.parse([&](MMCZip::ArchiveReader::File* f, bool& stop) {
        if (m_cancelled) {
            stop = true;
            return true;
        }
        const QString path = normalizeEntryPath(f->filename());
        if (path == ManifestFileName)
            return f->skip();
        if (const QString reason = checkEntryPath(path); !reason.isEmpty()) {
            failure = Failure{ Messages::unsafe(), QString("%1: %2").arg(path, reason) };
            return false;
        }
        if (f->isDirectory())
            return f->skip();
        if (!f->isFile()) {
            failure = Failure{ Messages::unsafe(), QString("%1: links and special files aren't allowed").arg(path) };
            return false;
        }
        const FileEntry* entry = expected.value(path, nullptr);
        if (!entry) {
            failure = Failure{ Messages::damaged(), QString("%1 isn't listed in the manifest").arg(path) };
            return false;
        }
        if (seen.contains(path)) {
            failure = Failure{ Messages::damaged(), QString("%1 appears twice in the zip").arg(path) };
            return false;
        }
        seen.insert(path);

        const QString target = QDir(staging).filePath(path);
        QDir().mkpath(QFileInfo(target).absolutePath());
        QFile out(target);
        if (!out.open(QIODevice::WriteOnly)) {
            failure = Failure{ Messages::couldNotWrite(), QString("can't write %1").arg(target) };
            return false;
        }
        QCryptographicHash sha1(QCryptographicHash::Sha1);
        const bool written = f->writeTo(out, &sha1);
        const qint64 size = out.size();
        out.close();
        if (!written) {
            failure = Failure{ Messages::damaged(), QString("%1 couldn't be read from the zip").arg(path) };
            return false;
        }
        const QString actual = QString::fromLatin1(sha1.result().toHex());
        if (size != entry->size || actual != entry->sha1) {
            failure = Failure{ Messages::damaged(), QString("%1: expected %2 bytes with SHA-1 %3, got %4 bytes with SHA-1 %5")
                                                        .arg(path)
                                                        .arg(entry->size)
                                                        .arg(entry->sha1)
                                                        .arg(size)
                                                        .arg(actual) };
            return false;
        }
        ++done;
        if (done % 50 == 0 || done == total) {
            setProgress(done, total);
            setStatus(tr("Checking and unpacking files (%1 of %2)...").arg(done).arg(total));
        }
        return true;
    });
    if (failure)
        return failure;
    if (m_cancelled)
        return std::nullopt;
    if (!parsed)
        return Failure{ Messages::damaged(), "the zip file couldn't be read" };
    for (auto it = expected.cbegin(); it != expected.cend(); ++it) {
        if (!seen.contains(it.key()))
            return Failure{ Messages::damaged(), QString("%1 is missing from the zip").arg(it.key()) };
    }
    return std::nullopt;
}

auto ImportTask::mergeFiles(const QString& staging) -> Outcome
{
    setStatus(tr("Installing libraries and game files..."));
    for (const FileEntry& file : m_manifest.files) {
        if (!file.path.startsWith("libraries/") && !file.path.startsWith("assets/"))
            continue;
        const QString to = QDir(m_dataRoot).filePath(file.path);
        if (sameFile(to, file))
            continue;
        if (!placeFile(QDir(staging).filePath(file.path), to))
            return Failure{ Messages::couldNotWrite(), QString("can't write %1").arg(to) };
    }
    return std::nullopt;
}

auto ImportTask::mergeJava(const QString& staging) -> Outcome
{
    const QDir stagedJava(QDir(staging).filePath("java"));
    if (!stagedJava.exists())
        return std::nullopt;
    setStatus(tr("Installing Java..."));
    QDir().mkpath(m_javaDir);
    const QStringList runtimes = stagedJava.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString& runtime : runtimes) {
        const QString prefix = "java/" + runtime + "/";
        QString target = QDir(m_javaDir).filePath(runtime);
        if (QFileInfo::exists(target)) {
            bool identical = true;
            for (const FileEntry& file : m_manifest.files) {
                if (file.path.startsWith(prefix) && !sameFile(QDir(target).filePath(file.path.mid(prefix.size())), file)) {
                    identical = false;
                    break;
                }
            }
            if (identical) {
                m_installedJavaFolders.append(runtime);
                continue;
            }
            int suffix = 2;
            while (QFileInfo::exists(QDir(m_javaDir).filePath(QString("%1-%2").arg(runtime).arg(suffix))))
                ++suffix;
            target = QDir(m_javaDir).filePath(QString("%1-%2").arg(runtime).arg(suffix));
        }
        const QString from = stagedJava.filePath(runtime);
        if (!QDir().rename(from, target) && !FS::copy(from, target)())
            return Failure{ Messages::couldNotWrite(), QString("can't write %1").arg(target) };
        m_installedJavaFolders.append(QFileInfo(target).fileName());
    }
    return std::nullopt;
}

}  // namespace OfflineBundle
