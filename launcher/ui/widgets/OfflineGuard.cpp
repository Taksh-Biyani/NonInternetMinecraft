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

#include "OfflineGuard.h"

#include <QAbstractButton>
#include <QCoreApplication>
#include <QVariant>

#include "Application.h"
#include "offline/OfflineMode.h"

namespace OfflineGuard {

static const char* const OnlineToolTip = "pineconeOnlineToolTip";

static void update(QAbstractButton* button, bool offline)
{
    if (offline) {
        if (!button->property(OnlineToolTip).isValid())
            button->setProperty(OnlineToolTip, button->toolTip());
        button->setEnabled(false);
        button->setToolTip(QCoreApplication::translate(
            "OfflineGuard", "You're offline, so this list can't be refreshed. Import an offline bundle to get new versions."));
        return;
    }
    button->setEnabled(true);
    if (button->property(OnlineToolTip).isValid()) {
        button->setToolTip(button->property(OnlineToolTip).toString());
        button->setProperty(OnlineToolTip, QVariant());
    }
}

void disableWhileOffline(QAbstractButton* button)
{
    auto* mode = APPLICATION->offlineMode();
    if (!button || !mode)
        return;
    QObject::connect(mode, &OfflineMode::offlineChanged, button, [button](bool offline) { update(button, offline); });
    update(button, mode->isOffline());
}

}  // namespace OfflineGuard
