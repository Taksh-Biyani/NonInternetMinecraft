// SPDX-License-Identifier: GPL-3.0-only
// PineconeMC Offline - Copyright (C) 2026 PineconeMC Offline Contributors
package org.pineconemc.systemtrust;

import java.net.Socket;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;
import java.util.Arrays;

import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLEngine;
import javax.net.ssl.X509ExtendedTrustManager;

// Run with: java -cp pinecone-system-trust.jar org.pineconemc.systemtrust.SelfTest (no network needed).
public final class SelfTest {
    private SelfTest() {}

    private static final class Fixed extends X509ExtendedTrustManager {
        private final boolean m_trusts;
        int calls;

        Fixed(boolean trusts) { m_trusts = trusts; }

        private void check() throws CertificateException {
            calls++;
            if (!m_trusts)
                throw new CertificateException("not trusted");
        }

        @Override public void checkServerTrusted(X509Certificate[] c, String a) throws CertificateException { check(); }
        @Override public void checkServerTrusted(X509Certificate[] c, String a, Socket s) throws CertificateException { check(); }
        @Override public void checkServerTrusted(X509Certificate[] c, String a, SSLEngine e) throws CertificateException { check(); }
        @Override public void checkClientTrusted(X509Certificate[] c, String a) throws CertificateException { check(); }
        @Override public void checkClientTrusted(X509Certificate[] c, String a, Socket s) throws CertificateException { check(); }
        @Override public void checkClientTrusted(X509Certificate[] c, String a, SSLEngine e) throws CertificateException { check(); }
        @Override public X509Certificate[] getAcceptedIssuers() { return new X509Certificate[0]; }
    }

    private static void expect(boolean ok, String what) {
        if (!ok) {
            System.out.println("FAIL: " + what);
            System.exit(1);
        }
        System.out.println("ok: " + what);
    }

    private static boolean trusts(X509ExtendedTrustManager tm) {
        try {
            tm.checkServerTrusted(new X509Certificate[0], "RSA", (SSLEngine) null);
            return true;
        } catch (CertificateException e) {
            return false;
        }
    }

    public static void main(String[] args) throws Exception {
        Fixed javaNo = new Fixed(false), windowsYes = new Fixed(true);
        expect(trusts(new EitherTrustManager(Arrays.asList(javaNo, windowsYes))), "trusted when only Windows trusts it");
        expect(javaNo.calls == 1 && windowsYes.calls == 1, "both lists were asked");

        Fixed javaYes = new Fixed(true), windowsUnused = new Fixed(true);
        expect(trusts(new EitherTrustManager(Arrays.asList(javaYes, windowsUnused))), "trusted when Java trusts it");
        expect(windowsUnused.calls == 0, "Windows list not asked when Java already trusts it");

        expect(!trusts(new EitherTrustManager(Arrays.asList(new Fixed(false), new Fixed(false)))), "rejected when neither trusts it");

        if (System.getProperty("os.name", "").startsWith("Windows")) {
            SSLContext before = SSLContext.getDefault();
            expect(Agent.install(), "agent switches on with this Java on Windows");
            expect(SSLContext.getDefault() != before, "default TLS context replaced");
        }
        System.out.println("SYSTEM_TRUST_SELFTEST_OK");
    }
}
