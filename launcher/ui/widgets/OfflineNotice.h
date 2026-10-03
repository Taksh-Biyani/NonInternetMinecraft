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

#include <QFrame>

// Shown at the top of pages that need the internet (modpack and mod browsers) while the launcher is offline (spec
// §4.3): why the list is empty, what to do instead, a Refresh button and a link into the guide.
class OfflineNotice : public QFrame {
    Q_OBJECT
   public:
    OfflineNotice(const QString& message, QWidget* parent);

    // False while offline: pages then wait for Refresh instead of loading (and failing) on their own.
    static bool autoLoadAllowed();

    static QString modpacksMessage();
    static QString resourcesMessage();

   signals:
    void refreshRequested();

   private:
    void updateVisibility();
};
