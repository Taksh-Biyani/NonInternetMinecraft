// SPDX-License-Identifier: GPL-3.0-only
// PineconeMC Offline - Copyright (C) 2026 PineconeMC Offline Contributors
package org.pineconemc.systemtrust;

import java.lang.instrument.Instrumentation;
import java.security.KeyStore;
import java.util.ArrayList;
import java.util.List;

import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;
import javax.net.ssl.X509ExtendedTrustManager;

// Makes the game's Java trust every certificate that either Java or Windows trusts. Office and school networks that
// inspect HTTPS re-sign sites with their own root certificate; IT installs it into Windows, but Java only knows its
// own list, so Forge's install steps fail with "SunCertPathBuilderException" before the game window opens.
public final class Agent {
    private Agent() {}

    public static void premain(String agentArgs, Instrumentation instrumentation) {
        try {
            install();
        } catch (Throwable t) {
            // Never stop the game from starting: without this agent Java just keeps its own list.
            System.out.println("[pinecone-system-trust] not active: " + t);
        }
    }

    static boolean install() throws Exception {
        // An instance that picked its own trust store on purpose keeps it.
        if (System.getProperty("javax.net.ssl.trustStore") != null || System.getProperty("javax.net.ssl.trustStoreType") != null)
            return false;
        List<X509ExtendedTrustManager> delegates = new ArrayList<>();
        addFrom(delegates, null);  // Java's own list (cacerts)
        // The user's view of the Windows root store (includes the machine's); Java 19+ can also read the machine store.
        for (String type : new String[] { "Windows-ROOT", "Windows-ROOT-LOCALMACHINE" }) {
            try {
                KeyStore ks = KeyStore.getInstance(type);
                ks.load(null, null);
                addFrom(delegates, ks);
            } catch (Exception e) {
                // Not on Windows, or this Java doesn't have that store.
            }
        }
        if (delegates.size() < 2)
            return false;
        SSLContext context = SSLContext.getInstance("TLS");
        context.init(null, new TrustManager[] { new EitherTrustManager(delegates) }, null);
        SSLContext.setDefault(context);
        HttpsURLConnection.setDefaultSSLSocketFactory(context.getSocketFactory());
        return true;
    }

    private static void addFrom(List<X509ExtendedTrustManager> out, KeyStore ks) throws Exception {
        TrustManagerFactory tmf = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
        tmf.init(ks);
        for (TrustManager tm : tmf.getTrustManagers()) {
            if (tm instanceof X509ExtendedTrustManager)
                out.add((X509ExtendedTrustManager) tm);
        }
    }
}
