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

#include "BaseWizardPage.h"

class QLabel;
class QLineEdit;
class QPushButton;

// First run, page 2 (spec §6.1): create an offline account. Leaving the name empty skips it.
class OfflineAccountWizardPage : public BaseWizardPage {
    Q_OBJECT
   public:
    explicit OfflineAccountWizardPage(QWidget* parent = nullptr);
    bool isComplete() const override;  // empty (skip) or a valid name
    bool validatePage() override;      // creates the account
    void retranslate() override;

   private:
    void updateProblem();
    void signInWithMicrosoft();

    QLabel* m_intro;
    QLabel* m_nameLabel;
    QLineEdit* m_name;
    QLabel* m_problem;
    QLabel* m_later;
    QPushButton* m_microsoft;
    QString m_created;  // the account this page made, so Back + Next with a new name replaces it
};
