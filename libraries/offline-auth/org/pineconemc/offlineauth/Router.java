// SPDX-License-Identifier: GPL-3.0-only
// PineconeMC Offline - Copyright (C) 2026 PineconeMC Offline Contributors
package org.pineconemc.offlineauth;

import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * The Yggdrasil endpoints authlib-injector forwards to us. Every player is an offline player:
 * joining always succeeds, and every name maps to its offline UUID. No skins are served.
 */
final class Router {
    static final class Response {
        final int status;
        final String body;

        Response(int status, String body) {
            this.status = status;
            this.body = body;
        }
    }

    private static final String SESSION = "/sessionserver/session/minecraft/";
    private static final String METADATA = "{\"meta\":{\"serverName\":\"PineconeMC Offline\","
            + "\"implementationName\":\"pinecone-offline-auth\",\"implementationVersion\":\"1.0.0\"},\"skinDomains\":[]}";
    private static final String ATTRIBUTES = "{\"privileges\":{\"onlineChat\":{\"enabled\":true},\"multiplayerServer\":{\"enabled\":true},"
            + "\"multiplayerRealms\":{\"enabled\":false},\"telemetry\":{\"enabled\":false},\"optionalTelemetry\":{\"enabled\":false}},"
            + "\"profanityFilterPreferences\":{\"profanityFilterOn\":false},\"banStatus\":{\"bannedScopes\":{}}}";
    private static final Pattern JSON_STRING = Pattern.compile("\"((?:[^\"\\\\]|\\\\.)*)\"");

    // Names seen in lookups, so a later profile-by-UUID request can be answered.
    private final Map<String, String> namesByUuid = new ConcurrentHashMap<>();

    Response handle(String method, String target, String body) {
        String path = target;
        String query = "";
        int q = target.indexOf('?');
        if (q >= 0) {
            path = target.substring(0, q);
            query = target.substring(q + 1);
        }
        while (path.length() > 1 && path.endsWith("/")) {
            path = path.substring(0, path.length() - 1);
        }
        boolean get = method.equals("GET");
        boolean post = method.equals("POST");

        if (get && (path.equals("/") || path.isEmpty())) {
            return ok(METADATA);
        }
        if (post && path.equals(SESSION + "join")) {
            return noContent();
        }
        if (get && path.equals(SESSION + "hasJoined")) {
            String name = queryParam(query, "username");
            return name == null || name.isEmpty() ? noContent() : ok(OfflineProfiles.profileJson(remember(name), true));
        }
        if (get && path.startsWith(SESSION + "profile/")) {
            String uuid = path.substring((SESSION + "profile/").length()).replace("-", "").toLowerCase(Locale.ROOT);
            String name = namesByUuid.get(uuid);
            return name == null ? noContent() : ok(OfflineProfiles.profileJson(name, true));
        }
        if (post && (path.equals("/api/profiles/minecraft") || path.equals("/minecraftservices/minecraft/profile/lookup/bulk/byname"))) {
            StringBuilder out = new StringBuilder("[");
            Matcher m = JSON_STRING.matcher(body);
            while (m.find()) {
                if (out.length() > 1) {
                    out.append(',');
                }
                out.append(OfflineProfiles.profileJson(remember(Json.unquote(m.group(1))), false));
            }
            return ok(out.append(']').toString());
        }
        String single = null;
        if (get && path.startsWith("/api/users/profiles/minecraft/")) {
            single = path.substring("/api/users/profiles/minecraft/".length());
        } else if (get && path.startsWith("/minecraftservices/minecraft/profile/lookup/name/")) {
            single = path.substring("/minecraftservices/minecraft/profile/lookup/name/".length());
        }
        if (single != null && !single.isEmpty()) {
            return ok(OfflineProfiles.profileJson(remember(urlDecode(single)), false));
        }
        if (get && path.equals("/minecraftservices/player/attributes")) {
            return ok(ATTRIBUTES);
        }
        if (get && path.equals("/minecraftservices/privacy/blocklist")) {
            return ok("{\"blockedProfiles\":[]}");
        }
        if (get && path.equals("/minecraftservices/publickeys")) {
            return ok("{\"profilePropertyKeys\":[],\"playerCertificateKeys\":[]}");
        }
        return new Response(404, "{\"error\":\"NotFound\",\"errorMessage\":\"Not handled by the PineconeMC Offline sign-in server\"}");
    }

    private String remember(String name) {
        namesByUuid.put(OfflineProfiles.uuidFor(name), name);
        return name;
    }

    private static String queryParam(String query, String key) {
        for (String pair : query.split("&")) {
            int eq = pair.indexOf('=');
            String k = eq >= 0 ? pair.substring(0, eq) : pair;
            if (k.equals(key)) {
                return urlDecode(eq >= 0 ? pair.substring(eq + 1) : "");
            }
        }
        return null;
    }

    private static String urlDecode(String s) {
        try {
            return URLDecoder.decode(s, "UTF-8");
        } catch (UnsupportedEncodingException | IllegalArgumentException e) {
            return s;
        }
    }

    private static Response ok(String body) {
        return new Response(200, body);
    }

    private static Response noContent() {
        return new Response(204, "");
    }
}
