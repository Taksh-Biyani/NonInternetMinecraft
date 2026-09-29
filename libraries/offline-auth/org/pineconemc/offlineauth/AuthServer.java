// SPDX-License-Identifier: GPL-3.0-only
// PineconeMC Offline - Copyright (C) 2026 PineconeMC Offline Contributors
package org.pineconemc.offlineauth;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.StandardCharsets;

/**
 * A minimal HTTP/1.1 server on 127.0.0.1 (one request per connection). It only needs java.base, because the
 * trimmed Java runtimes Mojang ships may not include the jdk.httpserver module.
 */
final class AuthServer {
    private static final int MAX_BODY = 1 << 20;

    private final ServerSocket socket;
    private final Router router;
    private final Log log;

    /** Binds immediately, so the server is reachable as soon as the constructor returns. Port 0 picks a free port. */
    AuthServer(int port, Router router, Log log) throws IOException {
        this.socket = new ServerSocket(port, 50, InetAddress.getLoopbackAddress());
        this.router = router;
        this.log = log;
    }

    int port() {
        return socket.getLocalPort();
    }

    void start() {
        Thread acceptor = new Thread(this::acceptLoop, "pinecone-offline-auth");
        acceptor.setDaemon(true);
        acceptor.start();
    }

    void close() {
        try {
            socket.close();
        } catch (IOException ignored) {
            // closing anyway
        }
    }

    private void acceptLoop() {
        while (!socket.isClosed()) {
            final Socket client;
            try {
                client = socket.accept();
            } catch (IOException e) {
                if (socket.isClosed()) {
                    return;
                }
                log.line("accept failed: " + e);
                continue;
            }
            Thread worker = new Thread(() -> serve(client), "pinecone-offline-auth-request");
            worker.setDaemon(true);
            worker.start();
        }
    }

    private void serve(Socket client) {
        try (Socket c = client) {
            c.setSoTimeout(10000);
            InputStream in = new BufferedInputStream(c.getInputStream());
            String requestLine = readLine(in);
            if (requestLine == null || requestLine.isEmpty()) {
                return;
            }
            String[] parts = requestLine.split(" ");
            if (parts.length < 2) {
                return;
            }
            int contentLength = 0;
            String line;
            while ((line = readLine(in)) != null && !line.isEmpty()) {
                int colon = line.indexOf(':');
                if (colon > 0 && line.substring(0, colon).trim().equalsIgnoreCase("Content-Length")) {
                    contentLength = Integer.parseInt(line.substring(colon + 1).trim());
                }
            }
            byte[] body = new byte[Math.max(0, Math.min(contentLength, MAX_BODY))];
            int read = 0;
            while (read < body.length) {
                int n = in.read(body, read, body.length - read);
                if (n < 0) {
                    break;
                }
                read += n;
            }
            Router.Response response = router.handle(parts[0], parts[1], new String(body, 0, read, StandardCharsets.UTF_8));
            log.line(parts[0] + " " + parts[1] + " -> " + response.status);
            write(c.getOutputStream(), response);
        } catch (IOException | RuntimeException e) {
            log.line("request failed: " + e);
        }
    }

    private static String readLine(InputStream in) throws IOException {
        ByteArrayOutputStream buf = new ByteArrayOutputStream();
        int b;
        while ((b = in.read()) != -1 && b != '\n') {
            if (b != '\r') {
                buf.write(b);
            }
        }
        if (b == -1 && buf.size() == 0) {
            return null;
        }
        return new String(buf.toByteArray(), StandardCharsets.UTF_8);
    }

    private static void write(OutputStream out, Router.Response response) throws IOException {
        byte[] body = response.body.getBytes(StandardCharsets.UTF_8);
        String head = "HTTP/1.1 " + response.status + " " + reason(response.status) + "\r\n"
                + "Content-Type: application/json; charset=utf-8\r\n"
                + "Content-Length: " + body.length + "\r\n"
                + "Connection: close\r\n\r\n";
        out.write(head.getBytes(StandardCharsets.US_ASCII));
        out.write(body);
        out.flush();
    }

    private static String reason(int status) {
        switch (status) {
            case 200:
                return "OK";
            case 204:
                return "No Content";
            case 404:
                return "Not Found";
            default:
                return "Status";
        }
    }
}
