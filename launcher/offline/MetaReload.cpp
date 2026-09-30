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

#include "MetaReload.h"

#include "meta/Index.h"
#include "meta/Version.h"
#include "meta/VersionList.h"

namespace OfflineBundle {

void reloadMetadata(Meta::Index* index, const QStringList& changedUids)
{
    index->invalidateLocalCopy();
    index->loadTask(Net::Mode::Offline)->start();
    for (const QString& uid : changedUids) {
        auto list = index->get(uid);
        if (!list || list->status() == Meta::BaseEntity::LoadStatus::NotLoaded)
            continue;
        list->invalidateLocalCopy();
        list->loadTask(Net::Mode::Offline)->start();
        for (const auto& version : list->versions()) {
            if (version->status() == Meta::BaseEntity::LoadStatus::NotLoaded)
                continue;
            version->invalidateLocalCopy();
            version->loadTask(Net::Mode::Offline)->start();
        }
    }
}

}  // namespace OfflineBundle
