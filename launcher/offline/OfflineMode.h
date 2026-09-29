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

#include <QObject>
#include <QString>

#include "net/Mode.h"

/**
 * Decides whether the launcher is offline for this session.
 *
 * Offline means: the user chose "Always offline", or the setting is "Auto" and the
 * startup network check has not confirmed that the internet is reachable (yet).
 * Code about to use the network asks OfflineMode::effective() which mode it may use.
 * This never affects the game's own networking (LAN, servers).
 */
class OfflineMode : public QObject {
    Q_OBJECT

   public:
    enum class Setting { Auto, AlwaysOffline, AlwaysOnline };
    Q_ENUM(Setting)
    enum class CheckState { Pending, Reachable, Unreachable };
    Q_ENUM(CheckState)

    static Setting settingFromString(const QString& value);
    static QString settingToString(Setting setting);
    static bool resolveOffline(Setting setting, CheckState check);
    static Net::Mode resolve(Setting setting, CheckState check, Net::Mode wanted);

    /// The mode a caller that wants `wanted` may use now. Returns `wanted` when no OfflineMode exists (unit tests).
    static Net::Mode effective(Net::Mode wanted);
    /// True when an OfflineMode exists and it says the launcher is offline.
    static bool globallyOffline();

    explicit OfflineMode(QObject* parent = nullptr);
    ~OfflineMode() override;

    Setting setting() const { return m_setting; }
    void setSetting(Setting setting);
    CheckState checkState() const { return m_check; }
    void setCheckState(CheckState check);
    bool isOffline() const { return m_offline; }

   signals:
    void offlineChanged(bool offline);

   private:
    void recompute();

    static OfflineMode* s_instance;

    Setting m_setting = Setting::Auto;
    CheckState m_check = CheckState::Pending;
    bool m_offline = true;
};
