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

#include "OfflineRefreshButton.h"

#include "Application.h"
#include "offline/OfflineMode.h"

OfflineRefreshButton::OfflineRefreshButton(QWidget* parent) : QPushButton(parent)
{
    setText(tr("Refresh"));
    setToolTip(tr("The launcher is offline, so nothing was loaded from the internet. "
                  "If this computer is connected now, click to load the list."));
    if (auto* offline = APPLICATION->offlineMode())
        connect(offline, &OfflineMode::offlineChanged, this, &OfflineRefreshButton::updateVisibility);
    updateVisibility();
}

bool OfflineRefreshButton::autoLoadAllowed()
{
    return !OfflineMode::globallyOffline();
}

void OfflineRefreshButton::updateVisibility()
{
    setVisible(!autoLoadAllowed());
}
