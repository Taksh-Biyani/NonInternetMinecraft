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

#include "BundleSummary.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace OfflineBundle {

QList<SummaryLine> summarize(const Manifest& manifest, const QString& dataRoot, const QString& javaDir)
{
    QList<SummaryLine> lines;
    for (const ComponentEntry& component : manifest.components) {
        const QString file = QDir(dataRoot).filePath(QString("meta/%1/%2.json").arg(component.uid, component.version));
        lines.append({ QString("%1 %2").arg(component.name, component.version), QFileInfo::exists(file) });
    }
    for (const JavaEntry& java : manifest.java) {
        const bool installed = !java.folder.isEmpty() && QFileInfo(QDir(javaDir).filePath(java.folder)).isDir();
        lines.append({ java.name, installed });
    }
    if (manifest.instance)
        lines.append({ QCoreApplication::translate("OfflineBundle", "Instance: %1").arg(manifest.instance->name), false });
    return lines;
}

}  // namespace OfflineBundle
