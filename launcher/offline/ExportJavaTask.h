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

#include <QList>
#include <QString>

#include "meta/Version.h"
#include "tasks/Task.h"

namespace OfflineBundle {

// Makes sure <javaRoot>/<javaName> holds the Mojang Java runtime a Minecraft version needs, downloading the Windows x64
// build if it isn't there yet (the same source AutoInstallJava uses).
class ExportJavaTask : public Task {
    Q_OBJECT
   public:
    ExportJavaTask(QString javaName, QList<int> majors, QString javaRoot);

    QString javaName() const { return m_javaName; }
    QString javaFolder() const;  // absolute
    bool abort() override;

   protected:
    void executeTask() override;

   private:
    void tryNextMajor();
    void download(Meta::Version::Ptr version);
    void follow(Task::Ptr task);
    bool installed() const;

    QString m_javaName;
    QList<int> m_majors;
    QString m_javaRoot;
    int m_next = 0;
    Task::Ptr m_current;
};

}  // namespace OfflineBundle
