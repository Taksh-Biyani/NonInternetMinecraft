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

#include <QToolButton>

#include "offline/GuideLinks.h"

// A "?" button that opens one section of the bundled guide (spec §7.5).
class GuideButton : public QToolButton {
    Q_OBJECT
   public:
    GuideButton(OfflineGuide::Section section, QWidget* parent);

    // Opens Guide.html (next to the exe) at `section`; explains in a dialog when the file is missing.
    static void openSection(OfflineGuide::Section section, QWidget* parent);
};
