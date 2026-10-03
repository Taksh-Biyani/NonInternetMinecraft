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

#include "AccessibilityPage.h"

#include <QLabel>
#include <QVBoxLayout>

#include "ui/widgets/AccessibilityOptionsWidget.h"

AccessibilityPage::AccessibilityPage(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("accessibilityPage"));
    auto* layout = new QVBoxLayout(this);
    m_options = new AccessibilityOptionsWidget(this);
    layout->addWidget(m_options);
    auto* note = new QLabel(tr("Changes apply when you press OK or Apply. Keyboard users: Tab moves between controls, F1 opens "
                               "the guide.\n\nMinecraft has its own accessibility options (narrator, subtitles, high-contrast "
                               "text and more) in the game under Options → Accessibility Settings."),
                            this);
    note->setWordWrap(true);
    layout->addWidget(note);
    layout->addStretch(1);
}

bool AccessibilityPage::apply()
{
    m_options->save();
    return true;
}
