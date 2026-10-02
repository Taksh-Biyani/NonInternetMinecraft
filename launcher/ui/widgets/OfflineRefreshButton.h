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

#include <QPushButton>

// A "Refresh" button for pages that load from the internet (Modrinth, CurseForge). While the launcher is offline those
// pages don't load anything by themselves, so the user gets no error pop-ups; this button is shown instead and loads
// once when clicked. It hides itself while the launcher is online.
class OfflineRefreshButton : public QPushButton {
    Q_OBJECT
   public:
    explicit OfflineRefreshButton(QWidget* parent);

    // False while the launcher is offline: pages must then wait for a click instead of loading on their own.
    static bool autoLoadAllowed();

   private:
    void updateVisibility();
};
