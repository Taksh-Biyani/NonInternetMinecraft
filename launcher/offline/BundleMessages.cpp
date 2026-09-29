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

#include "BundleMessages.h"

#include <QCoreApplication>

namespace OfflineBundle::Messages {

static QString tr(const char* text)
{
    return QCoreApplication::translate("OfflineBundle", text);
}

QString notABundle()
{
    return tr("This zip isn't an offline bundle. If it's a modpack or an instance export, use Add Instance → Import instead "
              "(that needs internet).");
}

QString damaged()
{
    return tr("This bundle is damaged or wasn't copied completely. Copy it to the USB stick again and retry.");
}

QString newerFormat()
{
    return tr("This bundle was made by a newer version of PineconeMC Offline. Update the launcher first "
              "(the update can come on the same USB stick).");
}

QString unsafe()
{
    return tr("This bundle tries to put files where a bundle isn't allowed to, so it wasn't imported. "
              "Make the bundle again with PineconeMC Offline.");
}

QString couldNotWrite()
{
    return tr("PineconeMC Offline couldn't write to its folder. Make sure the drive isn't full or read-only, then try again.");
}

QString notEnoughSpace(qint64 neededBytes, qint64 availableBytes)
{
    return tr("Not enough free space on this drive: the import needs %1, but only %2 is free. Free up some space and try again.")
        .arg(formatSize(neededBytes), formatSize(availableBytes));
}

QString formatSize(qint64 bytes)
{
    constexpr double KiB = 1024.0;
    constexpr double MiB = KiB * 1024.0;
    constexpr double GiB = MiB * 1024.0;
    const double value = static_cast<double>(bytes);
    if (value >= GiB)
        return QString("%1 GB").arg(value / GiB, 0, 'f', 1);
    if (value >= MiB)
        return QString("%1 MB").arg(qRound64(value / MiB));
    return QString("%1 KB").arg(qMax<qint64>(1, qRound64(value / KiB)));
}

}  // namespace OfflineBundle::Messages
