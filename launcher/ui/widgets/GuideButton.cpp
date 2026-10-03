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

#include "GuideButton.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "DesktopServices.h"
#include "ui/dialogs/FriendlyErrorDialog.h"

GuideButton::GuideButton(OfflineGuide::Section section, QWidget* parent) : QToolButton(parent)
{
    setText(QStringLiteral("?"));
    setToolTip(tr("Help: %1").arg(OfflineGuide::title(section)));
    setAccessibleName(tr("Help"));
    setAccessibleDescription(tr("Opens the guide at \"%1\"").arg(OfflineGuide::title(section)));
    connect(this, &QToolButton::clicked, this, [this, section] { openSection(section, this); });
}

void GuideButton::openSection(OfflineGuide::Section section, QWidget* parent)
{
    const QString guide = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("Guide.html");
    if (!QFileInfo::exists(guide)) {
        FriendlyErrorDialog::show(parent, tr("Guide not found"),
                                  tr("The guide (Guide.html) belongs in the launcher's folder, but it isn't there. Copy the whole "
                                     "launcher folder again from the USB stick or the zip file."),
                                  guide);
        return;
    }
    // Windows drops "#section" from file links, so open a small page that forwards to the right section.
    const QString redirect = QDir::current().absoluteFilePath("cache/open-guide.html");
    QDir().mkpath(QFileInfo(redirect).absolutePath());
    QFile file(redirect);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(OfflineGuide::redirectPage(OfflineGuide::sectionUrl(guide, section)).toUtf8());
        file.close();
        DesktopServices::openUrl(QUrl::fromLocalFile(redirect));
    } else {
        DesktopServices::openUrl(QUrl::fromLocalFile(guide));
    }
}
