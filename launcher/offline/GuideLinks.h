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
#include <optional>

// The sections of the bundled guide (Guide.html next to the exe). Every "?" button and "Open guide" link names one of
// these; GuideLinks_test checks that docs/guide/Guide.html has each anchor.
namespace OfflineGuide {
enum class Section {
    Start,
    Install,
    FirstRun,
    OfflineMode,
    MakeBundle,
    BundleModpacks,
    ImportBundle,
    Updating,
    Java,
    Worlds,
    Lan,
    UpdateLauncher,
    Accessibility,
    Troubleshooting,
    MissingFiles,
    Legal
};

QList<Section> allSections();
QString anchor(Section section);  // e.g. "import-bundle"
QString title(Section section);   // e.g. "Importing a bundle"
std::optional<Section> sectionFromAnchor(const QString& anchorText);

// Guide.html adapted for Qt's rich text (QTextBrowser), which the in-app guide window uses.
QString forTextBrowser(QString html);
}  // namespace OfflineGuide
