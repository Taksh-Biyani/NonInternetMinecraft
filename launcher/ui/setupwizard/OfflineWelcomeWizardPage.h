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

#include "BaseWizardPage.h"

class QGroupBox;
class QLabel;
class AccessibilityOptionsWidget;

// First run, page 1 (spec §8.1): what the launcher is, how bundles travel, and the accessibility options right away.
class OfflineWelcomeWizardPage : public BaseWizardPage {
    Q_OBJECT
   public:
    explicit OfflineWelcomeWizardPage(QWidget* parent = nullptr);
    bool validatePage() override;
    void retranslate() override;

   private:
    QLabel* m_text;
    QGroupBox* m_group;
    AccessibilityOptionsWidget* m_options;
};
