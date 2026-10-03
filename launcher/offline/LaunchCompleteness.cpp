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

#include "LaunchCompleteness.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

namespace OfflineBundle {

static QString tr(const char* text)
{
    return QCoreApplication::translate("OfflineBundle", text);
}

bool MissingReport::isEmpty() const
{
    return components.isEmpty() && gameJar.isEmpty() && libraryFiles.isEmpty() && assetIndex.isEmpty() && assetObjects.isEmpty() &&
           javaMajors.isEmpty();
}

QList<PackComponent> readPackComponents(const QString& mmcPackPath)
{
    QFile file(mmcPackPath);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QJsonArray array = QJsonDocument::fromJson(file.readAll()).object().value("components").toArray();
    QList<PackComponent> result;
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        const QString uid = object.value("uid").toString();
        const QString version = object.value("version").toString();
        if (uid.isEmpty() || version.isEmpty() || object.value("disabled").toBool())
            continue;
        const QString name = object.value("cachedName").toString();
        result.append({ uid, version, name.isEmpty() ? uid : name });
    }
    return result;
}

QStringList missingComponentMeta(const QString& metaRoot, const QString& patchesDir, const QList<PackComponent>& components)
{
    QStringList missing;
    const QDir meta(metaRoot);
    const QDir patches(patchesDir);
    for (const auto& component : components) {
        if (QFileInfo::exists(patches.absoluteFilePath(component.uid + ".json")))
            continue;
        if (!QFileInfo::exists(meta.absoluteFilePath(component.uid + '/' + component.version + ".json")))
            missing.append(component.name + ' ' + component.version);
    }
    return missing;
}

void checkLaunchFiles(MissingReport& report,
                      const QStringList& libraryFiles,
                      const QString& gameJar,
                      const QString& assetsRoot,
                      const QString& assetIndexId)
{
    if (!gameJar.isEmpty() && !QFileInfo::exists(gameJar))
        report.gameJar = gameJar;
    QSet<QString> seen(report.libraryFiles.begin(), report.libraryFiles.end());
    for (const QString& file : libraryFiles) {
        if (file == gameJar || seen.contains(file) || QFileInfo::exists(file))
            continue;
        seen.insert(file);
        report.libraryFiles.append(file);
    }

    if (assetIndexId.isEmpty())
        return;
    const QDir assets(assetsRoot);
    const QString indexPath = assets.absoluteFilePath("indexes/" + assetIndexId + ".json");
    QFile indexFile(indexPath);
    const QJsonDocument doc = indexFile.open(QIODevice::ReadOnly) ? QJsonDocument::fromJson(indexFile.readAll()) : QJsonDocument();
    if (!doc.isObject()) {
        report.assetIndex = indexPath;
        return;
    }
    QSet<QString> seenObjects(report.assetObjects.begin(), report.assetObjects.end());
    const QJsonObject objects = doc.object().value("objects").toObject();
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        const QString hash = it.value().toObject().value("hash").toString();
        if (hash.size() < 2)
            continue;
        const QString path = assets.absoluteFilePath("objects/" + hash.left(2) + '/' + hash);
        if (seenObjects.contains(path) || QFileInfo::exists(path))
            continue;
        seenObjects.insert(path);
        report.assetObjects.append(path);
    }
}

static QString javaText(const QList<int>& majors)
{
    QStringList numbers;
    for (int major : majors)
        numbers.append(QString::number(major));
    return tr("Java %1").arg(numbers.join(tr(" or ")));
}

QStringList describeMissing(const MissingReport& report)
{
    QStringList lines = report.components;
    const QString minecraft = report.minecraftVersion.isEmpty() ? tr("Minecraft") : tr("Minecraft %1").arg(report.minecraftVersion);
    if (!report.gameJar.isEmpty())
        lines.append(tr("The %1 game file").arg(minecraft));
    if (report.libraryFiles.size() == 1)
        lines.append(tr("1 game library file"));
    else if (report.libraryFiles.size() > 1)
        lines.append(tr("%1 game library files").arg(report.libraryFiles.size()));
    if (!report.assetIndex.isEmpty() || !report.assetObjects.isEmpty())
        lines.append(tr("%1 sounds and textures").arg(minecraft));
    if (!report.javaMajors.isEmpty())
        lines.append(javaText(report.javaMajors));
    return lines;
}

QString missingMessage(const MissingReport& report, const QString& instanceName)
{
    QString items;
    for (const QString& line : describeMissing(report))
        items += "<li>" + line.toHtmlEscaped() + "</li>";
    return tr("<p><b>%1</b> can't start because this computer is missing:</p><ul>%2</ul>"
              "<p>Import an offline bundle that contains them. To make one, use <b>Export Bundle</b> in this launcher on a "
              "computer with internet, then bring the file over on a USB stick.</p>")
        .arg(instanceName.toHtmlEscaped(), items);
}

QString missingDetails(const MissingReport& report)
{
    QStringList lines;
    auto section = [&lines](const QString& heading, const QStringList& entries) {
        if (entries.isEmpty())
            return;
        lines.append(heading);
        for (const QString& entry : entries)
            lines.append("  " + entry);
    };
    section(tr("Version information:"), report.components);
    section(tr("Game file:"), report.gameJar.isEmpty() ? QStringList() : QStringList{ report.gameJar });
    section(tr("Libraries:"), report.libraryFiles);
    section(tr("Asset index:"), report.assetIndex.isEmpty() ? QStringList() : QStringList{ report.assetIndex });
    // A missing asset folder means thousands of files: list a few and count the rest.
    QStringList objects = report.assetObjects.mid(0, 20);
    if (report.assetObjects.size() > 20)
        objects.append(tr("... and %1 more").arg(report.assetObjects.size() - 20));
    section(tr("Sound and texture files:"), objects);
    section(tr("Java:"), report.javaMajors.isEmpty() ? QStringList() : QStringList{ javaText(report.javaMajors) });
    return lines.join('\n');
}

static QHash<QString, MissingReport>& reports()
{
    static QHash<QString, MissingReport> store;
    return store;
}

void storeLaunchReport(const QString& instanceId, const MissingReport& report)
{
    reports().insert(instanceId, report);
}

std::optional<MissingReport> takeLaunchReport(const QString& instanceId)
{
    auto it = reports().find(instanceId);
    if (it == reports().end())
        return std::nullopt;
    MissingReport report = it.value();
    reports().erase(it);
    return report;
}

}  // namespace OfflineBundle
