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

#include "OfflineName.h"

#include <QCoreApplication>
#include <QRegularExpression>

namespace OfflineName {

static QString tr(const char* text)
{
    return QCoreApplication::translate("OfflineName", text);
}

QString problem(const QString& name)
{
    static const QRegularExpression allowed("^[A-Za-z0-9_]*$");
    if (name.isEmpty())
        return tr("Type a name of 3 to 16 characters.");
    if (!allowed.match(name).hasMatch())
        return tr("Use only the letters A-Z, the digits 0-9 and _ (no spaces or other symbols).");
    if (name.size() < 3)
        return tr("The name is too short: use at least 3 characters.");
    if (name.size() > 16)
        return tr("The name is too long: use at most 16 characters.");
    return {};
}

}  // namespace OfflineName
