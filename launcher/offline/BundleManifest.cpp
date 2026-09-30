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

#include "BundleManifest.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>

#include "archive/ArchiveReader.h"
#include "offline/BundleMessages.h"
#include "offline/BundlePaths.h"

namespace OfflineBundle {

qint64 Manifest::totalSize() const
{
    qint64 total = 0;
    for (const FileEntry& file : files)
        total += file.size;
    return total;
}

static ReadResult failure(const QString& message, const QString& details)
{
    return ReadResult{ std::nullopt, message, details };
}

ReadResult parseManifest(const QByteArray& json)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
        return failure(Messages::damaged(), QString("%1 is not valid JSON: %2").arg(ManifestFileName, parseError.errorString()));
    const QJsonObject root = doc.object();

    Manifest m;
    const QJsonValue formatValue = root.value("formatVersion");
    if (!formatValue.isDouble())
        return failure(Messages::damaged(), "formatVersion is missing");
    m.formatVersion = formatValue.toInt();
    if (m.formatVersion > SupportedFormatVersion)
        return failure(Messages::newerFormat(),
                       QString("formatVersion %1; this launcher supports up to %2").arg(m.formatVersion).arg(SupportedFormatVersion));
    if (m.formatVersion < 1)
        return failure(Messages::damaged(), QString("invalid formatVersion %1").arg(m.formatVersion));

    const QString kind = root.value("kind").toString();
    if (kind == "versions")
        m.kind = Kind::Versions;
    else if (kind == "instance")
        m.kind = Kind::Instance;
    else
        return failure(Messages::damaged(), QString("unknown kind \"%1\"").arg(kind));

    m.name = root.value("name").toString();
    if (m.name.isEmpty())
        m.name = QCoreApplication::translate("OfflineBundle", "Offline bundle");
    m.createdAt = root.value("createdAt").toString();
    m.createdBy = root.value("createdBy").toString();

    const QJsonObject contents = root.value("contents").toObject();
    for (const QJsonValue& value : contents.value("components").toArray()) {
        const QJsonObject obj = value.toObject();
        ComponentEntry component{ obj.value("uid").toString(), obj.value("version").toString(), obj.value("name").toString() };
        if (component.uid.isEmpty() || component.version.isEmpty())
            return failure(Messages::damaged(), "a component entry has no uid or version");
        if (component.name.isEmpty())
            component.name = component.uid;
        m.components.append(component);
    }
    for (const QJsonValue& value : contents.value("java").toArray()) {
        const QJsonObject obj = value.toObject();
        JavaEntry java{ obj.value("name").toString(), obj.value("major").toInt(), obj.value("folder").toString() };
        if (java.name.isEmpty())
            java.name = java.folder;
        if (java.name.isEmpty())
            return failure(Messages::damaged(), "a java entry has no name");
        m.java.append(java);
    }
    if (const QJsonValue instanceValue = contents.value("instance"); instanceValue.isObject()) {
        const QJsonObject obj = instanceValue.toObject();
        InstanceEntry instance{ obj.value("name").toString(), obj.value("folder").toString(), obj.value("group").toString() };
        if (instance.folder != "instance")
            return failure(Messages::damaged(), "contents.instance.folder must be \"instance\"");
        if (instance.name.isEmpty())
            instance.name = m.name;
        m.instance = instance;
    }
    if (m.kind == Kind::Instance && !m.instance)
        return failure(Messages::damaged(), "an instance bundle needs contents.instance");
    if (m.kind == Kind::Versions && m.instance)
        return failure(Messages::damaged(), "a versions bundle can't contain an instance");

    static const QRegularExpression s_sha1("^[0-9a-f]{40}$");
    const QJsonValue filesValue = root.value("files");
    if (!filesValue.isArray())
        return failure(Messages::damaged(), "the files list is missing");
    QSet<QString> seen;
    QSet<QString> metaPackages;
    bool hasInstanceConfig = false;
    for (const QJsonValue& value : filesValue.toArray()) {
        const QJsonObject obj = value.toObject();
        FileEntry file{ normalizeEntryPath(obj.value("path").toString()), obj.value("sha1").toString().toLower(),
                        static_cast<qint64>(obj.value("size").toDouble(-1)) };
        if (const QString reason = checkEntryPath(file.path); !reason.isEmpty())
            return failure(Messages::unsafe(), QString("%1: %2").arg(file.path, reason));
        if (file.path == ManifestFileName)
            return failure(Messages::damaged(), "the manifest lists itself");
        if (!s_sha1.match(file.sha1).hasMatch() || file.size < 0)
            return failure(Messages::damaged(), QString("bad sha1 or size for %1").arg(file.path));
        if (seen.contains(file.path.toLower()))
            return failure(Messages::damaged(), QString("%1 is listed twice").arg(file.path));
        if (m.kind == Kind::Versions && file.path.startsWith("instance/"))
            return failure(Messages::damaged(), QString("a versions bundle can't contain %1").arg(file.path));
        seen.insert(file.path.toLower());
        hasInstanceConfig = hasInstanceConfig || file.path == "instance/instance.cfg";
        if (const QStringList segments = file.path.split('/'); segments.size() >= 3 && segments.first() == "meta")
            metaPackages.insert(segments.at(1));
        m.files.append(file);
    }
    if (m.kind == Kind::Instance && !hasInstanceConfig)
        return failure(Messages::damaged(), "instance/instance.cfg is missing");
    // Every metadata package folder needs its index.json, or its versions can't be installed (spec §3.2 step 5).
    for (const QString& uid : metaPackages) {
        if (!seen.contains(QString("meta/%1/index.json").arg(uid).toLower()))
            return failure(Messages::damaged(), QString("meta/%1/ has no index.json").arg(uid));
    }
    return ReadResult{ m, {}, {} };
}

ReadResult readManifestFromZip(const QString& zipPath)
{
    MMCZip::ArchiveReader zip(zipPath);
    auto file = zip.goToFile(ManifestFileName);
    if (!file)
        return failure(Messages::notABundle(), QString("%1 has no %2 at its top level").arg(QFileInfo(zipPath).fileName(), ManifestFileName));
    return parseManifest(file->readAll());
}

bool isBundle(const QString& path)
{
    if (!path.endsWith(".zip", Qt::CaseInsensitive) || !QFileInfo(path).isFile())
        return false;
    MMCZip::ArchiveReader zip(path);
    return zip.goToFile(ManifestFileName) != nullptr;
}

}  // namespace OfflineBundle
