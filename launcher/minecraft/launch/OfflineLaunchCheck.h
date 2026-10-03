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

#include "launch/LaunchStep.h"
#include "minecraft/MinecraftInstance.h"
#include "offline/LaunchCompleteness.h"

// Offline launches only (spec §4.4).
//  - Components runs before the metadata is loaded: is every component's version file installed?
//  - Files runs right after AutoInstallJava: libraries, natives, the game jar, assets, and the Java AutoInstallJava
//    couldn't find.
// Anything missing is stored with OfflineBundle::storeLaunchReport and the step fails; LaunchController then shows one
// plain-language dialog.
class OfflineLaunchCheck : public LaunchStep {
    Q_OBJECT
   public:
    enum class Phase { Components, Files };

    OfflineLaunchCheck(LaunchTask* parent, MinecraftInstance* instance, Phase phase);
    ~OfflineLaunchCheck() override = default;

    void executeTask() override;
    bool canAbort() const override { return false; }

   private:
    void checkComponents();
    void checkFiles();
    void finish(const OfflineBundle::MissingReport& report);

    MinecraftInstance* m_instance;
    Phase m_phase;
};
