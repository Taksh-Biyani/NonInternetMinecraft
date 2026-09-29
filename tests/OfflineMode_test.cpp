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

#include <QSignalSpy>
#include <QTest>

#include <offline/OfflineMode.h>

class OfflineModeTest : public QObject {
    Q_OBJECT

   private slots:
    void test_resolve_data()
    {
        QTest::addColumn<OfflineMode::Setting>("setting");
        QTest::addColumn<OfflineMode::CheckState>("check");
        QTest::addColumn<bool>("offline");

        QTest::newRow("auto, check pending") << OfflineMode::Setting::Auto << OfflineMode::CheckState::Pending << true;
        QTest::newRow("auto, reachable") << OfflineMode::Setting::Auto << OfflineMode::CheckState::Reachable << false;
        QTest::newRow("auto, unreachable") << OfflineMode::Setting::Auto << OfflineMode::CheckState::Unreachable << true;
        QTest::newRow("always offline, reachable") << OfflineMode::Setting::AlwaysOffline << OfflineMode::CheckState::Reachable << true;
        QTest::newRow("always offline, pending") << OfflineMode::Setting::AlwaysOffline << OfflineMode::CheckState::Pending << true;
        QTest::newRow("always online, unreachable") << OfflineMode::Setting::AlwaysOnline << OfflineMode::CheckState::Unreachable << false;
        QTest::newRow("always online, pending") << OfflineMode::Setting::AlwaysOnline << OfflineMode::CheckState::Pending << false;
    }

    void test_resolve()
    {
        QFETCH(OfflineMode::Setting, setting);
        QFETCH(OfflineMode::CheckState, check);
        QFETCH(bool, offline);

        QCOMPARE(OfflineMode::resolveOffline(setting, check), offline);
        QVERIFY(OfflineMode::resolve(setting, check, Net::Mode::Online) == (offline ? Net::Mode::Offline : Net::Mode::Online));
        // A caller that already asked for offline always stays offline.
        QVERIFY(OfflineMode::resolve(setting, check, Net::Mode::Offline) == Net::Mode::Offline);
    }

    void test_settingStrings()
    {
        for (auto setting : { OfflineMode::Setting::Auto, OfflineMode::Setting::AlwaysOffline, OfflineMode::Setting::AlwaysOnline }) {
            QCOMPARE(OfflineMode::settingFromString(OfflineMode::settingToString(setting)), setting);
        }
        QCOMPARE(OfflineMode::settingToString(OfflineMode::Setting::Auto), QString("Auto"));
        QCOMPARE(OfflineMode::settingToString(OfflineMode::Setting::AlwaysOffline), QString("AlwaysOffline"));
        QCOMPARE(OfflineMode::settingToString(OfflineMode::Setting::AlwaysOnline), QString("AlwaysOnline"));
        QCOMPARE(OfflineMode::settingFromString(""), OfflineMode::Setting::Auto);
        QCOMPARE(OfflineMode::settingFromString("garbage"), OfflineMode::Setting::Auto);
    }

    void test_effectiveWithoutInstance()
    {
        QVERIFY(OfflineMode::effective(Net::Mode::Online) == Net::Mode::Online);
        QVERIFY(OfflineMode::effective(Net::Mode::Offline) == Net::Mode::Offline);
        QVERIFY(!OfflineMode::globallyOffline());
    }

    void test_instanceStateAndSignals()
    {
        OfflineMode mode;
        QVERIFY(mode.isOffline());  // Auto + Pending
        QVERIFY(OfflineMode::globallyOffline());
        QVERIFY(OfflineMode::effective(Net::Mode::Online) == Net::Mode::Offline);

        QSignalSpy spy(&mode, &OfflineMode::offlineChanged);

        mode.setCheckState(OfflineMode::CheckState::Reachable);
        QVERIFY(!mode.isOffline());
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toBool(), false);
        QVERIFY(OfflineMode::effective(Net::Mode::Online) == Net::Mode::Online);

        mode.setSetting(OfflineMode::Setting::AlwaysOffline);
        QVERIFY(mode.isOffline());
        QCOMPARE(spy.count(), 2);
        QCOMPARE(spy.at(1).at(0).toBool(), true);

        mode.setSetting(OfflineMode::Setting::AlwaysOffline);  // no change, no signal
        QCOMPARE(spy.count(), 2);
    }

    void test_instanceUnregistersOnDestruction()
    {
        {
            OfflineMode mode;
            QVERIFY(OfflineMode::globallyOffline());
        }
        QVERIFY(!OfflineMode::globallyOffline());
        QVERIFY(OfflineMode::effective(Net::Mode::Online) == Net::Mode::Online);
    }
};

QTEST_GUILESS_MAIN(OfflineModeTest)

#include "OfflineMode_test.moc"
