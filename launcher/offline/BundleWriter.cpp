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

#include "BundleWriter.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QIODevice>
#include <QSet>
#include <optional>

#include "archive/ArchiveReader.h"
#include "archive/ArchiveWriter.h"
#include "offline/BundleMessages.h"
#include "offline/BundlePaths.h"
#include "offline/ExportRules.h"

namespace OfflineBundle {

namespace {
QString tr(const char* text)
{
    return QCoreApplication::translate("OfflineBundle", text);
}

// Swallows data, counting the bytes (used to hash zip entries without keeping them).
class CountingSink : public QIODevice {
   public:
    CountingSink() { open(QIODevice::WriteOnly); }
    qint64 written = 0;

   protected:
    qint64 readData(char*, qint64) override { return -1; }
    qint64 writeData(const char*, qint64 len) override
    {
        written += len;
        return len;
    }
};

struct Hashed {
    QString sha1;
    qint64 size = 0;
};

std::optional<Hashed> hashFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return std::nullopt;
    QCryptographicHash sha1(QCryptographicHash::Sha1);
    if (!sha1.addData(&file))
        return std::nullopt;
    return Hashed{ QString::fromLatin1(sha1.result().toHex()), file.size() };
}
}  // namespace

WriteResult writeBundle(const QString& zipPath,
                        Manifest manifest,
                        const FileSet& files,
                        const WriteProgress& progress,
                        const std::atomic_bool& cancelled)
{
    const QString part = zipPath + ".part";
    QFile::remove(part);
    auto report = [&](const QString& step, qint64 done, qint64 total) {
        if (progress)
            progress(step, done, total);
    };
    auto failed = [&](const QString& details) {
        QFile::remove(part);
        return WriteResult{ false, false, Messages::couldNotWriteBundle(), details, 0 };
    };
    auto stopped = [&] {
        QFile::remove(part);
        return WriteResult{ false, true, {}, "cancelled", 0 };
    };
    const qint64 total = files.entries().size();

    // 1. Hash every file for the manifest.
    manifest.files.clear();
    qint64 done = 0;
    qint64 alreadyCompressedBytes = 0;
    qint64 compressibleBytes = 0;
    for (const FileSet::Entry& e : files.entries()) {
        if (cancelled)
            return stopped();
        FileEntry entry{ e.bundlePath, {}, 0 };
        if (e.sourcePath.isEmpty()) {
            entry.sha1 = QString::fromLatin1(QCryptographicHash::hash(e.data, QCryptographicHash::Sha1).toHex());
            entry.size = e.data.size();
        } else if (const auto hashed = hashFile(e.sourcePath)) {
            entry.sha1 = hashed->sha1;
            entry.size = hashed->size;
        } else {
            return failed(QString("can't read %1").arg(e.sourcePath));
        }
        (storeUncompressed(e.bundlePath) ? alreadyCompressedBytes : compressibleBytes) += entry.size;
        manifest.files.append(entry);
        report(tr("Checking files"), ++done, total);
    }

    // 2. Write the files, then the manifest. libarchive fixes the zip compression method for the whole archive when
    // it's opened, so store everything when most of the bytes are already compressed (jars, assets) and deflate otherwise.
    {
        MMCZip::ArchiveWriter zip(part);
        zip.setStoreOnly(alreadyCompressedBytes >= compressibleBytes);
        if (!zip.open())
            return failed(QString("can't create %1").arg(part));
        done = 0;
        for (const FileSet::Entry& e : files.entries()) {
            if (cancelled) {
                zip.close();
                return stopped();
            }
            const bool added = e.sourcePath.isEmpty() ? zip.addFile(e.bundlePath, e.data) : zip.addFile(e.sourcePath, e.bundlePath);
            if (!added) {
                zip.close();
                return failed(QString("can't add %1 to the zip").arg(e.bundlePath));
            }
            report(tr("Writing the bundle"), ++done, total);
        }
        if (!zip.addFile(QString(ManifestFileName), serializeManifest(manifest)) || !zip.close())
            return failed(QString("can't finish %1").arg(part));
    }

    // 3. Re-read the zip and check every entry.
    QHash<QString, const FileEntry*> expected;
    for (const FileEntry& entry : manifest.files)
        expected.insert(entry.path, &entry);
    QSet<QString> seen;
    bool sawManifest = false;
    QString problem;
    done = 0;
    MMCZip::ArchiveReader reader(part);
    const bool parsed = reader.parse([&](MMCZip::ArchiveReader::File* f, bool& stop) {
        if (cancelled) {
            stop = true;
            return true;
        }
        const QString path = f->filename();
        if (f->isDirectory())
            return f->skip();
        if (path == ManifestFileName) {
            sawManifest = true;
            return f->skip();
        }
        const FileEntry* entry = expected.value(path, nullptr);
        if (!entry || seen.contains(path)) {
            problem = QString("%1 is unexpected in the zip").arg(path);
            return false;
        }
        seen.insert(path);
        CountingSink sink;
        QCryptographicHash sha1(QCryptographicHash::Sha1);
        if (!f->writeTo(sink, &sha1)) {
            problem = QString("%1 can't be read back").arg(path);
            return false;
        }
        if (sink.written != entry->size || QString::fromLatin1(sha1.result().toHex()) != entry->sha1) {
            problem = QString("%1 was written wrongly").arg(path);
            return false;
        }
        report(tr("Verifying the bundle"), ++done, total);
        return true;
    });
    if (cancelled)
        return stopped();
    if (!parsed || !problem.isEmpty() || !sawManifest || seen.size() != expected.size())
        return failed(problem.isEmpty() ? QString("verification found %1 of %2 files").arg(seen.size()).arg(expected.size()) : problem);

    // 4. Replace the destination only now.
    if (QFile::exists(zipPath) && !QFile::remove(zipPath))
        return failed(QString("can't replace %1").arg(zipPath));
    if (!QFile::rename(part, zipPath))
        return failed(QString("can't rename %1").arg(part));
    return WriteResult{ true, false, {}, {}, QFileInfo(zipPath).size() };
}

}  // namespace OfflineBundle
