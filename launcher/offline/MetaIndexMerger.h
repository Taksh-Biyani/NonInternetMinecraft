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

#include <QString>
#include <QStringList>

namespace OfflineBundle {
struct MetaMergeResult {
    bool ok = false;
    QString details;          // technical reason when !ok
    bool badBundle = false;   // the bundle's own metadata is unusable (as opposed to a read/write problem on this PC)
    QStringList changedUids;  // packages whose index.json was written
};

// Copies the version files from stagedMeta (<staging>/meta) into liveMeta (<data>/meta) and merges the index files.
// Versions are only ever added or updated, never removed. A bundle's index entries are used only for versions whose
// file the bundle ships, and every sha256 is recomputed from the bytes actually written (spec §3.2 step 5).
MetaMergeResult mergeMeta(const QString& stagedMeta, const QString& liveMeta);
}  // namespace OfflineBundle
