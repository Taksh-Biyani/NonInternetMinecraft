// SPDX-License-Identifier: GPL-3.0-only
// PineconeMC Offline - Copyright (C) 2026 PineconeMC Offline Contributors
package org.pineconemc.offlineauth;

import java.nio.charset.StandardCharsets;
import java.util.UUID;

final class OfflineProfiles {
    private OfflineProfiles() {}

    /** The UUID Minecraft and the launcher give an offline player: nameUUIDFromBytes("OfflinePlayer:" + name), no dashes. */
    static String uuidFor(String name) {
        return UUID.nameUUIDFromBytes(("OfflinePlayer:" + name).getBytes(StandardCharsets.UTF_8)).toString().replace("-", "");
    }

    /** A Yggdrasil profile object. Session-server responses carry an (empty) properties list; name lookups don't. */
    static String profileJson(String name, boolean withProperties) {
        return "{\"id\":\"" + uuidFor(name) + "\",\"name\":" + Json.quote(name) + (withProperties ? ",\"properties\":[]" : "") + "}";
    }
}
