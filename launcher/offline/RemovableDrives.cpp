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

#include "RemovableDrives.h"

#include <QDir>
#include <QStorageInfo>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace OfflineBundle {

QString firstRemovableDriveRoot()
{
#ifdef Q_OS_WIN
    for (const QStorageInfo& volume : QStorageInfo::mountedVolumes()) {
        if (!volume.isValid() || !volume.isReady())
            continue;
        const std::wstring root = QDir::toNativeSeparators(volume.rootPath()).toStdWString();
        if (GetDriveTypeW(root.c_str()) == DRIVE_REMOVABLE)
            return volume.rootPath();
    }
#endif
    return {};
}

}  // namespace OfflineBundle
