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
#include <atomic>
#include <functional>

#include "offline/BundleManifest.h"
#include "offline/ExportCollector.h"

namespace OfflineBundle {

struct WriteResult {
    bool ok = false;
    bool cancelled = false;
    QString message;  // plain language, for the user
    QString details;  // technical
    qint64 zipSize = 0;
};

// step is a translated label ("Checking files", "Writing the bundle", "Verifying the bundle").
using WriteProgress = std::function<void(const QString& step, qint64 done, qint64 total)>;

// Hashes every file (filling manifest.files), writes them and then the manifest into "<zipPath>.part", re-reads that
// zip checking every entry's SHA-1 and size, and only then renames it to zipPath (spec §5.3.5). On failure or cancel
// the partial file is deleted and an existing zipPath is left as it was. Safe to call from a worker thread.
WriteResult writeBundle(const QString& zipPath,
                        Manifest manifest,
                        const FileSet& files,
                        const WriteProgress& progress,
                        const std::atomic_bool& cancelled);

}  // namespace OfflineBundle
