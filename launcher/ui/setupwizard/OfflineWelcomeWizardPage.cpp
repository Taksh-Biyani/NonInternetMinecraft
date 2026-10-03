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

#include "OfflineWelcomeWizardPage.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "ui/widgets/AccessibilityOptionsWidget.h"
#include "ui/widgets/GuideButton.h"

OfflineWelcomeWizardPage::OfflineWelcomeWizardPage(QWidget* parent) : BaseWizardPage(parent)
{
    auto* layout = new QVBoxLayout(this);
    m_text = new QLabel(this);
    m_text->setWordWrap(true);
    m_text->setTextFormat(Qt::RichText);
    layout->addWidget(m_text);

    m_group = new QGroupBox(this);
    auto* groupLayout = new QVBoxLayout(m_group);
    m_options = new AccessibilityOptionsWidget(m_group);
    groupLayout->addWidget(m_options);
    layout->addWidget(m_group);
    layout->addStretch(1);

    auto* helpRow = new QHBoxLayout();
    helpRow->addStretch(1);
    helpRow->addWidget(new GuideButton(OfflineGuide::Section::Start, this));
    layout->addLayout(helpRow);

    // Applied at once, so someone who needs bigger text or high contrast can read the rest of the wizard.
    connect(m_options, &AccessibilityOptionsWidget::changed, m_options, &AccessibilityOptionsWidget::save);
    retranslate();
}

bool OfflineWelcomeWizardPage::validatePage()
{
    m_options->save();
    return true;
}

void OfflineWelcomeWizardPage::retranslate()
{
    setTitle(tr("Welcome to PineconeMC Offline"));
    m_text->setText(
        tr("<p>This launcher plays Minecraft on computers <b>without internet</b>.</p>"
           "<p>New Minecraft versions, mod loaders, modpacks and Java arrive as an <b>offline bundle</b>: one file that you make on "
           "a computer with internet and carry over on a USB stick.</p>"
           "<table cellpadding=\"8\" align=\"center\"><tr>"
           "<td align=\"center\"><b>Computer with internet</b><br>Export Bundle</td><td>&rarr;</td>"
           "<td align=\"center\"><b>USB stick</b><br>one .zip file</td><td>&rarr;</td>"
           "<td align=\"center\"><b>This computer</b><br>Import Bundle</td></tr></table>"));
    m_group->setTitle(tr("Make the launcher easier to read"));
}
