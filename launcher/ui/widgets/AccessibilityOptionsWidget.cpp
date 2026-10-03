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

#include "AccessibilityOptionsWidget.h"

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <algorithm>

#include "Application.h"
#include "settings/SettingsObject.h"
#include "ui/themes/Accessibility.h"
#include "ui/themes/ThemeManager.h"

using Accessibility::Contrast;

AccessibilityOptionsWidget::AccessibilityOptionsWidget(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QFormLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_textScale = new QComboBox(this);
    for (int scale : Accessibility::textScales())
        m_textScale->addItem(scale == 100 ? tr("%1% (normal)").arg(scale) : tr("%1%").arg(scale), scale);
    m_textScale->setToolTip(tr("Makes the launcher's text bigger. Minecraft's own size is in the game: Options → Video Settings → "
                               "GUI Scale."));
    auto* textLabel = new QLabel(tr("&Text size:"), this);
    textLabel->setBuddy(m_textScale);
    layout->addRow(textLabel, m_textScale);

    m_contrast = new QComboBox(this);
    m_contrast->addItem(tr("Automatic (on when Windows high contrast is on)"), Accessibility::contrastToString(Contrast::Auto));
    m_contrast->addItem(tr("Off"), Accessibility::contrastToString(Contrast::Off));
    m_contrast->addItem(tr("On: white text on black"), Accessibility::contrastToString(Contrast::Dark));
    m_contrast->addItem(tr("On: black text on white"), Accessibility::contrastToString(Contrast::Light));
    m_contrast->setToolTip(tr("Strong colours and a thick outline around whatever the keyboard is on."));
    auto* contrastLabel = new QLabel(tr("&High contrast:"), this);
    contrastLabel->setBuddy(m_contrast);
    layout->addRow(contrastLabel, m_contrast);

    load();
    connect(m_textScale, &QComboBox::currentIndexChanged, this, &AccessibilityOptionsWidget::changed);
    connect(m_contrast, &QComboBox::currentIndexChanged, this, &AccessibilityOptionsWidget::changed);
}

void AccessibilityOptionsWidget::load()
{
    auto settings = APPLICATION->settings();
    const QSignalBlocker blockScale(m_textScale);
    const QSignalBlocker blockContrast(m_contrast);
    m_textScale->setCurrentIndex(std::max(0, m_textScale->findData(Accessibility::clampTextScale(settings->get("TextScale").toInt()))));
    const QString contrast = Accessibility::contrastToString(Accessibility::contrastFromString(settings->get("HighContrast").toString()));
    m_contrast->setCurrentIndex(std::max(0, m_contrast->findData(contrast)));
}

void AccessibilityOptionsWidget::save()
{
    auto settings = APPLICATION->settings();
    settings->set("TextScale", m_textScale->currentData().toInt());
    settings->set("HighContrast", m_contrast->currentData().toString());
    APPLICATION->themeManager()->applyCurrentlySelectedTheme();
}
