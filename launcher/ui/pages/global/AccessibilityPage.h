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

#include <QWidget>

#include "ui/pages/BasePage.h"

class AccessibilityOptionsWidget;

class AccessibilityPage : public QWidget, public BasePage {
    Q_OBJECT
   public:
    explicit AccessibilityPage(QWidget* parent = nullptr);

    QString displayName() const override { return tr("Accessibility"); }
    QIcon icon() const override { return QIcon::fromTheme("appearance"); }
    QString id() const override { return "accessibility-settings"; }
    QString helpPage() const override { return "offline:accessibility"; }
    bool apply() override;
    void retranslate() override {}

   private:
    AccessibilityOptionsWidget* m_options;
};
