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

#include "OfflineGetStartedWizardPage.h"

#include <QCommandLinkButton>
#include <QDir>
#include <QLabel>
#include <QVBoxLayout>
#include <QWizard>
#include <algorithm>

#include "offline/OfflineMode.h"
#include "ui/widgets/GuideButton.h"

OfflineGetStartedWizardPage::OfflineGetStartedWizardPage(QWidget* parent) : BaseWizardPage(parent)
{
    auto* layout = new QVBoxLayout(this);
    m_text = new QLabel(this);
    m_text->setWordWrap(true);
    layout->addWidget(m_text);
    m_import = new QCommandLinkButton(this);
    m_create = new QCommandLinkButton(this);
    m_guide = new QCommandLinkButton(this);
    layout->addWidget(m_import);
    layout->addWidget(m_create);
    layout->addWidget(m_guide);
    layout->addStretch(1);
    connect(m_import, &QCommandLinkButton::clicked, this, [this] { choose(Action::ImportBundle); });
    connect(m_create, &QCommandLinkButton::clicked, this, [this] { choose(Action::CreateInstance); });
    connect(m_guide, &QCommandLinkButton::clicked, this, [this] { GuideButton::openSection(OfflineGuide::Section::Start, this); });
    retranslate();
}

bool OfflineGetStartedWizardPage::hasLocalMinecraftVersion()
{
    const auto files = QDir("meta/net.minecraft").entryList({ "*.json" }, QDir::Files);
    return std::any_of(files.begin(), files.end(), [](const QString& file) { return file != "index.json"; });
}

void OfflineGetStartedWizardPage::initializePage()
{
    // Online, Add Instance can download any version; offline it needs one from a bundle.
    const bool canCreate = !OfflineMode::globallyOffline() || hasLocalMinecraftVersion();
    m_create->setEnabled(canCreate);
    m_create->setDescription(canCreate ? tr("Set up a game with the Minecraft versions on this computer.")
                                       : tr("Import a bundle first: there's no Minecraft version on this computer yet."));
}

void OfflineGetStartedWizardPage::choose(Action action)
{
    m_action = action;
    wizard()->accept();
}

void OfflineGetStartedWizardPage::retranslate()
{
    setTitle(tr("You're ready"));
    m_text->setText(tr("What would you like to do first? You can also just press Finish."));
    m_import->setText(tr("Import a bundle"));
    m_import->setDescription(tr("Add Minecraft versions, mod loaders, modpacks or Java from a USB stick."));
    m_create->setText(tr("Create an instance"));
    m_create->setDescription(tr("Set up a game with the Minecraft versions on this computer."));
    m_guide->setText(tr("Open the guide"));
    m_guide->setDescription(tr("Step-by-step help for everything, including how to make a bundle."));
}
