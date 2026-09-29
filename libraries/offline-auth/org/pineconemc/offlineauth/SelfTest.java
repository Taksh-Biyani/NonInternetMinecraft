// SPDX-License-Identifier: GPL-3.0-only
// PineconeMC Offline - Copyright (C) 2026 PineconeMC Offline Contributors
package org.pineconemc.offlineauth;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.Proxy;
import java.net.URL;
import java.nio.charset.StandardCharsets;

/** Run by ctest (test name OfflineAuthStub): starts the server on a free port and checks every endpoint. */
public final class SelfTest {
    private static int failures = 0;

    private SelfTest() {}

    public static void main(String[] args) throws Exception {
        AuthServer server = new AuthServer(0, new Router(), new Log(null));
        server.start();
        String base = "http://127.0.0.1:" + server.port();

        check("metadata", request("GET", base + "/", null), 200, "\"implementationName\":\"pinecone-offline-auth\"");
        check("join", request("POST", base + "/sessionserver/session/minecraft/join",
                "{\"accessToken\":\"0\",\"selectedProfile\":\"f3d28cb072253cb1baeb2dadd2be89ae\",\"serverId\":\"abc\"}"), 204, "");
        check("hasJoined", request("GET", base + "/sessionserver/session/minecraft/hasJoined?username=Tester&serverId=abc", null),
                200, "{\"id\":\"f3d28cb072253cb1baeb2dadd2be89ae\",\"name\":\"Tester\",\"properties\":[]}");
        check("hasJoined without username", request("GET", base + "/sessionserver/session/minecraft/hasJoined?serverId=abc", null), 204, "");
        check("profile by uuid after hasJoined",
                request("GET", base + "/sessionserver/session/minecraft/profile/f3d28cb072253cb1baeb2dadd2be89ae?unsigned=false", null),
                200, "\"name\":\"Tester\"");
        check("unknown profile", request("GET", base + "/sessionserver/session/minecraft/profile/00000000000000000000000000000000", null), 204, "");
        check("bulk lookup", request("POST", base + "/api/profiles/minecraft", "[\"Tester\",\"Tester2\"]"), 200,
                "[{\"id\":\"f3d28cb072253cb1baeb2dadd2be89ae\",\"name\":\"Tester\"},{\"id\":\"e4c9cd18ee963574b06ae9866b0fa22d\",\"name\":\"Tester2\"}]");
        check("services bulk lookup", request("POST", base + "/minecraftservices/minecraft/profile/lookup/bulk/byname", "[\"Tester\"]"),
                200, "\"name\":\"Tester\"");
        check("name lookup", request("GET", base + "/api/users/profiles/minecraft/Tester2", null), 200,
                "{\"id\":\"e4c9cd18ee963574b06ae9866b0fa22d\",\"name\":\"Tester2\"}");
        check("services name lookup", request("GET", base + "/minecraftservices/minecraft/profile/lookup/name/Tester", null), 200,
                "\"id\":\"f3d28cb072253cb1baeb2dadd2be89ae\"");
        check("attributes", request("GET", base + "/minecraftservices/player/attributes", null), 200, "\"multiplayerServer\":{\"enabled\":true}");
        check("blocklist", request("GET", base + "/minecraftservices/privacy/blocklist", null), 200, "{\"blockedProfiles\":[]}");
        check("publickeys", request("GET", base + "/minecraftservices/publickeys", null), 200, "\"playerCertificateKeys\":[]");
        check("unknown path", request("GET", base + "/nope", null), 404, "NotFound");
        check("name with a quote is escaped", request("GET", base + "/sessionserver/session/minecraft/hasJoined?username=a%22b", null),
                200, "\"name\":\"a\\\"b\"");
        server.close();

        boolean agentSurvivedBadArgument = true;
        try {
            Agent.premain("not-a-port", null);
        } catch (RuntimeException e) {
            agentSurvivedBadArgument = false;
        }
        check("bad agent argument doesn't throw", new String[] { agentSurvivedBadArgument ? "0" : "1", "" }, 0, "");

        if (failures > 0) {
            System.out.println(failures + " check(s) failed");
            System.exit(1);
        }
        System.out.println("all checks passed");
    }

    private static void check(String name, String[] result, int status, String contains) {
        boolean ok = Integer.parseInt(result[0]) == status && result[1].contains(contains);
        System.out.println((ok ? "PASS " : "FAIL ") + name + " -> " + result[0] + " " + result[1]);
        if (!ok) {
            failures++;
        }
    }

    private static String[] request(String method, String url, String body) throws IOException {
        HttpURLConnection c = (HttpURLConnection) new URL(url).openConnection(Proxy.NO_PROXY);
        c.setRequestMethod(method);
        c.setConnectTimeout(5000);
        c.setReadTimeout(5000);
        if (body != null) {
            c.setDoOutput(true);
            c.setRequestProperty("Content-Type", "application/json");
            try (OutputStream out = c.getOutputStream()) {
                out.write(body.getBytes(StandardCharsets.UTF_8));
            }
        }
        int status = c.getResponseCode();
        InputStream in = status >= 400 ? c.getErrorStream() : c.getInputStream();
        String text = "";
        if (in != null) {
            try (InputStream s = in) {
                ByteArrayOutputStream buf = new ByteArrayOutputStream();
                byte[] chunk = new byte[4096];
                int n;
                while ((n = s.read(chunk)) > 0) {
                    buf.write(chunk, 0, n);
                }
                text = new String(buf.toByteArray(), StandardCharsets.UTF_8);
            }
        }
        return new String[] { Integer.toString(status), text };
    }
}
