// SPDX-License-Identifier: GPL-3.0-only
// PineconeMC Offline - Copyright (C) 2026 PineconeMC Offline Contributors
package org.pineconemc.offlineauth;

final class Json {
    private Json() {}

    /** A JSON string literal for s, with quotes, backslashes and control characters escaped. */
    static String quote(String s) {
        StringBuilder b = new StringBuilder(s.length() + 2).append('"');
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            if (c == '"' || c == '\\') {
                b.append('\\').append(c);
            } else if (c < 0x20) {
                b.append(String.format("\\u%04x", (int) c));
            } else {
                b.append(c);
            }
        }
        return b.append('"').toString();
    }

    /** The contents of a JSON string literal (without its quotes), with backslash escapes removed. Enough for player names. */
    static String unquote(String escaped) {
        StringBuilder b = new StringBuilder(escaped.length());
        for (int i = 0; i < escaped.length(); i++) {
            char c = escaped.charAt(i);
            if (c == '\\' && i + 1 < escaped.length()) {
                c = escaped.charAt(++i);
            }
            b.append(c);
        }
        return b.toString();
    }
}
