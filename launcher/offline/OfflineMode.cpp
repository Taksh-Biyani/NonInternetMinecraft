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

#include "OfflineMode.h"

OfflineMode* OfflineMode::s_instance = nullptr;

OfflineMode::Setting OfflineMode::settingFromString(const QString& value)
{
    if (value == QLatin1String("AlwaysOffline")) {
        return Setting::AlwaysOffline;
    }
    if (value == QLatin1String("AlwaysOnline")) {
        return Setting::AlwaysOnline;
    }
    return Setting::Auto;
}

QString OfflineMode::settingToString(Setting setting)
{
    switch (setting) {
        case Setting::AlwaysOffline:
            return QStringLiteral("AlwaysOffline");
        case Setting::AlwaysOnline:
            return QStringLiteral("AlwaysOnline");
        case Setting::Auto:
            break;
    }
    return QStringLiteral("Auto");
}

bool OfflineMode::resolveOffline(Setting setting, CheckState check)
{
    switch (setting) {
        case Setting::AlwaysOffline:
            return true;
        case Setting::AlwaysOnline:
            return false;
        case Setting::Auto:
            break;
    }
    return check != CheckState::Reachable;
}

Net::Mode OfflineMode::resolve(Setting setting, CheckState check, Net::Mode wanted)
{
    return resolveOffline(setting, check) ? Net::Mode::Offline : wanted;
}

Net::Mode OfflineMode::effective(Net::Mode wanted)
{
    return s_instance ? resolve(s_instance->m_setting, s_instance->m_check, wanted) : wanted;
}

bool OfflineMode::globallyOffline()
{
    return s_instance && s_instance->m_offline;
}

OfflineMode::OfflineMode(QObject* parent) : QObject(parent)
{
    s_instance = this;
    m_offline = resolveOffline(m_setting, m_check);
}

OfflineMode::~OfflineMode()
{
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void OfflineMode::setSetting(Setting setting)
{
    m_setting = setting;
    recompute();
}

void OfflineMode::setCheckState(CheckState check)
{
    if (check == m_check) {
        return;
    }
    m_check = check;
    recompute();
    emit checkStateChanged(m_check);
}

void OfflineMode::recompute()
{
    const bool offline = resolveOffline(m_setting, m_check);
    if (offline == m_offline) {
        return;
    }
    m_offline = offline;
    emit offlineChanged(m_offline);
}
