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

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>
#include <functional>

#include "RuntimeContext.h"
#include "minecraft/Library.h"

namespace OfflineBundle {

// The files that go into a bundle: bundle path -> a source file on disk, or in-memory data (e.g. a cleaned-up
// instance.cfg). Bundle paths are unique ignoring case, as the importer requires.
class FileSet {
   public:
    struct Entry {
        QString bundlePath;
        QString sourcePath;  // empty when `data` is used
        QByteArray data;
    };

    // False (and recorded in conflicts()) if the path is already taken by a different source.
    bool addFile(const QString& bundlePath, const QString& sourcePath);
    bool addData(const QString& bundlePath, const QByteArray& data);
    void addMissing(const QString& sourcePath) { m_missing.append(sourcePath); }
    void addOutside(const QString& sourcePath) { m_outside.append(sourcePath); }

    const QList<Entry>& entries() const { return m_entries; }
    const QStringList& conflicts() const { return m_conflicts; }
    const QStringList& missing() const { return m_missing; }  // expected files that don't exist
    const QStringList& outside() const { return m_outside; }  // files not under the expected root (skipped)
    qint64 totalSize() const;

   private:
    QList<Entry> m_entries;
    QHash<QString, int> m_index;  // lower-case bundle path -> entry
    QStringList m_conflicts;
    QStringList m_missing;
    QStringList m_outside;
};

// Adds absoluteFile as <rootName>/<path relative to root>. Missing files go to missing(), files outside root to outside().
void addFileUnder(FileSet& files, const QString& absoluteFile, const QString& root, const QString& rootName);

// Adds every regular file under dir as <prefix>/<relative path>, if filter(relative path) allows it (no filter = all).
// Symbolic links are skipped. False if dir doesn't exist.
bool addTree(FileSet& files, const QString& dir, const QString& prefix, const std::function<bool(const QString&)>& filter = {});

// meta/index.json, meta/<uid>/index.json and meta/<uid>/<version>.json from metaRoot (the live meta/ folder).
void addMeta(FileSet& files, const QString& metaRoot, const QString& uid, const QString& version);

// assets/indexes/<indexId>.json and every object it lists. False if the index is missing or unreadable.
bool addAssets(FileSet& files, const QString& assetsRoot, const QString& indexId);

// The files of every library active in ctx: jars and natives (for "${arch}" natives only the 64-bit one).
// Files outside librariesRoot (instance-local libraries) are recorded in outside() and skipped.
void addLibraries(FileSet& files,
                  const QList<LibraryPtr>& libraries,
                  const RuntimeContext& ctx,
                  const QString& overridePath,
                  const QString& librariesRoot);

// Size and modification time (ms) of every file under root, keyed by absolute path.
using TreeSnapshot = QHash<QString, QPair<qint64, qint64>>;
TreeSnapshot snapshotTree(const QString& root);
// Absolute paths of files under root that are new or changed since `before`.
QStringList changedFiles(const QString& root, const TreeSnapshot& before);

}  // namespace OfflineBundle
