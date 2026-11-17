// SPDX-License-Identifier: GPL-3.0-only
// PineconeMC Offline - Copyright (C) 2026 PineconeMC Offline Contributors
package org.pineconemc.systemtrust;

import java.net.Socket;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.List;

import javax.net.ssl.SSLEngine;
import javax.net.ssl.X509ExtendedTrustManager;

// Trusts a certificate chain when any one of its trust managers does (Java's own list first, then Windows').
// Each delegate still does its own hostname check, so this only widens the set of trusted roots.
final class EitherTrustManager extends X509ExtendedTrustManager {
    private interface Check {
        void run(X509ExtendedTrustManager tm) throws CertificateException;
    }

    private final List<X509ExtendedTrustManager> m_delegates;

    EitherTrustManager(List<X509ExtendedTrustManager> delegates) { m_delegates = delegates; }

    private void any(Check check) throws CertificateException {
        CertificateException first = null;
        for (X509ExtendedTrustManager tm : m_delegates) {
            try {
                check.run(tm);
                return;
            } catch (CertificateException e) {
                if (first == null)
                    first = e;
            }
        }
        throw first != null ? first : new CertificateException("no trust managers");
    }

    @Override
    public void checkServerTrusted(X509Certificate[] chain, String authType) throws CertificateException {
        any(tm -> tm.checkServerTrusted(chain, authType));
    }

    @Override
    public void checkServerTrusted(X509Certificate[] chain, String authType, Socket socket) throws CertificateException {
        any(tm -> tm.checkServerTrusted(chain, authType, socket));
    }

    @Override
    public void checkServerTrusted(X509Certificate[] chain, String authType, SSLEngine engine) throws CertificateException {
        any(tm -> tm.checkServerTrusted(chain, authType, engine));
    }

    @Override
    public void checkClientTrusted(X509Certificate[] chain, String authType) throws CertificateException {
        any(tm -> tm.checkClientTrusted(chain, authType));
    }

    @Override
    public void checkClientTrusted(X509Certificate[] chain, String authType, Socket socket) throws CertificateException {
        any(tm -> tm.checkClientTrusted(chain, authType, socket));
    }

    @Override
    public void checkClientTrusted(X509Certificate[] chain, String authType, SSLEngine engine) throws CertificateException {
        any(tm -> tm.checkClientTrusted(chain, authType, engine));
    }

    @Override
    public X509Certificate[] getAcceptedIssuers() {
        List<X509Certificate> all = new ArrayList<>();
        for (X509ExtendedTrustManager tm : m_delegates) {
            for (X509Certificate c : tm.getAcceptedIssuers())
                all.add(c);
        }
        return all.toArray(new X509Certificate[0]);
    }
}
