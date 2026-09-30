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

#include "BundlePaths.h"

#include <QRegularExpression>

namespace OfflineBundle {

const QStringList& allowedRoots()
{
    static const QStringList s_roots = { "meta", "libraries", "assets", "java", "instance" };
    return s_roots;
}

QString normalizeEntryPath(const QString& path)
{
    QString result = path;
    result.replace('\\', '/');
    if (result.endsWith('/'))
        result.chop(1);
    return result;
}

QString checkEntryPath(const QString& path)
{
    static const QRegularExpression s_driveLetter("^[A-Za-z]:");
    static const QRegularExpression s_reservedName("^(con|prn|aux|nul|com[0-9]|lpt[0-9])(\\..*)?$",
                                                   QRegularExpression::CaseInsensitiveOption);
    if (path.isEmpty())
        return "empty path";
    if (path.startsWith('/'))
        return "absolute path";
    if (s_driveLetter.match(path).hasMatch())
        return "drive letter in path";
    for (const QChar c : path) {
        if (c.unicode() < 0x20 || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
            return "character not allowed in Windows paths";
    }
    const QStringList segments = path.split('/');
    for (const QString& segment : segments) {
        if (segment.isEmpty())
            return "empty path segment";
        if (segment == "." || segment == "..")
            return "relative path segment";
        if (segment.endsWith('.') || segment.endsWith(' '))
            return "path segment ends with a dot or space";
        if (s_reservedName.match(segment).hasMatch())
            return "reserved Windows file name";
    }
    if (path == ManifestFileName)
        return {};
    if (!allowedRoots().contains(segments.first()))
        return "top-level entry not allowed in a bundle";
    return {};
}

}  // namespace OfflineBundle
