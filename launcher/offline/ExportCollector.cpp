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

#include "ExportCollector.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

namespace OfflineBundle {

static QString cleanAbsolute(const QString& path)
{
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

bool FileSet::addFile(const QString& bundlePath, const QString& sourcePath)
{
    const QString key = bundlePath.toLower();
    const QString source = cleanAbsolute(sourcePath);
    if (const auto it = m_index.constFind(key); it != m_index.constEnd()) {
        if (m_entries.at(*it).sourcePath.compare(source, Qt::CaseInsensitive) == 0)
            return true;
        m_conflicts.append(bundlePath);
        return false;
    }
    m_index.insert(key, m_entries.size());
    m_entries.append({ bundlePath, source, {} });
    return true;
}

bool FileSet::addData(const QString& bundlePath, const QByteArray& data)
{
    const QString key = bundlePath.toLower();
    if (m_index.contains(key)) {
        m_conflicts.append(bundlePath);
        return false;
    }
    m_index.insert(key, m_entries.size());
    m_entries.append({ bundlePath, {}, data });
    return true;
}

qint64 FileSet::totalSize() const
{
    qint64 total = 0;
    for (const Entry& e : m_entries)
        total += e.sourcePath.isEmpty() ? e.data.size() : QFileInfo(e.sourcePath).size();
    return total;
}

void addFileUnder(FileSet& files, const QString& absoluteFile, const QString& root, const QString& rootName)
{
    const QString file = cleanAbsolute(absoluteFile);
    const QString rel = QDir(cleanAbsolute(root)).relativeFilePath(file);
    if (rel.startsWith("..") || QDir::isAbsolutePath(rel)) {
        files.addOutside(file);
        return;
    }
    if (!QFileInfo(file).isFile()) {
        files.addMissing(file);
        return;
    }
    files.addFile(rootName + '/' + rel, file);
}

bool addTree(FileSet& files, const QString& dir, const QString& prefix, const std::function<bool(const QString&)>& filter)
{
    if (!QFileInfo(dir).isDir())
        return false;
    const QDir base(dir);
    QDirIterator it(dir, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QFileInfo info = it.fileInfo();
        if (info.isSymLink())
            continue;
        const QString rel = base.relativeFilePath(info.absoluteFilePath());
        if (filter && !filter(rel))
            continue;
        files.addFile(prefix + '/' + rel, info.absoluteFilePath());
    }
    return true;
}

void addMeta(FileSet& files, const QString& metaRoot, const QString& uid, const QString& version)
{
    const QDir meta(metaRoot);
    addFileUnder(files, meta.absoluteFilePath("index.json"), metaRoot, "meta");
    addFileUnder(files, meta.absoluteFilePath(uid + "/index.json"), metaRoot, "meta");
    addFileUnder(files, meta.absoluteFilePath(uid + '/' + version + ".json"), metaRoot, "meta");
}

bool addAssets(FileSet& files, const QString& assetsRoot, const QString& indexId)
{
    const QDir assets(assetsRoot);
    const QString indexPath = assets.absoluteFilePath("indexes/" + indexId + ".json");
    QFile indexFile(indexPath);
    if (!indexFile.open(QIODevice::ReadOnly)) {
        files.addMissing(indexPath);
        return false;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(indexFile.readAll());
    if (!doc.isObject())
        return false;
    addFileUnder(files, indexPath, assetsRoot, "assets");
    const QJsonObject objects = doc.object().value("objects").toObject();
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        const QString hash = it.value().toObject().value("hash").toString();
        if (hash.size() < 2)
            continue;
        addFileUnder(files, assets.absoluteFilePath("objects/" + hash.left(2) + '/' + hash), assetsRoot, "assets");
    }
    return true;
}

void addLibraries(FileSet& files,
                  const QList<LibraryPtr>& libraries,
                  const RuntimeContext& ctx,
                  const QString& overridePath,
                  const QString& librariesRoot)
{
    for (const LibraryPtr& lib : libraries) {
        if (!lib || !lib->isActive(ctx))
            continue;
        QStringList jars, natives, natives32, natives64;
        lib->getApplicableFiles(ctx, jars, natives, natives32, natives64, overridePath);
        for (const QString& path : jars + natives + natives64)
            addFileUnder(files, path, librariesRoot, "libraries");
    }
}

TreeSnapshot snapshotTree(const QString& root)
{
    TreeSnapshot snapshot;
    QDirIterator it(root, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QFileInfo info = it.fileInfo();
        snapshot.insert(cleanAbsolute(info.absoluteFilePath()), { info.size(), info.lastModified().toMSecsSinceEpoch() });
    }
    return snapshot;
}

QStringList changedFiles(const QString& root, const TreeSnapshot& before)
{
    QStringList changed;
    const TreeSnapshot after = snapshotTree(root);
    for (auto it = after.begin(); it != after.end(); ++it) {
        const auto old = before.constFind(it.key());
        if (old == before.constEnd() || *old != it.value())
            changed.append(it.key());
    }
    return changed;
}

}  // namespace OfflineBundle
