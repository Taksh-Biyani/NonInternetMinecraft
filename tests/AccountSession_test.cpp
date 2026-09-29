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

#include <QTest>

#include <minecraft/auth/AuthSession.h>
#include <minecraft/auth/MinecraftAccount.h>

class AccountSessionTest : public QObject {
    Q_OBJECT

    static AuthSessionPtr fill(const MinecraftAccountPtr& account, int elyPatchPreference)
    {
        auto session = std::make_shared<AuthSession>();
        account->fillSession(session, elyPatchPreference);
        return session;
    }

   private slots:
    void test_offlineAccount_usesLocalAuthNeverEly_data()
    {
        QTest::addColumn<int>("elyPatchPreference");
        QTest::newRow("always") << 0;
        QTest::newRow("ely and offline (default)") << 1;
        QTest::newRow("ely only") << 2;
        QTest::newRow("never") << 3;
    }

    void test_offlineAccount_usesLocalAuthNeverEly()
    {
        QFETCH(int, elyPatchPreference);
        const auto session = fill(MinecraftAccount::createOffline("Tester"), elyPatchPreference);
        QVERIFY(session->wantsLocalAuth);
        QVERIFY(!session->wantsElyPatch);
        QCOMPARE(session->uuid, QString("f3d28cb072253cb1baeb2dadd2be89ae"));
    }

    void test_elyAndMicrosoftAccounts_unchanged()
    {
        QVERIFY(fill(MinecraftAccount::createBlank(AccountType::Ely), 1)->wantsElyPatch);
        QVERIFY(!fill(MinecraftAccount::createBlank(AccountType::Ely), 1)->wantsLocalAuth);
        QVERIFY(fill(MinecraftAccount::createBlank(AccountType::MSA), 0)->wantsElyPatch);
        QVERIFY(!fill(MinecraftAccount::createBlank(AccountType::MSA), 2)->wantsElyPatch);
        QVERIFY(!fill(MinecraftAccount::createBlank(AccountType::MSA), 0)->wantsLocalAuth);
    }
};

QTEST_GUILESS_MAIN(AccountSessionTest)

#include "AccountSession_test.moc"
