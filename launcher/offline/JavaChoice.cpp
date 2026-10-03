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

#include "JavaChoice.h"

#include <QDir>

namespace JavaChoice {
int pick(const QList<Candidate>& javas, const QList<int>& majors, const QString& ownJavaDir)
{
    const QString own = QDir::cleanPath(QDir::fromNativeSeparators(ownJavaDir)) + '/';
    auto inOwnDir = [&own](const QString& path) {
        return QDir::cleanPath(QDir::fromNativeSeparators(path)).startsWith(own, Qt::CaseInsensitive);
    };
    int fallback = -1;
    for (int i = 0; i < javas.size(); i++) {
        if (!majors.contains(javas.at(i).major))
            continue;
        if (inOwnDir(javas.at(i).path))
            return i;
        if (fallback < 0)
            fallback = i;
    }
    return fallback;
}
}  // namespace JavaChoice
