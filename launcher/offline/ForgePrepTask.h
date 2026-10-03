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

#include <QProcess>
#include <QString>
#include <QStringList>

#include "offline/ExportCollector.h"
#include "tasks/Task.h"

namespace OfflineBundle {

// Runs pinecone-forge-prep.jar with the given Java, so the Forge/NeoForge installer processors write their outputs
// into librariesDir now (spec §5.3.3). createdFiles() lists every file under librariesDir that appeared or changed.
class ForgePrepTask : public Task {
    Q_OBJECT
   public:
    ForgePrepTask(QString javaExe, QString prepJar, QString forgeWrapperJar, QString installerJar, QString librariesDir, QString minecraftJar);

    QStringList createdFiles() const { return m_created; }
    QString output() const { return m_output; }
    bool abort() override;

   protected:
    void executeTask() override;

   private:
    QString m_javaExe, m_prepJar, m_forgeWrapperJar, m_installerJar, m_librariesDir, m_minecraftJar;
    TreeSnapshot m_before;
    QStringList m_created;
    QString m_output;
    QProcess m_process;
};

}  // namespace OfflineBundle
