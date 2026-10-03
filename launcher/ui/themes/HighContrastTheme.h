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

#include "FusionTheme.h"

// High-contrast colours (Settings → Accessibility). Dark: white on black with a yellow focus outline (like Windows'
// "High Contrast Black"); light: black on white with a blue focus outline.
class HighContrastTheme : public FusionTheme {
   public:
    explicit HighContrastTheme(bool dark) : m_dark(dark) {}
    ~HighContrastTheme() override = default;

    QString id() override;
    QString name() override;
    QString tooltip() override;
    bool hasStyleSheet() override { return true; }
    QString appStyleSheet() override;
    QPalette colorScheme() override;
    double fadeAmount() override { return 0.0; }
    QColor fadeColor() override;

   private:
    bool m_dark;
};
