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

#include <QDialog>
#include <optional>

#include "offline/GuideLinks.h"

class QDialogButtonBox;
class QPushButton;

// A plain-language error (spec §7.1): the message, a collapsible "Details" section and a "Copy details" button,
// optionally an "Open guide" button and one action button (e.g. "Import Bundle...").
class FriendlyErrorDialog : public QDialog {
    Q_OBJECT
   public:
    // The dialog's result code when the button from addActionButton() was clicked.
    static constexpr int ActionResult = 2;

    FriendlyErrorDialog(QWidget* parent,
                        const QString& title,
                        const QString& message,
                        const QString& details,
                        std::optional<OfflineGuide::Section> guide = std::nullopt);

    // Adds a default button that closes the dialog with ActionResult.
    QPushButton* addActionButton(const QString& text);

    static void show(QWidget* parent,
                     const QString& title,
                     const QString& message,
                     const QString& details,
                     std::optional<OfflineGuide::Section> guide = std::nullopt);

   private:
    QDialogButtonBox* m_buttons = nullptr;
};
