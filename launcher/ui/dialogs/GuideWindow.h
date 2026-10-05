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

#include "offline/GuideLinks.h"

class QLineEdit;
class QTextBrowser;

// The offline guide (Guide.html, built into the launcher) in a window of its own, styled like the rest of the launcher.
class GuideWindow : public QDialog {
    Q_OBJECT
   public:
    // Opens the guide at `section`, reusing the window if it's already open.
    static void showSection(OfflineGuide::Section section);

   protected:
    void changeEvent(QEvent* event) override;

   private:
    explicit GuideWindow(QWidget* parent);
    void loadGuide();
    void goTo(OfflineGuide::Section section);
    void find(bool backwards);

    QTextBrowser* m_browser = nullptr;
    QLineEdit* m_find = nullptr;
};
