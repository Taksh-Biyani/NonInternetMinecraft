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

#include "GuideLinks.h"

#include <QCoreApplication>
#include <QRegularExpression>

namespace OfflineGuide {

static QString tr(const char* text)
{
    return QCoreApplication::translate("OfflineGuide", text);
}

QList<Section> allSections()
{
    return { Section::Start,         Section::Install,         Section::FirstRun,     Section::OfflineMode,
             Section::MakeBundle,    Section::BundleModpacks,  Section::ImportBundle, Section::Updating,
             Section::Java,          Section::Worlds,          Section::Lan,          Section::UpdateLauncher,
             Section::Accessibility, Section::Troubleshooting, Section::MissingFiles, Section::Legal };
}

QString anchor(Section section)
{
    switch (section) {
        case Section::Start: return "start";
        case Section::Install: return "install";
        case Section::FirstRun: return "first-run";
        case Section::OfflineMode: return "offline-mode";
        case Section::MakeBundle: return "make-bundle";
        case Section::BundleModpacks: return "bundle-modpacks";
        case Section::ImportBundle: return "import-bundle";
        case Section::Updating: return "updating";
        case Section::Java: return "java";
        case Section::Worlds: return "worlds";
        case Section::Lan: return "lan";
        case Section::UpdateLauncher: return "update-launcher";
        case Section::Accessibility: return "accessibility";
        case Section::Troubleshooting: return "troubleshooting";
        case Section::MissingFiles: return "missing-files";
        case Section::Legal: return "legal";
    }
    return "start";
}

QString title(Section section)
{
    switch (section) {
        case Section::Start: return tr("What PineconeMC Offline is");
        case Section::Install: return tr("Installing the launcher");
        case Section::FirstRun: return tr("First start and player names");
        case Section::OfflineMode: return tr("Offline mode");
        case Section::MakeBundle: return tr("Making a bundle on a computer with internet");
        case Section::BundleModpacks: return tr("Bundling modpacks (Modrinth and CurseForge)");
        case Section::ImportBundle: return tr("Importing a bundle");
        case Section::Updating: return tr("Updating Minecraft or a mod loader");
        case Section::Java: return tr("Java");
        case Section::Worlds: return tr("Backing up and moving worlds");
        case Section::Lan: return tr("Playing together on LAN");
        case Section::UpdateLauncher: return tr("Updating the launcher");
        case Section::Accessibility: return tr("Accessibility");
        case Section::Troubleshooting: return tr("Troubleshooting");
        case Section::MissingFiles: return tr("When a game says files are missing");
        case Section::Legal: return tr("Legal and credits");
    }
    return {};
}

std::optional<Section> sectionFromAnchor(const QString& anchorText)
{
    for (Section section : allSections()) {
        if (anchor(section) == anchorText)
            return section;
    }
    return std::nullopt;
}

QString forTextBrowser(QString html)
{
    static const QRegularExpression style("<style>.*?</style>\\s*", QRegularExpression::DotMatchesEverythingOption);
    static const QRegularExpression summary("<summary>(.*?)</summary>", QRegularExpression::DotMatchesEverythingOption);
    static const QRegularExpression details("</?details[^>]*>");
    static const QRegularExpression flow("<div class=\"flow\"[^>]*>(.*?)\\n</div>", QRegularExpression::DotMatchesEverythingOption);
    static const QRegularExpression cell("<div class=\"(step|arrow)\"[^>]*>");

    // Qt's rich text has no CSS variables, flexbox or media queries: the guide window styles the page itself.
    html.remove(style);
    // No <details> either: show each troubleshooting question as a heading, with its answer below.
    html.replace(summary, "<h4>\\1</h4>");
    html.remove(details);
    // The bundle diagram is a flexbox row of boxes; a one-row table looks the same.
    for (auto match = flow.match(html); match.hasMatch(); match = flow.match(html)) {
        QString row = match.captured(1);
        row.replace(cell, "<td class=\"\\1\">");
        row.replace("</div>", "</td>");
        html.replace(match.capturedStart(), match.capturedLength(), "<table class=\"flow\" align=\"center\"><tr>" + row + "</tr></table>");
    }
    return html;
}

}  // namespace OfflineGuide
