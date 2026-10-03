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

#include "OfflineLaunchCheck.h"

#include <QDir>

#include "minecraft/PackProfile.h"

OfflineLaunchCheck::OfflineLaunchCheck(LaunchTask* parent, MinecraftInstance* instance, Phase phase)
    : LaunchStep(parent), m_instance(instance), m_phase(phase)
{}

void OfflineLaunchCheck::executeTask()
{
    if (m_phase == Phase::Components)
        checkComponents();
    else
        checkFiles();
}

void OfflineLaunchCheck::checkComponents()
{
    // A new launch: forget anything an earlier launch of this instance left behind.
    OfflineBundle::takeLaunchReport(m_instance->id());

    const QDir root(m_instance->instanceRoot());
    const auto components = OfflineBundle::readPackComponents(root.absoluteFilePath("mmc-pack.json"));
    OfflineBundle::MissingReport report;
    report.components = OfflineBundle::missingComponentMeta(QDir("meta").absolutePath(), root.absoluteFilePath("patches"), components);
    for (const auto& component : components) {
        if (component.uid == "net.minecraft")
            report.minecraftVersion = component.version;
    }
    finish(report);
}

void OfflineLaunchCheck::checkFiles()
{
    // AutoInstallJava may already have recorded a missing Java for this launch.
    OfflineBundle::MissingReport report = OfflineBundle::takeLaunchReport(m_instance->id()).value_or(OfflineBundle::MissingReport{});
    const auto profile = m_instance->getPackProfile()->getProfile();
    const auto context = m_instance->runtimeContext();
    const QString localLibraries = m_instance->getLocalLibraryPath();
    report.minecraftVersion = profile->getMinecraftVersion();

    QStringList files, natives;
    profile->getLibraryFiles(context, files, natives, localLibraries, m_instance->binRoot(), false);
    files += natives;
    // ForgeWrapper reads the installer jars (maven files) at launch; agents are loaded by the JVM.
    QList<LibraryPtr> extras = profile->getMavenFiles();
    for (const auto& agent : profile->getAgents())
        extras.append(agent.library);
    for (const auto& library : extras) {
        QStringList jars, nativeJars, natives32, natives64;
        library->getApplicableFiles(context, jars, nativeJars, natives32, natives64, localLibraries);
        files += jars + nativeJars + natives64;
    }
    QString gameJar;
    if (const auto mainJar = profile->getMainJar()) {
        QStringList jars, nativeJars, natives32, natives64;
        mainJar->getApplicableFiles(context, jars, nativeJars, natives32, natives64, localLibraries);
        gameJar = jars.value(0);
    }
    const auto assets = profile->getMinecraftAssets();
    OfflineBundle::checkLaunchFiles(report, files, gameJar, QDir("assets").absolutePath(), assets ? assets->id : QString());
    finish(report);
}

void OfflineLaunchCheck::finish(const OfflineBundle::MissingReport& report)
{
    if (report.isEmpty()) {
        emitSucceeded();
        return;
    }
    emit logLine(tr("This instance can't start offline because files are missing:"), MessageLevel::Error);
    emit logLine(OfflineBundle::missingDetails(report), MessageLevel::Error);
    OfflineBundle::storeLaunchReport(m_instance->id(), report);
    emitFailed(tr("Files this instance needs aren't on this computer. Import an offline bundle that contains them."));
}
