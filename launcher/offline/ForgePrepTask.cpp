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

#include "ForgePrepTask.h"

#include <QFileInfo>

namespace OfflineBundle {

ForgePrepTask::ForgePrepTask(QString javaExe, QString prepJar, QString forgeWrapperJar, QString installerJar, QString librariesDir, QString minecraftJar)
    : m_javaExe(std::move(javaExe))
    , m_prepJar(std::move(prepJar))
    , m_forgeWrapperJar(std::move(forgeWrapperJar))
    , m_installerJar(std::move(installerJar))
    , m_librariesDir(std::move(librariesDir))
    , m_minecraftJar(std::move(minecraftJar))
{}

void ForgePrepTask::executeTask()
{
    setStatus(tr("Running the Forge installer steps..."));
    m_before = snapshotTree(m_librariesDir);
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    m_process.setWorkingDirectory(QFileInfo(m_librariesDir).absolutePath());
    connect(&m_process, &QProcess::readyRead, this, [this] {
        const QString chunk = QString::fromLocal8Bit(m_process.readAll());
        m_output += chunk;
        const QString last = chunk.trimmed().section('\n', -1).trimmed();
        if (!last.isEmpty())
            setDetails(last);
    });
    connect(&m_process, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status) {
        m_output += QString::fromLocal8Bit(m_process.readAll());
        if (status == QProcess::NormalExit && exitCode == 0 && m_output.contains("FORGE_PREP_OK")) {
            m_created = changedFiles(m_librariesDir, m_before);
            emitSucceeded();
        } else {
            emitFailed(tr("The Forge installer steps failed (exit code %1).").arg(exitCode));
        }
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            emitFailed(tr("Couldn't start Java: %1").arg(m_javaExe));
    });
    m_process.start(m_javaExe, { "-jar", m_prepJar, m_forgeWrapperJar, m_installerJar, m_librariesDir, m_minecraftJar });
}

bool ForgePrepTask::abort()
{
    m_process.kill();
    emitAborted();
    return true;
}

}  // namespace OfflineBundle
