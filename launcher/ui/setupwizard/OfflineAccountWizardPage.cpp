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

#include "OfflineAccountWizardPage.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWizard>

#include "Application.h"
#include "minecraft/auth/AccountList.h"
#include "minecraft/auth/MinecraftAccount.h"
#include "offline/OfflineMode.h"
#include "offline/OfflineName.h"
#include "ui/dialogs/MSALoginDialog.h"

OfflineAccountWizardPage::OfflineAccountWizardPage(QWidget* parent) : BaseWizardPage(parent)
{
    auto* layout = new QVBoxLayout(this);
    m_intro = new QLabel(this);
    m_intro->setWordWrap(true);
    layout->addWidget(m_intro);

    auto* row = new QHBoxLayout();
    m_nameLabel = new QLabel(this);
    m_name = new QLineEdit(this);
    m_name->setMaxLength(16);
    m_nameLabel->setBuddy(m_name);
    row->addWidget(m_nameLabel);
    row->addWidget(m_name, 1);
    layout->addLayout(row);

    m_problem = new QLabel(this);
    m_problem->setWordWrap(true);
    layout->addWidget(m_problem);

    m_later = new QLabel(this);
    m_later->setWordWrap(true);
    layout->addWidget(m_later);

    m_microsoft = new QPushButton(this);
    layout->addWidget(m_microsoft, 0, Qt::AlignLeft);
    layout->addStretch(1);

    m_microsoft->setVisible(!OfflineMode::globallyOffline());
    connect(APPLICATION->offlineMode(), &OfflineMode::offlineChanged, m_microsoft, [this](bool offline) { m_microsoft->setVisible(!offline); });
    connect(m_microsoft, &QPushButton::clicked, this, &OfflineAccountWizardPage::signInWithMicrosoft);
    connect(m_name, &QLineEdit::textChanged, this, [this] {
        updateProblem();
        emit completeChanged();
    });
    retranslate();
}

bool OfflineAccountWizardPage::isComplete() const
{
    const QString name = m_name->text();
    return name.isEmpty() || OfflineName::problem(name).isEmpty();
}

bool OfflineAccountWizardPage::validatePage()
{
    const QString name = m_name->text();
    if (name.isEmpty() || name == m_created)
        return true;
    auto* accounts = APPLICATION->accounts();
    if (!m_created.isEmpty()) {
        // Back, a new name, Next: replace the account made a moment ago instead of keeping both.
        if (auto old = accounts->getAccountByProfileName(m_created)) {
            const int row = accounts->findAccountByProfileId(old->profileId());
            if (row >= 0)
                accounts->removeAccount(accounts->index(row, 0));
        }
        m_created.clear();
    }
    if (auto account = MinecraftAccount::createOffline(name)) {
        account->login()->start();  // offline accounts finish at once (as AccountListPage does)
        accounts->addAccount(account);
        accounts->setDefaultAccount(account);
        m_created = name;
    }
    return true;
}

void OfflineAccountWizardPage::signInWithMicrosoft()
{
    wizard()->hide();
    auto account = MSALoginDialog::newAccount(nullptr);
    wizard()->show();
    if (account) {
        APPLICATION->accounts()->addAccount(account);
        APPLICATION->accounts()->setDefaultAccount(account);
        wizard()->next();
    }
}

void OfflineAccountWizardPage::updateProblem()
{
    const QString name = m_name->text();
    if (name.isEmpty()) {
        m_problem->setText(tr("Leave it empty to skip this step."));
        return;
    }
    const QString problem = OfflineName::problem(name);
    m_problem->setText(problem.isEmpty() ? tr("%1 looks good.").arg(name) : problem);
}

void OfflineAccountWizardPage::retranslate()
{
    setTitle(tr("Choose your player name"));
    m_intro->setText(tr("This is your name in the game, and the name friends see when you play together on LAN. It works "
                        "without internet."));
    m_nameLabel->setText(tr("Player &name:"));
    m_name->setPlaceholderText(tr("3 to 16 letters, digits or _"));
    m_later->setText(tr("You can add more names later under Accounts. With internet you can also sign in with a Microsoft account "
                        "there."));
    m_microsoft->setText(tr("Sign in with Microsoft instead"));
    updateProblem();
}
