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

#include <QWidget>

class QComboBox;

// The launcher's accessibility options (text size, high contrast). Used by Settings → Accessibility and by the
// first-run welcome page.
class AccessibilityOptionsWidget : public QWidget {
    Q_OBJECT
   public:
    explicit AccessibilityOptionsWidget(QWidget* parent = nullptr);

    void load();  // from the settings
    void save();  // to the settings, and applied right away

   signals:
    void changed();

   private:
    QComboBox* m_textScale;
    QComboBox* m_contrast;
};
