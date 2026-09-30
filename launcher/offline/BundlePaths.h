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
inline constexpr auto ManifestFileName = "pinecone-offline-bundle.json";

// The top-level folders a bundle may contain, besides the manifest file (spec §2.1).
const QStringList& allowedRoots();

// Zip entry paths may use '\' (older Windows zip tools): converts to '/' and drops one trailing '/'.
QString normalizeEntryPath(const QString& path);

// Empty if a normalized entry path is safe to extract into the staging folder, otherwise a short technical reason.
QString checkEntryPath(const QString& path);
}  // namespace OfflineBundle
