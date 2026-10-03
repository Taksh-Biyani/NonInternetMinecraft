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

#include <QList>
#include <QString>
#include <QStringList>
#include <optional>

// What an offline launch needs that isn't on this computer (spec §4.4), and the plain-language texts for it.
namespace OfflineBundle {

struct PackComponent {
    QString uid;
    QString version;
    QString name;  // "Minecraft", "Fabric Loader" (cachedName); the uid when unknown
};

struct MissingReport {
    QString minecraftVersion;  // for the wording; may be empty
    QStringList components;    // "Fabric Loader 0.19.5": version information that isn't installed
    QString gameJar;           // the missing Minecraft client jar; empty when it's there
    QStringList libraryFiles;  // other missing jars and natives
    QString assetIndex;        // the missing asset index; empty when it's there
    QStringList assetObjects;  // missing sound and texture files (when the index is there)
    QList<int> javaMajors;     // not empty: no installed Java can run this, one of these is needed

    bool isEmpty() const;
};

// The enabled components of an instance's mmc-pack.json. Empty if the file can't be read.
QList<PackComponent> readPackComponents(const QString& mmcPackPath);

// "Name version" for each component whose metadata (metaRoot/<uid>/<version>.json) is missing. A component with a
// local patch (patchesDir/<uid>.json) needs no metadata and is skipped.
QStringList missingComponentMeta(const QString& metaRoot, const QString& patchesDir, const QList<PackComponent>& components);

// Adds the missing files to `report`. libraryFiles may contain gameJar; it's reported once, as the game file.
void checkLaunchFiles(MissingReport& report,
                      const QStringList& libraryFiles,
                      const QString& gameJar,
                      const QString& assetsRoot,
                      const QString& assetIndexId);

QStringList describeMissing(const MissingReport& report);                          // one plain line per kind
QString missingMessage(const MissingReport& report, const QString& instanceName);  // HTML for the dialog
QString missingDetails(const MissingReport& report);                               // plain text, every missing file

// The launch steps leave what's missing here (keyed by instance id); LaunchController shows it when the launch fails.
void storeLaunchReport(const QString& instanceId, const MissingReport& report);
std::optional<MissingReport> takeLaunchReport(const QString& instanceId);

}  // namespace OfflineBundle
