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

namespace JavaChoice {
struct Candidate {
    QString path;  // the java(w) executable
    int major;
};
// The index of the Java to use: one whose major version is in `majors`. Javas inside `ownJavaDir` (the launcher's
// java folder, where the shipped runtimes and imported bundles put theirs) come first, so a Java installed on Windows
// is only used when the launcher has none of its own. -1 if nothing fits.
int pick(const QList<Candidate>& javas, const QList<int>& majors, const QString& ownJavaDir);
}  // namespace JavaChoice
