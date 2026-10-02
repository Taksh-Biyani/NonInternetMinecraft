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

// Plain-language texts for offline bundles (spec §7). Each says what happened and what to do.
namespace OfflineBundle::Messages {
QString notABundle();
QString damaged();
QString newerFormat();
QString unsafe();
QString couldNotWrite();
QString notEnoughSpace(qint64 neededBytes, qint64 availableBytes);
QString downloadFailed(const QString& item);
QString exportNeedsInternet();
QString couldNotWriteBundle();

// "1 KB", "612 MB", "1.5 GB" (binary units, as Windows Explorer shows them).
QString formatSize(qint64 bytes);
}  // namespace OfflineBundle::Messages
