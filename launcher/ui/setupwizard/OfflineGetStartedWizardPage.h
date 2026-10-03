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

class QCommandLinkButton;
class QLabel;

// First run, last page (spec §8.1): import a bundle, create an instance, or open the guide.
class OfflineGetStartedWizardPage : public BaseWizardPage {
    Q_OBJECT
   public:
    enum class Action { None, ImportBundle, CreateInstance };

    explicit OfflineGetStartedWizardPage(QWidget* parent = nullptr);
    Action chosenAction() const { return m_action; }
    void initializePage() override;
    void retranslate() override;

   private:
    void choose(Action action);
    static bool hasLocalMinecraftVersion();

    QLabel* m_text;
    QCommandLinkButton* m_import;
    QCommandLinkButton* m_create;
    QCommandLinkButton* m_guide;
    Action m_action = Action::None;
};
