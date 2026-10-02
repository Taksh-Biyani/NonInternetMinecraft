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
#include <QList>
#include <QString>
#include <optional>

#include "RuntimeContext.h"
#include "offline/BundleManifest.h"

// Pure rules for building offline bundles (spec §5).
namespace OfflineBundle {

// One Minecraft version plus an optional mod loader: one throwaway instance during export.
struct ExportSet {
    QString minecraft;
    QString loaderUid;      // empty = vanilla
    QString loaderVersion;  // set when loaderUid is set
};

// "26.3" or "26.3,net.neoforged=26.3.0.33-beta" (spaces allowed). Used by the --export-bundle command line.
std::optional<ExportSet> parseExportSet(const QString& text);

// relPath is relative to the instance folder, '/'-separated. Leaves out logs, crash reports, the extracted natives,
// caches and other hidden folders (except the mods' .index metadata and a .minecraft game folder), and saves unless
// includeWorlds (spec §5.3.4).
bool shouldExportInstanceFile(const QString& relPath, bool includeWorlds);

// When the launcher picks the instance's Java itself (AutomaticJava=true), drops JavaPath, OverrideJavaLocation and
// AutomaticJava, so the offline PC picks its own Java. Line endings are kept.
QByteArray sanitizeInstanceCfg(const QByteArray& cfg);

// Major version from a Java runtime's "release" file (JAVA_VERSION="25.0.1" -> 25, "1.8.0_392" -> 8), 0 if unknown.
int javaMajorFromRelease(const QByteArray& releaseFile);

// "Forge", "NeoForge", "Fabric Loader", "Quilt Loader", "LiteLoader" for loader uids, otherwise empty.
QString loaderDisplayName(const QString& uid);

// "MC-26.3-NeoForge-26.3.0.33-beta.zip" from the components (Minecraft and loaders only, duplicates skipped).
QString suggestedFileName(const QList<ComponentEntry>& components);

// "<instance name>-offline.zip", with characters Windows forbids in file names replaced by '_'.
QString suggestedInstanceFileName(const QString& instanceName);

// Store already-compressed files (jar, zip, png, ogg and asset objects) instead of deflating them.
bool storeUncompressed(const QString& bundlePath);

// The runtime context every export uses, whatever PC it runs on: Windows, 64-bit Java (spec §5.3.2).
RuntimeContext windowsX64Context();

}  // namespace OfflineBundle
