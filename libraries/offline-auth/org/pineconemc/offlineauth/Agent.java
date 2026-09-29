// SPDX-License-Identifier: GPL-3.0-only
// PineconeMC Offline - Copyright (C) 2026 PineconeMC Offline Contributors
package org.pineconemc.offlineauth;

import java.io.File;
import java.io.IOException;
import java.lang.instrument.Instrumentation;

/**
 * Java agent: runs the offline sign-in server on 127.0.0.1 inside the game process. authlib-injector, the next
 * agent on the command line, points Minecraft's auth calls at it, so LAN works for offline accounts without
 * internet. Agent argument: the port to listen on. Problems are logged and never stop the game from starting.
 */
public final class Agent {
    private Agent() {}

    public static void premain(String agentArgs, Instrumentation instrumentation) {
        // The game's working directory is the instance's .minecraft folder, so this lands next to latest.log.
        Log log = new Log(new File("logs", "pinecone-offline-auth.log"));
        int port;
        try {
            port = Integer.parseInt(agentArgs == null ? "" : agentArgs.trim());
        } catch (NumberFormatException e) {
            log.line("invalid agent argument \"" + agentArgs + "\": expected a port number");
            return;
        }
        try {
            AuthServer server = new AuthServer(port, new Router(), log);
            server.start();
            log.line("offline sign-in server listening on http://127.0.0.1:" + server.port());
        } catch (IOException e) {
            log.line("couldn't listen on 127.0.0.1:" + port + " (" + e + "); LAN sign-in won't work in this session");
        }
    }
}
