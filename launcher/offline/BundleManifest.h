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

#include <QByteArray>
#include <QList>
#include <QString>
#include <optional>

// pinecone-offline-bundle.json (spec §2.2).
namespace OfflineBundle {
inline constexpr int SupportedFormatVersion = 1;

enum class Kind { Versions, Instance };

struct ComponentEntry {
    QString uid;
    QString version;
    QString name;
};

struct JavaEntry {
    QString name;
    int major = 0;
    QString folder;  // the runtime's folder under java/, optional
};

struct InstanceEntry {
    QString name;
    QString folder;  // always "instance"
    QString group;   // optional
};

struct FileEntry {
    QString path;  // normalized, see BundlePaths
    QString sha1;  // lowercase hex
    qint64 size = 0;
};

struct Manifest {
    int formatVersion = 0;
    Kind kind = Kind::Versions;
    QString name;
    QString createdAt;
    QString createdBy;
    QList<ComponentEntry> components;
    QList<JavaEntry> java;
    std::optional<InstanceEntry> instance;
    QList<FileEntry> files;

    qint64 totalSize() const;
};

// On success `manifest` is set. On failure `message` is plain language for the user and `details` is technical.
struct ReadResult {
    std::optional<Manifest> manifest;
    QString message;
    QString details;
};

ReadResult parseManifest(const QByteArray& json);

// The manifest as compact JSON. parseManifest reads it back unchanged (spec §2.2).
QByteArray serializeManifest(const Manifest& manifest);

// Reads the manifest inside a zip. A zip without one gets the "isn't an offline bundle" message.
ReadResult readManifestFromZip(const QString& zipPath);

// True if `path` is a .zip file that contains an offline-bundle manifest.
bool isBundle(const QString& path);
}  // namespace OfflineBundle
