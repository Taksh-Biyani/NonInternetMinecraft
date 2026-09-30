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

#include "offline/BundleManifest.h"

namespace OfflineBundle {
struct SummaryLine {
    QString text;
    bool alreadyInstalled = false;
};

// One line per component, Java runtime and instance, for the import preview (spec §3.2 step 2).
QList<SummaryLine> summarize(const Manifest& manifest, const QString& dataRoot, const QString& javaDir);
}  // namespace OfflineBundle
