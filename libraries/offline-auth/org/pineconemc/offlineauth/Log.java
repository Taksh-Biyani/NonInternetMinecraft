// SPDX-License-Identifier: GPL-3.0-only
// PineconeMC Offline - Copyright (C) 2026 PineconeMC Offline Contributors
package org.pineconemc.offlineauth;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintStream;

/** Writes to standard output (the launcher's game console) and, if given, a log file that's replaced on every start. */
final class Log {
    private static final String PREFIX = "[pinecone-offline-auth] ";
    private final PrintStream file;

    Log(File path) {
        PrintStream p = null;
        if (path != null) {
            try {
                File dir = path.getAbsoluteFile().getParentFile();
                if (dir != null) {
                    dir.mkdirs();
                }
                p = new PrintStream(new FileOutputStream(path, false), true, "UTF-8");
            } catch (IOException e) {
                System.out.println(PREFIX + "can't write the log file " + path + ": " + e);
            }
        }
        file = p;
    }

    synchronized void line(String message) {
        System.out.println(PREFIX + message);
        if (file != null) {
            file.println(PREFIX + message);
        }
    }
}
