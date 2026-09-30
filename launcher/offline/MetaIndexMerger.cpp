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

#include "MetaIndexMerger.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QSaveFile>

namespace OfflineBundle {
namespace {

QString sha256Hex(const QByteArray& data)
{
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}

bool readObject(const QString& path, QJsonObject& out, QString& error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        error = QString("can't read %1").arg(path);
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        error = QString("%1 is not valid JSON: %2").arg(path, parseError.errorString());
        return false;
    }
    out = doc.object();
    return true;
}

// Writes the bytes atomically and returns their sha256, or an empty string on failure.
QString writeBytes(const QString& path, const QByteArray& data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit())
        return {};
    return sha256Hex(data);
}

QString writeObject(const QString& path, const QJsonObject& obj)
{
    return writeBytes(path, QJsonDocument(obj).toJson(QJsonDocument::Indented));
}

}  // namespace

MetaMergeResult mergeMeta(const QString& stagedMeta, const QString& liveMeta)
{
    MetaMergeResult result;
    QString error;
    QMap<QString, QString> packageSha;
    QMap<QString, QString> packageName;

    const QStringList uids = QDir(stagedMeta).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString& uid : uids) {
        const QDir stagedDir(QDir(stagedMeta).filePath(uid));
        const QDir liveDir(QDir(liveMeta).filePath(uid));
        QJsonObject stagedIndex;
        if (!readObject(stagedDir.filePath("index.json"), stagedIndex, error)) {
            result.details = error;
            return result;
        }

        // 1. Version files, remembering the sha256 of what was written.
        QMap<QString, QString> versionSha;
        const QStringList versionFiles = stagedDir.entryList({ "*.json" }, QDir::Files, QDir::Name);
        for (const QString& fileName : versionFiles) {
            if (fileName == "index.json")
                continue;
            QFile in(stagedDir.filePath(fileName));
            if (!in.open(QIODevice::ReadOnly)) {
                result.details = QString("can't read %1").arg(in.fileName());
                return result;
            }
            const QString sha = writeBytes(liveDir.filePath(fileName), in.readAll());
            if (sha.isEmpty()) {
                result.details = QString("can't write %1").arg(liveDir.filePath(fileName));
                return result;
            }
            versionSha.insert(fileName.chopped(5), sha);  // strip ".json"
        }

        // 2. Package index: keep every live entry; the bundle adds or replaces entries for the versions it ships.
        QJsonObject liveIndex;
        const QString liveIndexPath = liveDir.filePath("index.json");
        if (QFile::exists(liveIndexPath)) {
            if (!readObject(liveIndexPath, liveIndex, error)) {
                result.details = error;
                return result;
            }
        } else {
            liveIndex = stagedIndex;
            liveIndex["versions"] = QJsonArray();
        }
        QJsonArray versions = liveIndex.value("versions").toArray();
        for (const QJsonValue& value : stagedIndex.value("versions").toArray()) {
            QJsonObject entry = value.toObject();
            const QString version = entry.value("version").toString();
            if (!versionSha.contains(version))
                continue;
            entry["sha256"] = versionSha.value(version);
            bool replaced = false;
            for (int i = 0; i < versions.size(); ++i) {
                if (versions.at(i).toObject().value("version").toString() == version) {
                    versions[i] = entry;
                    replaced = true;
                    break;
                }
            }
            if (!replaced)
                versions.append(entry);
        }
        liveIndex["versions"] = versions;
        const QString sha = writeObject(liveIndexPath, liveIndex);
        if (sha.isEmpty()) {
            result.details = QString("can't write %1").arg(liveIndexPath);
            return result;
        }
        packageSha.insert(uid, sha);
        packageName.insert(uid, liveIndex.value("name").toString(stagedIndex.value("name").toString(uid)));
        result.changedUids.append(uid);
    }

    if (result.changedUids.isEmpty()) {
        result.ok = true;
        return result;
    }

    // 3. Root index: add or update the packages we touched.
    QJsonObject liveRoot;
    const QString liveRootPath = QDir(liveMeta).filePath("index.json");
    if (QFile::exists(liveRootPath)) {
        if (!readObject(liveRootPath, liveRoot, error)) {
            result.details = error;
            return result;
        }
    } else {
        liveRoot["formatVersion"] = 1;
    }
    QJsonArray packages = liveRoot.value("packages").toArray();
    for (const QString& uid : result.changedUids) {
        bool found = false;
        for (int i = 0; i < packages.size(); ++i) {
            QJsonObject package = packages.at(i).toObject();
            if (package.value("uid").toString() == uid) {
                package["sha256"] = packageSha.value(uid);
                packages[i] = package;
                found = true;
                break;
            }
        }
        if (!found)
            packages.append(QJsonObject{ { "name", packageName.value(uid) }, { "sha256", packageSha.value(uid) }, { "uid", uid } });
    }
    liveRoot["packages"] = packages;
    if (writeObject(liveRootPath, liveRoot).isEmpty()) {
        result.details = QString("can't write %1").arg(liveRootPath);
        return result;
    }
    result.ok = true;
    return result;
}

}  // namespace OfflineBundle
