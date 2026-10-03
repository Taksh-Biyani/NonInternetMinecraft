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

#include "ExportRules.h"

#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

namespace OfflineBundle {

std::optional<ExportSet> parseExportSet(const QString& text)
{
    const QStringList parts = text.split(',');
    ExportSet set;
    set.minecraft = parts.value(0).trimmed();
    if (set.minecraft.isEmpty() || parts.size() > 2)
        return std::nullopt;
    if (parts.size() == 2) {
        const QStringList loader = parts.at(1).split('=');
        if (loader.size() != 2)
            return std::nullopt;
        set.loaderUid = loader.at(0).trimmed();
        set.loaderVersion = loader.at(1).trimmed();
        if (set.loaderUid.isEmpty() || set.loaderVersion.isEmpty())
            return std::nullopt;
    }
    return set;
}

bool shouldExportInstanceFile(const QString& relPath, bool includeWorlds)
{
    const QStringList segments = relPath.split('/', Qt::SkipEmptyParts);
    if (segments.isEmpty())
        return false;
    const bool inGameFolder = segments.first() == "minecraft" || segments.first() == ".minecraft";
    if (segments.first() == "natives")
        return false;
    // Every folder segment (not the file name) that starts with '.' is a cache or tool folder, except these two.
    for (int i = 0; i + 1 < segments.size(); ++i) {
        const QString& s = segments.at(i);
        if (s.startsWith('.') && !(i == 0 && s == ".minecraft") && s != ".index")
            return false;
    }
    if (inGameFolder && segments.size() > 2) {
        const QString& top = segments.at(1);
        if (top == "logs" || top == "crash-reports")
            return false;
        if (top == "saves" && !includeWorlds)
            return false;
    }
    return true;
}

QByteArray sanitizeInstanceCfg(const QByteArray& cfg)
{
    const QList<QByteArray> lines = cfg.split('\n');
    bool automatic = false;
    for (const QByteArray& line : lines) {
        if (line.trimmed() == "AutomaticJava=true")
            automatic = true;
    }
    if (!automatic)
        return cfg;
    static const QList<QByteArray> dropped{ "AutomaticJava=", "JavaPath=", "OverrideJavaLocation=" };
    QList<QByteArray> kept;
    for (const QByteArray& line : lines) {
        bool drop = false;
        for (const QByteArray& key : dropped)
            drop = drop || line.startsWith(key);
        if (!drop)
            kept.append(line);
    }
    return kept.join('\n');
}

int javaMajorFromRelease(const QByteArray& releaseFile)
{
    static const QRegularExpression s_version(R"(JAVA_VERSION="(\d+)(?:\.(\d+))?)");
    const auto match = s_version.match(QString::fromUtf8(releaseFile));
    if (!match.hasMatch())
        return 0;
    const int first = match.captured(1).toInt();
    return first == 1 ? match.captured(2).toInt() : first;
}

QString loaderDisplayName(const QString& uid)
{
    static const QHash<QString, QString> s_names{
        { "net.minecraftforge", "Forge" },
        { "net.neoforged", "NeoForge" },
        { "net.fabricmc.fabric-loader", "Fabric Loader" },
        { "org.quiltmc.quilt-loader", "Quilt Loader" },
        { "com.mumfrey.liteloader", "LiteLoader" },
    };
    return s_names.value(uid);
}

static QString shortName(const QString& uid)
{
    if (uid == "net.minecraft")
        return "MC";
    return loaderDisplayName(uid).remove(" Loader");
}

static QString safeFileName(QString name)
{
    static const QRegularExpression s_forbidden(R"([<>:"/\\|?*\x00-\x1f])");
    name.replace(s_forbidden, "_");
    return name.trimmed().left(120);
}

QString suggestedFileName(const QList<ComponentEntry>& components)
{
    QStringList parts;
    QSet<QString> seen;
    for (const ComponentEntry& c : components) {
        const QString shortUid = shortName(c.uid);
        const QString key = c.uid + '=' + c.version;
        if (shortUid.isEmpty() || seen.contains(key))
            continue;
        seen.insert(key);
        parts << shortUid << c.version;
    }
    if (parts.isEmpty())
        parts << "offline-bundle";
    return safeFileName(parts.join('-')) + ".zip";
}

QString suggestedInstanceFileName(const QString& instanceName)
{
    return safeFileName(instanceName + "-offline") + ".zip";
}

RuntimeContext windowsX64Context()
{
    RuntimeContext ctx;
    ctx.javaArchitecture = "64";
    ctx.javaRealArchitecture = "amd64";
    ctx.system = "windows";
    return ctx;
}

}  // namespace OfflineBundle
