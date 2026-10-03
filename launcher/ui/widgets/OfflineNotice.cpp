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

#include "OfflineNotice.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "Application.h"
#include "offline/OfflineMode.h"
#include "ui/widgets/GuideButton.h"

OfflineNotice::OfflineNotice(const QString& message, QWidget* parent) : QFrame(parent)
{
    setFrameShape(QFrame::StyledPanel);
    setAccessibleName(tr("Offline notice"));
    auto* layout = new QHBoxLayout(this);
    auto* text = new QLabel(tr("<b>You're offline.</b> %1").arg(message), this);
    text->setWordWrap(true);
    text->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    layout->addWidget(text, 1);

    auto* refresh = new QPushButton(tr("&Refresh"), this);
    refresh->setToolTip(tr("If this computer is connected to the internet now, load the list."));
    connect(refresh, &QPushButton::clicked, this, [this] {
        // "Automatic" offline mode: let the launcher notice that the internet is back, so downloads work again.
        if (APPLICATION->offlineMode()->setting() == OfflineMode::Setting::Auto)
            APPLICATION->recheckNetwork();
        emit refreshRequested();
    });
    layout->addWidget(refresh);

    auto* how = new QPushButton(tr("How do I get these offline?"), this);
    connect(how, &QPushButton::clicked, this, [this] { GuideButton::openSection(OfflineGuide::Section::BundleModpacks, this); });
    layout->addWidget(how);

    connect(APPLICATION->offlineMode(), &OfflineMode::offlineChanged, this, &OfflineNotice::updateVisibility);
    updateVisibility();
}

bool OfflineNotice::autoLoadAllowed()
{
    return !OfflineMode::globallyOffline();
}

QString OfflineNotice::modpacksMessage()
{
    return tr("Modpacks can't be browsed without internet. To play a modpack here, install it on a computer with internet, "
              "use <b>Export → Offline Bundle</b> on it, and import that file here.");
}

QString OfflineNotice::resourcesMessage()
{
    return tr("Mods, resource packs and shaders can't be downloaded without internet. Add them to this instance on a computer "
              "with internet, then bring the instance over with <b>Export → Offline Bundle</b>.");
}

void OfflineNotice::updateVisibility()
{
    setVisible(!autoLoadAllowed());
}
